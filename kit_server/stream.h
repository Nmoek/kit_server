#ifndef _KIT_STREAM_H_
#define _KIT_STREAM_H_


#include <memory>
#include "bytearray.h"


namespace kit_server
{

class Stream
{
public:
    typedef std::shared_ptr<Stream> ptr;

    /**
     * @brief Stream类析构函数
     */
    virtual ~Stream() {}

    /**
     * @brief 读取数据
     * @param[out] buffer 存放数据的缓冲区
     * @param[in] len 期望读取数据长度
     * @return
     *      @retval >0 实际读取的字节数
     *      @retval 0 对端关闭
     *      @retval <0 错误
     */
    virtual int read(void *buffer, size_t len) = 0;

    /**
     * @brief 接收到数据后写入ByteArray中
     * @param[in] ba 接收数据的ByteArray
     * @param[in] len 接收数据长度
     * @return
     *      @retval >0 实际读取的字节数
     *      @retval 0 对端关闭
     *      @retval <0 错误
     */
    virtual int read(ByteArray::ptr ba, size_t len) = 0;

    /**
     * @brief 读取数据，读够指定字节才会返回
     * @param[out] buffer 存放数据的缓冲区
     * @param[in] len 期望读取数据长度
     * @return
     *      @retval >0 实际读取的字节数
     *      @retval 0 对端关闭
     *      @retval <0 错误
     */
    virtual int readFixSize(void *buffer, size_t len);


    /**
     * @brief 接收到数据后写入ByteArray中，读够指定字节才会返回
     * @param[in] ba 接收数据的ByteArray
     * @param[in] len 接收数据长度
     * @return
     *      @retval >0 实际读取的字节数
     *      @retval 0 对端关闭
     *      @retval <0 错误
     */
    virtual int readFixSize(ByteArray::ptr ba, size_t len);

    /**
     * @brief 写入数据
     * @param[in] buffer 存放数据的缓冲区
     * @param[in] len 期望写入数据长度
     * @return
     *      @retval >0 实际写入的字节数
     *      @retval <0 错误
     */
    virtual int write(const void* buffer, size_t len) = 0;

    /**
     * @brief 拿出ByteArray中数据写入
     * @param[out] ba 拿出数据的ByteArray
     * @param[in] len 期望写入数据长度
     * @return
     *      @retval >0 实际读取的字节数
     *      @retval <0 错误
     */
    virtual int write(ByteArray::ptr ba, size_t len) = 0;
    
    /**
     * @brief 写入数据，写入指定字节才会返回
     * @param[in] buffer 存放数据的缓冲区
     * @param[in] len 期望写入数据长度
     * @return
     *      @retval >0 实际读取的字节数
     *      @retval <0 错误
     */
    virtual int writeFixSize(const void* buffer, size_t len);


    /**
     * @brief 拿出ByteArray中数据写入，写入指定字节才会返回
     * @param[out] ba 拿出数据的ByteArray
     * @param[in] len 期望写入数据长度
     * @return
     *      @retval >0 实际读取的字节数
     *      @retval <0 错误
     */
    virtual int writeFixSize(ByteArray::ptr ba, size_t len);

    /**
     * @brief 关闭
     */
    virtual void close() = 0;
};


}

#endif 