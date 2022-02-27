#ifndef _KIT_BYTRARRAY_H_
#define _KIT_BYTRARRAY_H_

#include <stdint.h>
#include <memory>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <vector>
#include <math.h>

namespace kit_server
{

class ByteArray
{
public:
    typedef std::shared_ptr<ByteArray> ptr;

    /**
     * @brief 内存块结构体
     */
    struct Node
    {
        /**
         * @brief 内存块结构体构造函数
         * @param[in] size 传入内存块的申请空间大小 
         */
        Node(size_t size);
        Node();
        ~Node();

        /**
         * @brief 指向数据内存区域的指针
         */
        char *ptr;

        /**
         * @brief 内存块的字节大小
         */
        size_t size;

        /**
         * @brief 下一个内存结点块的指针
         */
        struct Node* next;

    };
    
    /**
     * @brief ByteArray类构造函数
     * @param base_size 传入的指定内存块的大小 默认为4KB
     */
    ByteArray(size_t base_size = 4 * 1024);

    /**
     * @brief ByteArray类析构函数
     */
    ~ByteArray();

    /**
     * @brief 清除当前内存结点中所有内容
     */
    void clear();

    /**
     * @brief 将数据写入内存
     * @param[in] buf 写入缓冲区指针 
     * @param[in] size 要写入的字节大小
     */
    void write(const void * buf, size_t size);

    /**
     * @brief 将数据从内存读出
     * @param[out] buf 读取缓冲区指针 
     * @param[in] size 预计要读取的字节大小
     */
    void read(void *buf, size_t size);

    /**
     * @brief 将数据从内存读出 但不影响操作指针位置
     * @param[out] buf 读取缓冲区指针 
     * @param[in] size 预计要读取的字节大小
     * @param[in] position 当前内存操作指针的位置
     */
    void read(void *buf, size_t size, size_t position) const;
    
    /**
     * @brief 获取当前操作内存指针的位置
     * @return size_t 返回位置坐标数值
     */
    size_t getPosition() const {return m_position;}

    /**
     * @brief 设置当前操作内存指针的位置
     * @param[in] val 传入准备设置的指针位置数值
     */
    void setPosition(size_t val); 

    /**
     * @brief 将数据从内存写入到文件中
     * @param[in] name 写入的文件路径
     * @return true 写入成功
     * @return false 写入失败
     */
    bool writeToFile(const std::string& name) const;

    /**
     * @brief 将数据从文件中读取到内存中
     * @param[in] name 读取的文件路径
     * @return true 读取成功
     * @return false 读取失败
     */
    bool readFromFile(const std::string& name);

    /**
     * @brief 获取内存单个结点存储容量大小
     * @return size_t 
     */
    size_t getBaseSize() const {return m_baseSize;};

    /**
     * @brief 获取当前内存中剩余可读数据的容量
     * @return size_t 
     */
    size_t getReadSize() const {return m_size - m_position;}

    /**
     * @brief 获取当前整个内存已经使用的空间大小
     * @return size_t 
     */
    size_t getSize() const {return m_size;}

    /**
     * @brief 获取内存块结点个数
     * @return size_t 
     */
    size_t getNodeSum() const {return ceil(1.0 * m_size / m_baseSize);}

    /**
     * @brief 获取当前已经开辟的总容量空间大小
     * @return size_t 
     */
    size_t getTotalCapacity() const  {return m_capacity;}


    /**
     * @brief 获取当前数据存储是否是小端模式
     * @return true 
     * @return false 
     */
    bool isSmallEndian() const;

    //设置为小端字节序
    /**
     * @brief 设置当前数据存储是否是小端模式
     * @param[in] val true或false 
     */
    void setSmallEndian(bool val);

    /**
     * @brief 将内存块的内容输出为可显示文本
     * @return std::string 
     */
    std::string toString() const;

    /**
     * @brief 将内存块的内容输出为十六进制文本
     * @return std::string 
     */
    std::string toHexString() const;

    /**
     * @brief 把内存中所有可读部分拿到用户缓冲区中去,从当前位置开始
     * @param[out] buffers 存储内存内容的缓冲区
     * @param[in] len 准备读取的长度
     * @return uint64_t 返回实际读取字节数 
     */
    uint64_t getReadBuf(std::vector<struct iovec>& buffers, uint64_t len) const;

    /**
     * @brief 把内存中所有可读部分拿到用户缓冲区中去,从指定位置开始
     * @param[out] buffers 存储内存内容的缓冲区
     * @param[in] len 准备读取的长度
     * @param[in] position 指定读取开始位置
     * @return uint64_t 返回实际读取字节数 
     */
    uint64_t getReadBuf(std::vector<struct iovec>& buffers, uint64_t len, uint64_t position) const;

    /**
     * @brief 把内存中所有可写部分拿到用户缓冲区中去,从指定位置开始
     * @param[out] buffers 存储内存内容的缓冲区
     * @param[in] len 准备读取的长度
     * @return uint64_t 返回实际读取字节数 
     */
    uint64_t getWriteBuf(std::vector<struct iovec>& buffers, uint64_t len);

public:

    /**write**/
    /*固定长度存储*/
    void writeFint8(int8_t value);
    void writeFuint8(uint8_t value);
    void writeFint16(int16_t value);
    void writeFuint16(uint16_t value);
    void writeFint32(int32_t value);
    void writeFuint32(uint32_t value);
    void writeFint64(int64_t value);
    void writeFuint64(uint64_t value);

    /*带varint压缩存储*/
    void writeInt32(int32_t value);
    void writeUint32(uint32_t value);
    void writeInt64(int64_t value);
    void writeUint64(uint64_t value);

    void writeFloat(float value);
    void writeDouble(double value);
    
    /**
     * @brief 写入 长度:int16_t + 字符串数据
     * @param value 传入的字符串
     */
    void writeStringF16(const std::string& value);

    /**
     * @brief 写入 长度:int32_t + 字符串数据
     * @param value 传入的字符串
     */
    void writeStringF32(const std::string& value);

    /**
     * @brief 写入 长度:int64_t + 字符串数据
     * @param value 传入的字符串
     */
    void writeStringF64(const std::string& value);

    /**
     * @brief 写入 长度:varint(变长) + 字符串数据
     * @param value 传入的字符串
     */
    void writeStringVint(const std::string& value);
    
    /**
     * @brief 写入 不带长度字符串数据
     * @param value 传入的字符串
     */
    void writeStringWithoutLen(const std::string& value);
    
    /**read**/
    /*固定长度读取*/
    int8_t      readFint8();
    uint8_t     readFuint8();
    int16_t     readFint16();
    uint16_t    readFuint16();
    int32_t     readFint32();
    uint32_t    readFuint32();
    int64_t     readFint64();
    uint64_t    readFuint64();

    /*读取varint压缩后的数据*/
    int32_t     readInt32();
    uint32_t    readUint32();
    int64_t     readInt64();
    uint64_t    readUint64();

    float       readFloat();
    double      readDouble();

    /**
     * @brief 读取 长度:int16_t + 字符串数据
     * @return std::string 
     */
    std::string     readStringF16();

    /**
     * @brief 读取 长度:int32_t + 字符串数据
     * @return std::string 
     */
    std::string     readStringF32();

    /**
     * @brief 读取 长度:int64_t + 字符串数据
     * @return std::string 
     */
    std::string     readStringF64();

    /**
     * @brief 读取 长度:varint(变长) + 字符串数据
     * @return std::string 
     */
    std::string     readStringVint();

private:
    /**
     * @brief 给内存扩容
     * @param[in] size 需要扩大的容量大小 
     */
    void addCapacity(size_t size);

    /**
     * @brief 获取能使用的剩余容量大小
     * @return size_t 
     */
    size_t getRemainCapacity() const {return m_capacity - m_position;}

private:
    //每一个内存块有多大
    size_t m_baseSize;
    //当前操作指针的位置
    size_t m_position;
    //总共容量大小
    size_t m_capacity;
    //已经使用的空间大小
    size_t m_size;
    //存储数据字节序(大端/小端)
    int8_t m_endian;
    //内存结点个数
    size_t m_nodeSum;
    //内存链表头结点
    Node* m_root;
    //内存链表尾结点
    Node* m_cur;

};


}



#endif 