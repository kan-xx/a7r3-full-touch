#include <stdint.h>
extern int main(int,char**);
extern "C" int __libc_start_main(int(*)(int,char**,char**),int,char**,void*,void*,void*,void*);
static int main_test(int ac,char**av,char**ep){(void)ac;(void)av;(void)ep;return main(ac,av);}
extern "C" __attribute__((used,noreturn)) void start_c(uint32_t*sp){__libc_start_main(main_test,sp[0],(char**)(sp+1),0,0,0,sp);__builtin_trap();}
extern "C" __attribute__((naked,noreturn)) void _start(void){__asm__ volatile("mov r0, sp\nbic sp, sp, #7\nb start_c");}
