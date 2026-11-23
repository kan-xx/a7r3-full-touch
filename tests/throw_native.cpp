extern "C" void test_throw_native(void){throw 123;}
extern "C" void pb_test_input_hook(void*);
extern "C" int test_try_hook(void*event){try{pb_test_input_hook(event);return 0;}catch(...){return 1;}}

#include <stdint.h>
struct ts;extern "C" void rt_pb_fence_impl(void*,ts*,void*,uint32_t);extern "C" void rt_pb_fence_failed(void*,ts*,void*,uint32_t);
extern "C" int test_try_pb_fence(void*r,void*d,uint32_t lr){try{rt_pb_fence_impl(r,nullptr,d,lr);return 0;}catch(...){rt_pb_fence_failed(r,nullptr,d,lr);return 1;}}
