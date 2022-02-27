#include "../kit_server/address.h"
#include "../kit_server/Log.h"
#include "../kit_server/coroutine.h"
#include "../kit_server/config.h"
#include "../kit_server/thread.h"
#include "../kit_server/scheduler.h"
#include "../kit_server/iomanager.h"
#include "../kit_server/hook.h"
#include "../kit_server/socket.h"
#include "../kit_server/bytearray.h"
#include "../kit_server/macro.h"

#include <iostream>
#include <stdlib.h>
#include <stdint.h>

using namespace std;
using namespace kit_server;


static Logger::ptr g_logger = KIT_LOG_ROOT();


void test()
{
    #if 1
#define XX(type, len, write_func, read_func, base_len){\
    std::vector<type> mv;\
    std::cout << "写入:";\
    for(int i = 0;i < len;++i)\
    {\
        mv.push_back((type)(1.2 + rand()));\
        std::cout << mv[i] << " ";\
    }\
    std::cout << std::endl << std::endl;\
    ByteArray::ptr ba(new ByteArray(base_len));\
    for(size_t i = 0; i < mv.size();++i)\
    {\
        ba->write_func(mv[i]);\
    }\
    ba->setPosition(0);\
    std::cout << "读取:";\
    for(size_t i = 0; i < mv.size();++i)\
    {\
        type v = ba->read_func();\
        std::cout << v << " ";\
        KIT_ASSERT(v == mv[i]);\
    }\
    std::cout << std::endl;\
    KIT_ASSERT(ba->getReadSize() == 0);\
    KIT_LOG_INFO(g_logger) << #write_func  "/" #read_func "(" #type ")" << ",len=" << len\
        << ",base_len=" << base_len << ",capacity=" << ba->getTotalCapacity() << ",used size=" << ba->getSize() << ", node count=" << ba->getNodeSum();\
    std::cout << std::endl;\
}

    // /*固定大小*/
    // XX(int8_t, 100, writeFint8, readFint8, 10);
    // XX(uint8_t, 100, writeFuint8, readFuint8, 10);
    // XX(int16_t, 100, writeFint16, readFint16, 10);
    // XX(uint16_t, 100, writeFuint16, readFuint16, 10);
    // XX(int32_t, 100, writeFint32, readFint32, 10);
    // XX(uint32_t, 100, writeFuint32, readFuint32, 10);
    // XX(int64_t, 100, writeFint64, readFint64, 10);
    // XX(uint64_t, 100, writeFuint64, readFuint64, 10);

    // /*变长大小*/
    // XX(int32_t, 100, writeInt32, readInt32, 10);
    // XX(uint32_t, 100, writeUint32, readUint32, 10);
    // XX(int64_t, 100, writeInt64, readInt64, 10);
    // XX(uint64_t, 100, writeUint64, readUint64, 10);

    XX(float, 100, writeFloat, readFloat, 10);
    XX(double, 100, writeDouble, readDouble, 10);
#undef XX 

#else
    std::vector<float> mv;
    for(int i = 0;i < 100;++i)
    {
        mv.push_back(1.2 + rand());
    }
    ByteArray ba(1);

    for(auto& x : mv)
    {
        ba.writeFint8(x);
    }
   
    ba.setPosition(0);
    for(size_t i = 0; i < mv.size();++i)
    {
        int8_t v = ba.readFint8();
        KIT_LOG_INFO(g_logger) << "v=" << std::hex << v << ", mv[" << i << "]=" << mv[i]; 
    }
    KIT_ASSERT(ba.getReadSize() == 0);
#endif

}


static std::string rand_str(uint64_t len)
{
    std::string s;
    s.resize(len);

    for(int i = 0; i < (int)len;++i)
    {
        switch (rand() % 3)
        {
        case 0: s[i] = 'a'  + rand() % 26; break;
        case 1: s[i] = 'A'  + rand() % 26; break;
        case 2: s[i] = '0'  + rand() % 9; break;
        }
    }

    return s;
}


void test2()
{

#define XX(type_len, len, write_func, read_func, base_len){\
    std::vector<std::string> mv;\
    for(int i = 0; i < len;++i)\
    {\
        mv.push_back(rand_str(type_len));\
    }\
    ByteArray::ptr ba(new ByteArray(base_len));\
    for(auto &x : mv)\
        ba->write_func(x);\
    ba->setPosition(0);\
    for(int i = 0; i < len;++i)\
    {\
        std::string v = ba->read_func();\
        KIT_ASSERT(v == mv[i]);\
    }\
    KIT_LOG_INFO(g_logger) << #write_func  "/" #read_func << "("  << type_len * 8  << ")" << ",len=" << len\
        << ",base_len=" << base_len << ",capacity=" << ba->getTotalCapacity() << ",used size=" << ba->getSize() << ", node count=" << ba->getNodeSum();\
}

    XX(2, 100, writeStringF16, readStringF16, 10);
    XX(4, 100,writeStringF32, readStringF32, 10);
    XX(8, 100, writeStringF64, readStringF64, 10);
    uint64_t t = rand() % 1000;
    XX(t, 100, writeStringVint, readStringVint, 100);

#undef XX


}

void test3()
{
#define XX(type, len, write_func, read_func, base_len){\
    std::vector<type> mv;\
    for(int i = 0;i < len;++i)\
    {\
        mv.push_back(rand());\
    }\
    ByteArray::ptr ba(new ByteArray(base_len));\
    for(size_t i = 0; i < mv.size();++i)\
    {\
        ba->write_func(mv[i]);\
    }\
    ba->setPosition(0);\
    for(size_t i = 0; i < mv.size();++i)\
    {\
        type v = ba->read_func();\
        KIT_ASSERT(v == mv[i]);\
    }\
    KIT_ASSERT(ba->getReadSize() == 0);\
    KIT_LOG_INFO(g_logger) << #write_func  "/" #read_func "(" #type ")" << ",len=" << len\
        << ",base_len=" << base_len << ",capacity=" << ba->getTotalCapacity() << ",used size=" << ba->getSize() << ", node count=" << ba->getNodeSum();\
    ba->setPosition(0);\
    KIT_ASSERT(ba->writeToFile("./tests/temp/" #type "_" #write_func "_" #len ".dat"));\
    ByteArray::ptr ba2(new ByteArray(base_len * 2));\
    KIT_ASSERT(ba2->readFromFile("./tests/temp/" #type "_" #write_func "_" #len ".dat"));\
    ba2->setPosition(0);\
    KIT_ASSERT(ba->toString() == ba2->toString());\
    KIT_ASSERT(ba->getPosition() == 0);\
    KIT_ASSERT(ba2->getPosition() == 0);\
}

    /*固定大小*/
    XX(int8_t, 100, writeFint8, readFint8, 10);
    XX(uint8_t, 100, writeFuint8, readFuint8, 10);
    XX(int16_t, 100, writeFint16, readFint16, 10);
    XX(uint16_t, 100, writeFuint16, readFuint16, 10);
    XX(int32_t, 100, writeFint32, readFint32, 10);
    XX(uint32_t, 100, writeFuint32, readFuint32, 10);
    XX(int64_t, 100, writeFint64, readFint64, 10);
    XX(uint64_t, 100, writeFuint64, readFuint64, 10);

    /*变长大小*/
    XX(int32_t, 100, writeInt32, readInt32, 10);
    XX(uint32_t, 100, writeUint32, readUint32, 10);
    XX(int64_t, 100, writeInt64, readInt64, 10);
    XX(uint64_t, 100, writeUint64, readUint64, 10);

#undef XX

}

int main()
{
    KIT_LOG_INFO(g_logger) << "test begin";

    test3();   

    KIT_LOG_INFO(g_logger) << "test end";


    return 0;
}