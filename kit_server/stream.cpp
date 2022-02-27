#include "stream.h"



namespace kit_server
{

//读够指定字节才会返回
int Stream::readFixSize(void *buffer, size_t len)
{
    //剩余应读字节数
    size_t left = len;
    //缓冲区偏移量
    size_t offset = 0;

    while(left > 0)
    {
        size_t ret = read((char*)buffer + offset, left);
        if(ret <= 0)
        {
            return ret;
        }

        offset += ret;
        left -= ret;
    }

    //返回出已经读完的字节数
    return len;
}

//从当前ByteArray的position 向内存池写入 
int Stream::readFixSize(ByteArray::ptr ba, size_t len)
{
    
    //剩余应读字节数
    size_t left = len;

    while(left > 0)
    {
        //复用read()函数
        size_t ret = read(ba, left);
        if(ret <= 0)
        {
            return ret;
        }

        left -= ret;
    }

    //返回出已经读完的字节数
    return len;
}


int Stream::writeFixSize(const void* buffer, size_t len)
{
    //剩余应写入字节数
    size_t left = len;
    //缓冲区偏移量
    size_t offset = 0;

    while(left > 0)
    {
        size_t ret = write((char*)buffer + offset, left);
        if(ret <= 0)
        {
            return ret;
        }

        offset += ret;
        left -= ret;
    }

    //返回出已经写完的字节数
    return len;
}

//从当前ByteArray的position 向外输出 
int Stream::writeFixSize(ByteArray::ptr ba, size_t len)
{
    //剩余应写入字节数
    size_t left = len;

    while(left > 0)
    {
        size_t ret = write(ba, left);
        if(ret <= 0)    //出错就要返回
        {
            return ret;
        }

        left -= ret;
    }

    //返回出已经写完的字节数
    return len;
}



}