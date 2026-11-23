"""Pure-stdlib 32-bit ARM ELF reader, DT_RELCOUNT normalizer, and PLT decoder.

Avoids external `pyelftools` / `capstone` dependencies so offline unit tests,
manifest sealing, and firmware DSO builds work under any standard Python 3.10+.
"""
from __future__ import annotations

import struct
from dataclasses import dataclass

ET_DYN = 3
EM_ARM = 40

PT_LOAD = 1
PF_W = 0x2
PF_X = 0x1

SHT_DYNSYM = 11
SHT_REL = 9

DT_NULL = 0
DT_NEEDED = 1
DT_INIT = 12
DT_FINI = 13
DT_TEXTREL = 22
DT_INIT_ARRAY = 25
DT_FINI_ARRAY = 26
DT_PREINIT_ARRAY = 32
DT_RELCOUNT = 0x6FFFFFFA

SHN_UNDEF = 0

FORBIDDEN_DT_TAGS = {
    DT_INIT: "DT_INIT",
    DT_FINI: "DT_FINI",
    DT_TEXTREL: "DT_TEXTREL",
    DT_INIT_ARRAY: "DT_INIT_ARRAY",
    DT_FINI_ARRAY: "DT_FINI_ARRAY",
    DT_PREINIT_ARRAY: "DT_PREINIT_ARRAY",
}


@dataclass(frozen=True)
class ElfSegment:
    p_type: int
    p_offset: int
    p_vaddr: int
    p_paddr: int
    p_filesz: int
    p_memsz: int
    p_flags: int
    p_align: int


@dataclass(frozen=True)
class ElfSection:
    name: str
    sh_type: int
    sh_flags: int
    sh_addr: int
    sh_offset: int
    sh_size: int
    sh_link: int
    sh_info: int
    sh_addralign: int
    sh_entsize: int


@dataclass(frozen=True)
class ElfSymbol:
    name: str
    st_value: int
    st_size: int
    st_info: int
    st_other: int
    st_shndx: int


def _read_cstr(buf: bytes | bytearray, offset: int) -> str:
    if offset < 0 or offset >= len(buf):
        return ""
    end = buf.find(b"\x00", offset)
    if end < 0:
        end = len(buf)
    return bytes(buf[offset:end]).decode("ascii", errors="replace")


class Elf32Image:
    """Minimal 32-bit little-endian ELF parser for Sony A7R III ARMv7 DSOs."""

    def __init__(self, raw: bytes | bytearray) -> None:
        if len(raw) < 52 or raw[:4] != b"\x7fELF":
            raise ValueError("Not a valid ELF image")
        self.raw = bytearray(raw)
        self.ei_class = self.raw[4]
        self.ei_data = self.raw[5]
        if self.ei_class != 1 or self.ei_data != 1:
            raise ValueError(f"Expected ELF32LE, got class={self.ei_class} data={self.ei_data}")
        (
            self.e_type,
            self.e_machine,
            _e_version,
            self.e_entry,
            self.e_phoff,
            self.e_shoff,
            self.e_flags,
            _e_ehsize,
            self.e_phentsize,
            self.e_phnum,
            self.e_shentsize,
            self.e_shnum,
            self.e_shstrndx,
        ) = struct.unpack_from("<HHIIIIIHHHHHH", self.raw, 16)

        self.segments: list[ElfSegment] = []
        for idx in range(self.e_phnum):
            off = self.e_phoff + idx * self.e_phentsize
            self.segments.append(ElfSegment(*struct.unpack_from("<8I", self.raw, off)))

        raw_sections: list[tuple[int, int, int, int, int, int, int, int, int, int]] = []
        for idx in range(self.e_shnum):
            off = self.e_shoff + idx * self.e_shentsize
            raw_sections.append(struct.unpack_from("<10I", self.raw, off))

        shstr = b""
        if 0 <= self.e_shstrndx < len(raw_sections):
            sh_off = raw_sections[self.e_shstrndx][4]
            sh_sz = raw_sections[self.e_shstrndx][5]
            shstr = bytes(self.raw[sh_off : sh_off + sh_sz])

        self.sections: list[ElfSection] = []
        self._sec_by_name: dict[str, ElfSection] = {}
        for fields in raw_sections:
            name = _read_cstr(shstr, fields[0])
            sec = ElfSection(name, *fields[1:])
            self.sections.append(sec)
            if name:
                self._sec_by_name[name] = sec

    def get_section(self, name: str) -> ElfSection | None:
        return self._sec_by_name.get(name)

    def section_bytes(self, sec: ElfSection) -> bytes:
        return bytes(self.raw[sec.sh_offset : sec.sh_offset + sec.sh_size])

    def zero_dt_relcount(self) -> bool:
        """Zero DT_RELCOUNT (0x6ffffffa) in .dynamic for Sony glibc-2.11 loader compatibility."""
        dyn = self.get_section(".dynamic")
        if dyn is None:
            return False
        patched = False
        for off in range(dyn.sh_offset, dyn.sh_offset + dyn.sh_size, 8):
            d_tag, _d_val = struct.unpack_from("<II", self.raw, off)
            if d_tag == DT_NULL:
                break
            if d_tag == DT_RELCOUNT:
                struct.pack_into("<I", self.raw, off + 4, 0)
                patched = True
        return patched

    def dynamic_tags(self) -> list[tuple[int, int]]:
        dyn = self.get_section(".dynamic")
        if dyn is None:
            return []
        tags: list[tuple[int, int]] = []
        for off in range(dyn.sh_offset, dyn.sh_offset + dyn.sh_size, 8):
            d_tag, d_val = struct.unpack_from("<II", self.raw, off)
            tags.append((d_tag, d_val))
            if d_tag == DT_NULL:
                break
        return tags

    def needed_libraries(self) -> list[str]:
        dynstr = self.get_section(".dynstr")
        if dynstr is None:
            return []
        strtab = self.section_bytes(dynstr)
        return [_read_cstr(strtab, val) for tag, val in self.dynamic_tags() if tag == DT_NEEDED]

    def dynsym_symbols(self) -> list[ElfSymbol]:
        dynsym = self.get_section(".dynsym")
        if dynsym is None:
            return []
        strtab_sec = self.sections[dynsym.sh_link] if 0 <= dynsym.sh_link < len(self.sections) else None
        strtab = self.section_bytes(strtab_sec) if strtab_sec else b""
        ent = dynsym.sh_entsize or 16
        syms: list[ElfSymbol] = []
        for off in range(dynsym.sh_offset, dynsym.sh_offset + dynsym.sh_size, ent):
            st_name, st_value, st_size, st_info, st_other, st_shndx = struct.unpack_from(
                "<IIIBBH", self.raw, off
            )
            syms.append(
                ElfSymbol(
                    name=_read_cstr(strtab, st_name),
                    st_value=st_value,
                    st_size=st_size,
                    st_info=st_info,
                    st_other=st_other,
                    st_shndx=st_shndx,
                )
            )
        return syms

    def relocations(self) -> list[tuple[int, int, str, ElfSymbol]]:
        """Return (r_offset, r_type, sym_name, symbol) for all SHT_REL sections."""
        out: list[tuple[int, int, str, ElfSymbol]] = []
        syms_cache: dict[int, list[ElfSymbol]] = {}
        for sec in self.sections:
            if sec.sh_type != SHT_REL:
                continue
            if sec.sh_link not in syms_cache:
                sym_sec = self.sections[sec.sh_link]
                str_sec = self.sections[sym_sec.sh_link]
                strtab = self.section_bytes(str_sec)
                ent = sym_sec.sh_entsize or 16
                slist: list[ElfSymbol] = []
                for off in range(sym_sec.sh_offset, sym_sec.sh_offset + sym_sec.sh_size, ent):
                    st_name, st_value, st_size, st_info, st_other, st_shndx = struct.unpack_from(
                        "<IIIBBH", self.raw, off
                    )
                    slist.append(
                        ElfSymbol(_read_cstr(strtab, st_name), st_value, st_size, st_info, st_other, st_shndx)
                    )
                syms_cache[sec.sh_link] = slist
            syms = syms_cache[sec.sh_link]
            ent = sec.sh_entsize or 8
            for off in range(sec.sh_offset, sec.sh_offset + sec.sh_size, ent):
                r_offset, r_info = struct.unpack_from("<II", self.raw, off)
                sym_idx = r_info >> 8
                r_type = r_info & 0xFF
                sym = syms[sym_idx]
                out.append((r_offset, r_type, sym.name, sym))
        return out

    def plt_map(self) -> dict[int, str]:
        """Decode ARM32 SysV .plt stubs (plus Thumb-to-ARM veneer 0x46c04778) to symbol names."""
        plt = self.get_section(".plt")
        if plt is None:
            return {}
        got = {r_off: name for r_off, _rt, name, _sym in self.relocations() if name}
        raw = self.section_bytes(plt)
        out: dict[int, str] = {}

        def decode_rot_imm(word: int) -> int:
            imm8 = word & 0xFF
            rot = ((word >> 8) & 0xF) * 2
            return ((imm8 >> rot) | (imm8 << (32 - rot))) & 0xFFFFFFFF if rot else imm8

        for off in range(0, len(raw) - 11, 4):
            w0, w1, w2 = struct.unpack_from("<3I", raw, off)
            # ARM32 PLT entry:
            #   add ip, pc, #imm0   (0xe28fc...)
            #   add ip, ip, #imm1   (0xe28cc...)
            #   ldr pc, [ip, #imm2]! (0xe5bcf...)
            if (
                (w0 & 0xFFFFF000) == 0xE28FC000
                and (w1 & 0xFFFFF000) == 0xE28CC000
                and (w2 & 0xFFFFF000) == 0xE5BCF000
            ):
                addr = plt.sh_addr + off
                target_got = (addr + 8 + decode_rot_imm(w0) + decode_rot_imm(w1) + (w2 & 0xFFF)) & 0xFFFFFFFF
                if target_got in got:
                    out[addr] = got[target_got]
                    if off >= 4 and raw[off - 4 : off] == b"\x78\x47\xc0\x46":
                        out[addr - 4] = got[target_got]
        return out

    def read_vaddr(self, vaddr: int, size: int, require_exec: bool = False) -> bytes:
        for seg in self.segments:
            if seg.p_type != PT_LOAD:
                continue
            if require_exec and not (seg.p_flags & PF_X):
                continue
            if seg.p_vaddr <= vaddr and vaddr + size <= seg.p_vaddr + seg.p_filesz:
                rel = vaddr - seg.p_vaddr
                return bytes(self.raw[seg.p_offset + rel : seg.p_offset + rel + size])
        raise ValueError(f"VA range 0x{vaddr:x}+0x{size:x} not backed by matching PT_LOAD")

    def verify_dso_invariants(self) -> dict[str, object]:
        if self.e_type != ET_DYN or self.e_machine != EM_ARM:
            raise ValueError(f"Expected ET_DYN EM_ARM, got type={self.e_type} machine={self.e_machine}")
        for tag, _val in self.dynamic_tags():
            if tag in FORBIDDEN_DT_TAGS:
                raise ValueError(f"Forbidden dynamic constructor/textrel tag {FORBIDDEN_DT_TAGS[tag]}")
        for seg in self.segments:
            if seg.p_type == PT_LOAD and (seg.p_flags & (PF_W | PF_X)) == (PF_W | PF_X):
                raise ValueError("Forbidden RWX PT_LOAD segment")
        syms = self.dynsym_symbols()
        exports = sorted(s.name for s in syms if s.name and s.st_shndx != SHN_UNDEF)
        export_entries = {s.name: s.st_value for s in syms if s.name and s.st_shndx != SHN_UNDEF}
        imports = sorted(s.name for s in syms if s.name and s.st_shndx == SHN_UNDEF)
        return {
            "exports": exports,
            "export_entries": export_entries,
            "imports": imports,
            "needed": self.needed_libraries(),
        }
