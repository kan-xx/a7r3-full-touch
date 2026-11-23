#ifndef A7_TOUCH_AUTOLOAD_CONTRACT_H
#define A7_TOUCH_AUTOLOAD_CONTRACT_H
#include <stdint.h>
enum as_code {AS_ACCEPTED=0,AS_SAFE_BUSY=0x7001,AS_ALREADY_ACCEPTED=0x7002,AS_ACTIVATION_PENDING=0x7005,AS_BAD_ARGUMENT=0x7010,AS_BAD_BORROW=0x7011,AS_FRESH_FAILED=0x7012,AS_PIN_FAILED=0x7013,AS_ALREADY_STARTED=0x7021,AS_INVARIANT=0x70fd,AS_CLOSE_UNCERTAIN=0x70fe,AS_EXCEPTION=0x70ff};
struct as_report {uint32_t abi,bytes,stage,result,ui_tid,adopted,independent_ref,close_attempted,close_uncertain,retryable,fresh,prepare_result,exception_caught,borrow_valid,existing_accepted,reserved;};
#endif
