#ifndef _KIT_MACRO_H_
#define _KIT_MACRO_H_

/*
*   自己定义一些后续方便调试的宏定义
*/

#include <string.h>
#include <assert.h>
#include "util.h"

//编译器优化
#if  defined __GNUC__ || defined __llvm__
#   define KIT_LICKLY(x)   __builtin_expect(!!(x), 1)
#   define KIT_UNLICKLY(x)   __builtin_expect(!!(x), 0)
#else
#   define KIT_LICKLY(x)    (x)
#   define KIT_UNLICKLY(x)  (x)
#endif


#define KIT_ASSERT(x) do{\
    if(KIT_UNLICKLY(!(x))){\
        KIT_LOG_ERROR(KIT_LOG_ROOT()) << "\nASSERTION: " #x   \
                                      <<  "\nbacktrace: \n" \
                                      << kit_server::BackTraceToString(100, "    ");\
        assert(x);\
    }\
}\
while(0);

//第二个参数可做一些补充说明
#define KIT_ASSERT2(x, w)do{\
    if(KIT_UNLICKLY(!(x))){\
        KIT_LOG_ERROR(KIT_LOG_ROOT()) << "\nASSERTION: " #x   \
                                      << "\n" << w \
                                      <<  "\nbacktrace: \n" \
                                      << kit_server::BackTraceToString(100, "    ");\
        assert(x);\
    }\
}\
while(0)

#endif