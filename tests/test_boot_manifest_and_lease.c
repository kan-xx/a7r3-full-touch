/* Host unit tests for 128-byte A7TBM001 boot_manifest (tbm_validate) and
 * STREAM_FILE88B {5, 15} JPEG/RAW discrete image_lease + image_reference_read.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "boot_manifest.h"
#include "image_lease.h"
#include "image_reference_read.h"

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

    printf("test_boot_manifest_and_lease cases=%u checks=%u passed\n", cases, checks);
#undef CK
    return 0;
}
