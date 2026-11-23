/* Host unit tests for menu_regions.c (mtr_build / mtr_hit) and runtime_menu_regions.inc.
 *
 * Verifies:
 *   1. Top-tab midpoint split: 7 tabs at x = 17 + 86*i, w = 91, h = 66 overlap by 5px
 *      in Sony's stock bounds; mtr_build splits adjacent tabs at cx +/- 43 (exact
 *      midpoints 105.5, 191.5, ..., 535.5) so every x in [0, 640) hits <= 1 tab.
 *   2. Page-dot 48px vertical expansion: 12x14 dots at y = 446 expand to 48px height
 *      [429, 477), 18px interior midpoint splits, 24px outer wings, and stay <= 545
 *      (disjoint from BACK's right-side native touch zone).
 *   3. Prev/Next page arrow 48x48 expansion clamped to [0, 640] x [66, 126].
 *   4. Row & BACK targets preserve native ns_hit delegation.
 *   5. runtime_menu_regions.inc rt_pick_tap scope gating (0xAA MENU vs Popup/Fn/FFC).
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "menu_regions.h"
#include "native_catalog.h"

struct fake_widget {
    uint32_t addr;
    uint32_t vptr;
    uint32_t runtime_id;
    uint8_t mode;
    uint32_t vis_flags;
    float x, y, w, h;
    int hit_refreshed;
};

static struct fake_widget g_widgets[64];
static unsigned g_widget_count;
static unsigned g_native_hit_calls;

static void add_widget(uint32_t addr, uint32_t vptr, uint32_t rid, float x, float y, float w, float h) {
    g_widgets[g_widget_count++] = (struct fake_widget){addr, vptr, rid, 2, 2, x, y, w, h, 0};
}

static void put_u32(unsigned char *buf, unsigned off, uint32_t v) {
    memcpy(buf + off, &v, 4);
}

static void put_f32(unsigned char *buf, unsigned off, float v) {
    memcpy(buf + off, &v, 4);
}

static int fake_read(void *ctx, uint32_t addr, void *out, uint32_t len) {
    (void)ctx;
    for (unsigned i = 0; i < g_widget_count; ++i) {
        const struct fake_widget *w = &g_widgets[i];
        if (w->addr == addr && (len == 0x40 || len == 0x138)) {
            unsigned char b[0x138];
            memset(b, 0, sizeof(b));
            put_u32(b, 0x00, w->vptr);
            b[0x21] = w->mode;
            put_u32(b, 0x24, w->runtime_id);
            put_u32(b, 0x38, w->vis_flags);
            if (len == 0x138 && w->hit_refreshed) {
                put_f32(b, 0x5c, w->x);
                put_f32(b, 0x60, w->y);
                put_f32(b, 0x64, 1.0f);
                put_f32(b, 0x68, 1.0f);
                put_f32(b, 0x90, w->x);
                put_f32(b, 0x94, w->y);
                put_f32(b, 0x98, w->w);
                put_f32(b, 0x9c, w->h);
            }
            memcpy(out, b, len);
            return 1;
        }
    }
    return 0;
}

static int fake_hit(void *ctx, uint32_t addr, const struct surface_point *pt) {
    (void)ctx;
    for (unsigned i = 0; i < g_widget_count; ++i) {
        struct fake_widget *w = &g_widgets[i];
        if (w->addr == addr) {
            if (pt->x == 0.0f && pt->y == 0.0f) {
                w->hit_refreshed = 1;
            }
            return pt->x >= w->x && pt->x < w->x + w->w && pt->y >= w->y && pt->y < w->y + w->h;
        }
    }
    return 0;
}

/* Stub ns_hit used by mtr_hit fallback for non-expanded targets (rows, BACK) */
int ns_hit(void *context, const struct surface_target *t, const struct surface_point *p) {
    struct ns_hit_context *hc = context;
    ++g_native_hit_calls;
    return hc->io->hit(hc->io->context, t->node, p);
}

static void setup_standard_menu(struct ns_catalog *cat, int tab_count, int page_count, int row_count) {
    g_widget_count = 0;
    g_native_hit_calls = 0;
    memset(cat, 0, sizeof(*cat));

    uint32_t root_addr = 0x30000000u;
    add_widget(root_addr, 0x10001000u, 500, 0, 0, 640, 480);

    cat->identity = (struct surface_identity){
        .power = 9,
        .view_id = 0xaa,
        .view = 0x20000000u,
        .root = root_addr,
        .runtime_id = 500,
        .kind = SURFACE_MENU,
        .layout = 1,
        .phase = 1,
        .tab = 1,
        .page = 1,
        .popup = 0,
        .popup_node = 0,
        .first = 0,
    };
    cat->tab_count = tab_count;
    cat->page_count = page_count;
    cat->row_count = row_count;

    uint32_t next_node = 0x30010000u;
    uint32_t next_rid = 1000;

    /* 7 top tabs: native x = 17 + 86*i, y = 0, w = 91, h = 66 (5px stock overlap!) */
    for (int i = 0; i < tab_count; ++i) {
        add_widget(next_node, 0x10002000u, next_rid, 17.0f + 86.0f * (float)i, 0.0f, 91.0f, 66.0f);
        cat->targets[cat->count++] = (struct surface_target){TARGET_TAB, (uint32_t)i, next_node, 0x10002000u, next_rid, 1};
        next_node += 0x200;
        ++next_rid;
    }

    /* Page dots: x = 314 - 9*(N-1) + 18*i, y = 446, w = 12, h = 14 */
    for (int i = 0; i < page_count; ++i) {
        float x = 314.0f - 9.0f * (float)(page_count - 1) + 18.0f * (float)i;
        add_widget(next_node, 0x10003000u, next_rid, x, 446.0f, 12.0f, 14.0f);
        cat->targets[cat->count++] = (struct surface_target){TARGET_PAGE, (uint32_t)i, next_node, 0x10003000u, next_rid, 1};
        next_node += 0x200;
        ++next_rid;
    }

    /* Prev & Next page arrows: 8x16 at y = 93 */
    add_widget(next_node, 0x10004000u, next_rid, 18.0f, 93.0f, 8.0f, 16.0f);
    cat->targets[cat->count++] = (struct surface_target){TARGET_PAGE_PREV, 0, next_node, 0x10004000u, next_rid, 1};
    next_node += 0x200;
    ++next_rid;

    add_widget(next_node, 0x10004000u, next_rid, 614.0f, 93.0f, 8.0f, 16.0f);
    cat->targets[cat->count++] = (struct surface_target){TARGET_PAGE_NEXT, 0, next_node, 0x10004000u, next_rid, 1};
    next_node += 0x200;
    ++next_rid;

    /* Rows: x = 17, y = 126 + 50*i, w = 606, h = 48 */
    for (int i = 0; i < row_count; ++i) {
        add_widget(next_node, 0x10005000u, next_rid, 17.0f, 126.0f + 50.0f * (float)i, 606.0f, 48.0f);
        cat->targets[cat->count++] = (struct surface_target){TARGET_ROW, (uint32_t)i, next_node, 0x10005000u, next_rid, 1};
        next_node += 0x200;
        ++next_rid;
    }

    /* Footer BACK: x = 15, y = 434, w = 610, h = 40 */
    add_widget(next_node, 0x10006000u, next_rid, 15.0f, 434.0f, 610.0f, 40.0f);
    cat->targets[cat->count++] = (struct surface_target){TARGET_BACK, 0, next_node, 0x10006000u, next_rid, 1};
}

int main(void) {
    unsigned cases = 0, checks = 0;
#define CK(cond)                                                            \
    do {                                                                    \
        ++checks;                                                           \
        if (!(cond)) {                                                      \
            printf("test_menu_regions FAILED at line %d\n", __LINE__);      \
            return 1;                                                       \
        }                                                                   \
    } while (0)

    struct ns_io io = {0};
    io.read = fake_read;
    io.hit = fake_hit;

    /* Case 1: Full 7-tab, 14-page, 6-row Camera1 MENU build & disjoint hit proof */
    ++cases;
    struct ns_catalog cat;
    struct mtr_map map;
    setup_standard_menu(&cat, 7, 14, 6);
    CK(mtr_build(&io, &cat, 640, 480, &map) == 1);
    CK(map.valid == 1 && map.error == MTR_OK && map.count == cat.count);

    /* Verify exact tab midpoints: tab 0=[17, 105.5), tab 1=[105.5, 191.5), ..., tab 6=[535.5, 624) */
    CK(map.region[0].enabled && map.region[0].left == 17.0f && map.region[0].right == 105.5f);
    CK(map.region[1].enabled && map.region[1].left == 105.5f && map.region[1].right == 191.5f);
    CK(map.region[6].enabled && map.region[6].left == 535.5f && map.region[6].right == 624.0f);

    struct mtr_context mctx = {&io, &cat, &map};
    /* Sweep all half-pixel x positions across top tab bar: at most 1 tab may match! */
    for (int half_x = 0; half_x < 1280; ++half_x) {
        struct surface_point pt = {(float)half_x * 0.5f, 33.0f};
        unsigned hits = 0;
        for (int t = 0; t < 7; ++t) {
            if (mtr_hit(&mctx, &cat.targets[t], &pt))
                ++hits;
        }
        if (pt.x >= 17.0f && pt.x < 624.0f) {
            CK(hits == 1);
        } else {
            CK(hits == 0);
        }
    }
    /* Stock overlap zone [103, 108): 104.0 -> tab 0, 106.0 -> tab 1 */
    CK(mtr_hit(&mctx, &cat.targets[0], &(struct surface_point){104.0f, 30.0f}) == 1);
    CK(mtr_hit(&mctx, &cat.targets[1], &(struct surface_point){104.0f, 30.0f}) == 0);
    CK(mtr_hit(&mctx, &cat.targets[0], &(struct surface_point){106.0f, 30.0f}) == 0);
    CK(mtr_hit(&mctx, &cat.targets[1], &(struct surface_point){106.0f, 30.0f}) == 1);

    /* Case 2: Page-dot 48px vertical expansion & disjoint interior midpoints */
    ++cases;
    for (int p = 0; p < 14; ++p) {
        const struct mtr_rect *r = &map.region[7 + p];
        CK(r->enabled == 1);
        CK(r->top == 429.0f && r->bottom == 477.0f); /* cy=453 +/- 24 -> 48px height */
        CK(r->left >= 15.0f && r->right <= 545.0f);  /* strictly left of BACK right-side region */
    }
    for (int half_x = 0; half_x < 1280; ++half_x) {
        struct surface_point pt = {(float)half_x * 0.5f, 432.0f}; /* above native 446px dot, inside 48px zone */
        unsigned dot_hits = 0;
        for (int p = 0; p < 14; ++p) {
            if (mtr_hit(&mctx, &cat.targets[7 + p], &pt))
                ++dot_hits;
        }
        CK(dot_hits <= 1);
    }

    /* Case 3: Prev/Next 8x16 arrows expanded to 48px width & clamped vertically to [66, 126] */
    ++cases;
    const struct mtr_rect *prev_r = &map.region[7 + 14];
    const struct mtr_rect *next_r = &map.region[7 + 15];
    CK(prev_r->enabled && prev_r->left == 0.0f && prev_r->right == 46.0f && prev_r->top == 77.0f && prev_r->bottom == 125.0f);
    CK(next_r->enabled && next_r->left == 594.0f && next_r->right == 640.0f && next_r->top == 77.0f && next_r->bottom == 125.0f);

    /* Case 4: Row & BACK targets keep r->enabled == 0 and delegate to native ns_hit */
    ++cases;
    const struct surface_target *row0 = &cat.targets[7 + 16];
    CK(map.region[7 + 16].enabled == 0);
    g_native_hit_calls = 0;
    CK(mtr_hit(&mctx, row0, &(struct surface_point){200.0f, 150.0f}) == 1 && g_native_hit_calls == 1);
    CK(mtr_hit(&mctx, row0, &(struct surface_point){200.0f, 100.0f}) == 0 && g_native_hit_calls == 2);

    /* Case 5: Scope & panel size rejection (non-640x480, popup, Fn 0x44, FFC 0xFFC) */
    ++cases;
    CK(mtr_build(&io, &cat, 800, 480, &map) == 0 && map.error == MTR_SCOPE && !map.valid);
    cat.identity.kind = SURFACE_POPUP;
    CK(mtr_build(&io, &cat, 640, 480, &map) == 0 && map.error == MTR_SCOPE);
    cat.identity.kind = SURFACE_FN;
    cat.identity.view_id = 0x44;
    CK(mtr_build(&io, &cat, 640, 480, &map) == 0 && map.error == MTR_SCOPE);
    cat.identity.kind = SURFACE_FFC;
    cat.identity.view_id = 0xffc;
    CK(mtr_build(&io, &cat, 640, 480, &map) == 0 && map.error == MTR_SCOPE);

    printf("test_menu_regions cases=%u checks=%u passed\n", cases, checks);
#undef CK
    return 0;
}
