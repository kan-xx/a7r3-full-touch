/* FW3.10 read ABI. No camera mutation or event injection in this file. */
#ifndef A7R3_NATIVE_RAW_TP_ABI_H
#define A7R3_NATIVE_RAW_TP_ABI_H
#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint32_t source_code;        /* +00; normal TP source 0x11013001 */
    int16_t y;                   /* +04; logical panel Y, not raw ADC */
    int16_t x;                   /* +06; logical panel X */
    int32_t state;               /* +08 */
    uint32_t opaque_0c;          /* +0c; not assigned semantics here */
    uint16_t flags;              /* +10; low 4 bits contact ID; bit4 edge filtering */
    uint16_t timestamp_ticks;    /* +12; low15 bits, native units=16ms */
    uint32_t opaque_14;          /* +14 */
} A7R3InfraInputEvent;

/* ParamBase/type6 copy object; all native addresses are 32-bit. */
typedef struct {
    uint8_t opaque_base[0x10];
    uint32_t data_address;       /* +10; getter libObj+31e1d1 */
    uint32_t data_bytes;         /* +14; require exactly24 for this event */
} A7R3ParamBlob32;

enum {
    A7R3_RAW_TP_EVENT_ID = 0x11013008u,
    A7R3_RAW_TP_SOURCE = 0x11013001u,
    A7R3_RAW_TP_BLOB_PREVIOUS = 0x1b,
    A7R3_RAW_TP_BLOB_CURRENT = 0x17,
    A7R3_PARAM_TYPE_BLOB = 6
};

typedef uint32_t (*A7R3EventGetIdABI)(const void *event);
typedef void *(*A7R3EventGetParameterABI)(void *event, uint32_t id,
                                         uint32_t required_type);
typedef const void *(*A7R3BlobGetDataABI)(const void *param_blob);
typedef void (*A7R3ViewSystemEventCallbackABI)(void *event);

_Static_assert(sizeof(A7R3InfraInputEvent)==24,"InfraInputEvent size");
_Static_assert(offsetof(A7R3InfraInputEvent,x)==6,"TP axis layout");
_Static_assert(offsetof(A7R3InfraInputEvent,state)==8,"TP state offset");
_Static_assert(offsetof(A7R3InfraInputEvent,flags)==0x10,"TP flags offset");
_Static_assert(sizeof(A7R3ParamBlob32)==24,"type6 copy object size");

static inline uint32_t a7r3_tp_contact(const A7R3InfraInputEvent *p) {
    return p->flags & 15u;
}
static inline int a7r3_tp_native_down_or_move(int32_t s) {
    return s==1 || (uint32_t)s==0x80000001u;
}
static inline int a7r3_tp_native_up_or_cancel(int32_t s) {
    return s==0 || s==2 || s==3 || (uint32_t)s==0x80000002u;
}
/* Conservative tap activation: only the normal, clean release is a tap. */
static inline int a7r3_tp_clean_release(int32_t s) { return s==0; }
static inline uint32_t a7r3_tp_elapsed_ms(uint16_t start,uint16_t end) {
    return ((uint32_t)(end-start) & 0x7fffu) << 4;
}
#endif
