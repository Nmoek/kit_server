#include "../kit_server/macro.h"
#include "../kit_server/config.h"
#include "../kit_server/Log.h"
#include "../kit_server/single.h"
#include "../kit_server/thread.h"
#include "../kit_server/util.h"

#include <assert.h>
#include <execinfo.h>

using namespace kit_server;

void test_assert()
{
    KIT_LOG_INFO(KIT_LOG_ROOT()) << "\n" << BackTraceToString(10,  "    ");
    //KIT_ASSERT(false);
    // KIT_ASSERT2(0 == 1, 你真牛逼);
}

int main()
{
    test_assert();


    return 0;
}