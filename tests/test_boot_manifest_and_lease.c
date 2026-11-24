/* Host unit tests for 128-byte A7TBM001 boot_manifest (tbm_validate) and
 * STREAM_FILE88B {5, 15} JPEG/RAW discrete image_lease + image_reference_read.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "boot_manifest.h"
#include "bounded_file_hash.h"
#include "power_gate.h"
#include "power_layout.h"
#include "image_lease.h"
#include "image_reference_read.h"

struct fake_bf_fs {
    uint8_t elf[256];
    uint32_t size;
    uint32_t pos;
    int open_fd;
};

static int fake_bf_open(void *ctx, const char *path) {
    struct fake_bf_fs *fs = ctx;
    (void)path;
    if (fs->open_fd >= 0) return -1;
    fs->pos = 0;
    fs->open_fd = 7;
    return 7;
}

static int fake_bf_stat(void *ctx, int fd, struct bf_identity *out) {
    struct fake_bf_fs *fs = ctx;
    if (fd != 7) return -1;
    *out = (struct bf_identity){0xfc0fu, 1001u, fs->size, 0100555u, 1u, 0u, 0u, 10u, 0u, 10u, 0u};
    return 0;
}

static int fake_bf_read(void *ctx, int fd, void *out, uint32_t n) {
    struct fake_bf_fs *fs = ctx;
    if (fd != 7) return -1;
    uint32_t left = fs->size - fs->pos;
    uint32_t take = left < n ? left : n;
    if (take) memcpy(out, fs->elf + fs->pos, take);
    fs->pos += take;
    return (int)take;
}

static int fake_bf_close(void *ctx, int fd) {
    struct fake_bf_fs *fs = ctx;
    if (fd != 7) return -1;
    fs->open_fd = -1;
    return 0;
}

struct fake_pg {
    uint32_t live[12];
    uint32_t report[2048];
};

static int fake_pg_live(void *ctx, uint32_t out[12]) {
    struct fake_pg *pg = ctx;
    memcpy(out, pg->live, sizeof(pg->live));
    return 0;
}

static int fake_pg_report(void *ctx, uint32_t *out, uint32_t bytes) {
    struct fake_pg *pg = ctx;
    if (bytes < sizeof(pg->report)) return 1;
    memcpy(out, pg->report, sizeof(pg->report));
    return 0;
}

struct fake_mem {
    uint32_t obj_base;
    uint32_t view3_base;
    uint32_t complete;
    uint32_t view_obj;
    uint32_t model_vptr;
    uint32_t complete_vptr;
    uint32_t stream_vptr;
    uint32_t view_vptr;
    uint32_t view_phase;
    uint32_t model_phase;
    uint32_t width;
    uint32_t height;
    uint32_t file_size;
    uint32_t raw_type;
    uint8_t busy[2];
    char dir[10];
    char file[14];
    unsigned reads;
    unsigned tear_on_second;
};

static int fake_pi_read(void *ctx, uint32_t addr, void *out, uint32_t n) {
    struct fake_mem *m = ctx;
    ++m->reads;
    if (m->tear_on_second && m->reads == 20) {
        m->file_size += 4096;
    }
    if (addr == m->complete + 0x8a4 && n == 4) {
        memcpy(out, &m->model_vptr, 4);
        return 1;
    }
    if (addr == m->complete && n == 4) {
        memcpy(out, &m->complete_vptr, 4);
        return 1;
    }
    if (addr == m->complete + 0x11c && n == 4) {
        memcpy(out, &m->stream_vptr, 4);
        return 1;
    }
    if (addr == m->view_obj && n == 4) {
        memcpy(out, &m->view_vptr, 4);
        return 1;
    }
    if (addr == m->view_obj + 4 && n == 4) {
        memcpy(out, &m->view_phase, 4);
        return 1;
    }
    if (addr == m->complete + 0x20 && n == 4) {
        memcpy(out, &m->model_phase, 4);
        return 1;
    }
    if (addr == m->complete + 0x88 && n == 2) {
        memcpy(out, m->busy, 2);
        return 1;
    }
    if (addr == m->complete + 0x194 && n == 8) {
        uint32_t wh[2] = {m->width, m->height};
        memcpy(out, wh, 8);
        return 1;
    }
    if (addr == m->complete + 0x1bc && n == 4) {
        memcpy(out, &m->file_size, 4);
        return 1;
    }
    if (addr == m->complete + 0x184 && n == 4) {
        memcpy(out, &m->raw_type, 4);
        return 1;
    }
    if (addr == m->complete + 0x1cc && n == 10) {
        memcpy(out, m->dir, 10);
        return 1;
    }
    if (addr == m->complete + 0x1d6 && n == 14) {
        memcpy(out, m->file, 14);
        return 1;
    }
    return 0;
}

static struct touch_boot_manifest valid_manifest(void) {
    struct touch_boot_manifest m;
    memset(&m, 0, sizeof(m));
    memcpy(m.magic, "A7TBM001", 8);
    m.abi = 1;
    m.bytes = 128;
    m.build = 0x3111fe07u;
    m.entry_api = 1;
    m.core_bytes = 161576u;
    m.entry_offset = 143856u;
    m.core_sha256[0] = 0x27;
    m.entry_anchor[0] = 0x2d;
    m.owner_sha256[31] = 0x63;
    m.installation[15] = 0xf6;
    return m;
}

static void reset_mem(struct fake_mem *m, struct pi_source *src) {
    memset(m, 0, sizeof(*m));
    m->obj_base = 0x10000000u;
    m->view3_base = 0x18000000u;
    m->complete = 0x21000000u;
    m->view_obj = 0x20000000u;
    m->model_vptr = m->obj_base + 0x11fee30u;
    m->complete_vptr = m->obj_base + 0x11fecf8u;
    m->stream_vptr = m->obj_base + 0xdc1f08u;
    m->view_vptr = m->view3_base + 0xa8f48u;
    m->view_phase = 3;
    m->model_phase = 3;
    m->width = 7952;
    m->height = 5304;
    m->file_size = 24500000;
    m->raw_type = PI_RAW_TYPE_NORMAL_STILL; /* 5 = JPEG */
    memcpy(m->dir, "100MSDCF", 9);
    memcpy(m->file, "DSC00001.JPG", 13);

    *src = (struct pi_source){
        .obj_base = m->obj_base,
        .view3_base = m->view3_base,
        .e9_element = 0x22000000u,
        .e9_model = m->complete + 0x8a4u,
        .e9_complete = m->complete,
        .view_object = m->view_obj,
        .packed_power = 9,
        .policy_epoch = 1,
        .view_epoch = 1,
        .observed_media_epoch = 0,
        .flags = 7,
    };
}

int main(void) {
    unsigned cases = 0, checks = 0;
#define CK(cond)                                                                        \
    do {                                                                                \
        ++checks;                                                                       \
        if (!(cond)) {                                                                  \
            printf("test_boot_manifest_and_lease FAILED at line %d\n", __LINE__);       \
            return 1;                                                                   \
        }                                                                               \
    } while (0)

    /* Part 1: 128-byte A7TBM001 boot_manifest validator */
    ++cases;
    CK(sizeof(struct touch_boot_manifest) == 128);
    struct touch_boot_manifest m = valid_manifest();
    CK(tbm_validate(&m) == 1);
    CK(tbm_validate(0) == 0);

    struct touch_boot_manifest bad;
    bad = m; bad.magic[7] = '2'; CK(!tbm_validate(&bad));
    bad = m; bad.abi = 2; CK(!tbm_validate(&bad));
    bad = m; bad.bytes = 64; CK(!tbm_validate(&bad));
    bad = m; bad.build = 0x3111fe06u; CK(!tbm_validate(&bad));
    bad = m; bad.entry_api = 0; CK(!tbm_validate(&bad));
    bad = m; bad.core_bytes = 16383; CK(!tbm_validate(&bad));
    bad = m; bad.core_bytes = 1048577; CK(!tbm_validate(&bad));
    bad = m; bad.entry_offset = 0; CK(!tbm_validate(&bad));
    bad = m; bad.entry_offset = 0x01000002u; CK(!tbm_validate(&bad));
    bad = m; bad.entry_offset = 0x01000001u; CK(tbm_validate(&bad) == 1); /* Thumb bit cleared <= 0x01000000 */
    bad = m; memset(bad.core_sha256, 0, 32); CK(!tbm_validate(&bad));
    bad = m; memset(bad.entry_anchor, 0, 16); CK(!tbm_validate(&bad));
    bad = m; memset(bad.owner_sha256, 0, 32); CK(!tbm_validate(&bad));
    bad = m; memset(bad.installation, 0, 16); CK(!tbm_validate(&bad));

    /* Part 2: STREAM_FILE88B {5, 15} discrete JPEG / RAW lease & reference reader */
    ++cases;
    CK(sizeof(struct pi_key) == 88);
    for (uint32_t ty = 0; ty < 32; ++ty) {
        CK(pi_raw_type_supported(ty) == (ty == 5 || ty == 15));
    }
    CK(!pi_raw_type_supported(0xffffffffu));

    struct fake_mem mem;
    struct pi_source src;
    struct pi_read_port port = {&mem, fake_pi_read};
    struct pi_key key_jpg, key_raw, key_tmp;
    struct pi_read_detail detail;
    uint32_t flags = 0, cookie = 0;
    struct pi_lease lease;
    pi_init(&lease);

    /* JPEG (m_contType == 5) initial acquisition -> PI_NEW (gen 1) */
    ++cases;
    reset_mem(&mem, &src);
    CK(pi_read_reference_detailed(&port, &src, &key_jpg, &flags, &detail) == PIR_OK);
    CK(detail.checked_mask == 0x80000000u && detail.raw_type == 5 && detail.view_phase == 3);
    CK((flags & (PI_REQUIRED | PI_NATIVE_READY)) == (PI_REQUIRED | PI_NATIVE_READY));
    CK(pi_observe(&lease, &key_jpg, flags, &cookie) == PI_NEW && cookie == 1 && lease.valid);

    /* ZoomPb phase 3 -> phase 4 transition on same JPEG preserves lease (PI_SAME, gen 1) */
    ++cases;
    mem.view_phase = 4;
    CK(pi_read_reference(&port, &src, &key_tmp, &flags) == PIR_OK);
    CK(pi_observe(&lease, &key_tmp, flags, &cookie) == PI_SAME && cookie == 1 && lease.key.view_phase == 4);

    /* Dual-slot JPEG (5) -> RAW .ARW (15) discrete switch -> PI_CHANGED (gen 2) */
    ++cases;
    mem.raw_type = PI_RAW_TYPE_RAW_STILL; /* 15 */
    memcpy(mem.file, "DSC00001.ARW", 13);
    mem.file_size = 85400000;
    CK(pi_read_reference_detailed(&port, &src, &key_raw, &flags, &detail) == PIR_OK);
    CK(detail.raw_type == 15);
    CK(pi_observe(&lease, &key_raw, flags, &cookie) == PI_CHANGED && cookie == 2 && lease.key.raw_type == 15);

    /* Same RAW image with same filename/size but hypothetical raw_type flip 15 -> 5 also invalidates */
    ++cases;
    key_tmp = key_raw;
    key_tmp.raw_type = 5;
    CK(pi_observe(&lease, &key_tmp, flags, &cookie) == PI_CHANGED && cookie == 3);

    /* Unsupported media types (e.g., XAVC S MP4 = 18, MPO/other = 4, 14, 0) reject with PIR_TYPED & revoke lease */
    const uint32_t unsupported[] = {0, 1, 4, 6, 14, 16, 18, 0xffffffffu};
    for (unsigned i = 0; i < sizeof(unsupported) / sizeof(unsupported[0]); ++i) {
        ++cases;
        reset_mem(&mem, &src);
        mem.raw_type = unsupported[i];
        CK(pi_read_reference_detailed(&port, &src, &key_tmp, &flags, &detail) == PIR_TYPED);
        CK(detail.checked_mask == (0x80000000u | 16u) && detail.raw_type == unsupported[i]);
        key_tmp = key_raw;
        key_tmp.raw_type = unsupported[i];
        CK(pi_observe(&lease, &key_tmp, PI_REQUIRED, &cookie) == PI_INVALID && !lease.valid && cookie == 0);
    }

    /* Double-read tearing, model busy, bad filename, and generation exhaustion */
    ++cases;
    reset_mem(&mem, &src);
    mem.tear_on_second = 1;
    CK(pi_read_reference(&port, &src, &key_tmp, &flags) == PIR_CHANGED);

    reset_mem(&mem, &src);
    mem.model_phase = 2;
    CK(pi_read_reference(&port, &src, &key_tmp, &flags) == PIR_BUSY);

    reset_mem(&mem, &src);
    mem.file[0] = '\0';
    CK(pi_read_reference(&port, &src, &key_tmp, &flags) == PIR_NAME);

    reset_mem(&mem, &src);
    CK(pi_read_reference(&port, &src, &key_jpg, &flags) == PIR_OK);
    lease.valid = 0;
    lease.generation = 0xffffffffu;
    CK(pi_observe(&lease, &key_jpg, flags, &cookie) == PI_EXHAUSTED && lease.exhausted && !lease.valid);
    CK(pi_observe(&lease, &key_jpg, flags, &cookie) == PI_EXHAUSTED);

    /* Part 3: BF_DYNAMIC_ELF accepts arbitrary user-compiled ARM32 ET_DYN ELF & rejects non-ELF */
    ++cases;
    struct fake_bf_fs fs;
    memset(&fs, 0, sizeof(fs));
    fs.open_fd = -1;
    fs.size = 128;
    memcpy(fs.elf, "\x7f""ELF\x01\x01\x01", 7);
    fs.elf[16] = 3;  /* ET_DYN */
    fs.elf[18] = 40; /* EM_ARM */
    struct bf_config cfgs[BF_FILES];
    for (unsigned i = 0; i < BF_FILES; ++i) {
        cfgs[i] = (struct bf_config){"/usr/lib/liba7r_menu_product_v21.so", 128, 0555, BF_ROOT_ONLY | BF_DYNAMIC_ELF, {0}};
    }
    struct bf_ports bfp = {&fs, fake_bf_open, fake_bf_stat, fake_bf_read, fake_bf_close};
    struct bf_state bfs;
    memset(&bfs, 0, sizeof(bfs));
    CK(bf_init(&bfs) == BF_MORE);
    CK(bf_step(&bfs, cfgs, &bfp) == BF_MORE && (bfs.verified_mask & 1u) == 1u);
    CK(bf_cancel(&bfs, &bfp) == BF_DONE);

    fs.elf[0] = 'R'; /* Corrupt ELF magic */
    memset(&bfs, 0, sizeof(bfs));
    CK(bf_init(&bfs) == BF_MORE);
    CK(bf_step(&bfs, cfgs, &bfp) == BF_FAILED);

    /* Part 4: power_gate compiler-independent live_state callback mode */
    ++cases;
    struct fake_pg pg;
    memset(&pg, 0, sizeof(pg));
    pg.live[0] = (2u << 3) | 5u; /* gen=2, ACT=5 */
    pg.live[2] = 1;              /* prepared=1 */
    pg.live[5] = 1;              /* bg_ready=1 */
    pg.live[6] = 22;             /* global_phase=22 */
    pg.live[10] = 4;             /* bootstrap_state=4 */
    pg.live[11] = 2;             /* act_claim=2 */
    pg.report[PG_W_ABI] = PG_REPORT_ABI;
    pg.report[PG_W_BYTES] = PG_REPORT_BYTES;
    pg.report[PG_W_BUILD] = PG_REPORT_BUILD;
    pg.report[PG_W_PHASE] = 22;
    pg.report[PG_W_READY] = 1;
    pg.report[PG_W_LO_EVENT] = 5;
    pg.report[PG_W_LO_GENERATION] = 2;
    pg.report[PG_W_WORKER_GENERATION] = 2;
    pg.report[PG_W_ACT_CLAIM] = 2;
    struct pg_ports pgp = {&pg, 0, fake_pg_report, fake_pg_live};
    struct pg_proof proof;
    CK(pg_check(&pgp, &proof) == 1 && proof.generation == 2);
    pg.live[0] = (2u << 3) | 2u; /* INACT=2 */
    uint32_t rev = 0;
    CK(pg_revoked(&pgp, &rev) == 1 && rev == pg.live[0]);

    printf("test_boot_manifest_and_lease cases=%u checks=%u passed\n", cases, checks);
#undef CK
    return 0;
}
