#include "bytearray.h"
#include "endian.h"
#include "Log.h"

#include <string.h>
#include <iostream>
#include <fstream>
#include <errno.h>
#include <sstream>
#include <iomanip>
#include <sys/types.h>
#include <sys/socket.h>


namespace kit_server
{

static Logger::ptr g_logger = KIT_LOG_NAME("system");

ByteArray::Node::Node(size_t size)
    :ptr(new char[size])
    ,size(size)
    ,next(nullptr)
{

}

ByteArray::Node::Node()
    :ptr(nullptr)
    ,size(0)
    ,next(nullptr)
{

}

ByteArray::Node::~Node()
{
    if(ptr)
        delete[] ptr;
    
}


ByteArray::ByteArray(size_t base_size)
    :m_baseSize(base_size)
    ,m_position(0)
    ,m_capacity(base_size)
    ,m_size(0)
    ,m_endian(KIT_BIG_ENDIAN)   //网络字节序是大端
    ,m_root(new Node(base_size))
    ,m_cur(m_root)
{

}

ByteArray::~ByteArray()
{
    //释放内存链表
    Node* temp = m_root;
    while(temp)
    {
        m_cur = temp;
        temp = temp->next;
        delete m_cur;
    }

}


//清除当前内存结点链表
void ByteArray::clear()
{
    m_position = m_size = 0;
    //只留下一个结点 其他结点全部清理
    m_capacity = m_baseSize;    
    Node* temp = m_root->next;
    while(temp)
    {
        m_cur = temp;
        temp = temp->next;
        delete m_cur;
    }
    m_cur = m_root;
    m_root->next = nullptr;
}

//重点 写入内存池
void ByteArray::write(const void * buf, size_t size)
{
    if(size == 0)
        return;

    //扩展容量size
    addCapacity(size);
    //内存指针现在在内存块结点哪一个字节位置上
    size_t npos = m_position % m_baseSize;
    //当前结点的剩余容量
    size_t ncap = m_cur->size - npos;
    //已经写入内存的数据量
    size_t bpos = 0;

    while(size > 0)
    {
        //内存结点当前剩余容量能放下size的数据
        if(ncap >= size)
        {
            memcpy(m_cur->ptr + npos, (const char*)buf + bpos, size);
            //如果当前结点恰好被写完 也要把内存指针移到下一个内存块上
            if(m_cur->size == (npos + size))
                m_cur = m_cur->next;

            m_position += size;
            bpos += size;
            size = 0;

        }
        else    //否则就是不够放 先把当前剩余空间写完 在下一个新结点继续写入
        {
            memcpy(m_cur->ptr + npos, (const char *)buf + bpos, ncap);
            m_position += ncap;
            bpos += ncap;
            size -= ncap;

            //去遍历下一个内存块
            m_cur = m_cur->next;
            ncap = m_cur->size;
            npos = 0;

        }
        
    }

    //如果内存指针超过了当前表示的已经使用的空间大小 更新一下
    if(m_position > m_size)
        m_size = m_position;
    
}


//重点 读取内存
void ByteArray::read(void *buf, size_t size)
{
    //读取的长度超出可读范围要抛异常
    if(size > getReadSize())
        throw std::out_of_range("memory not enough!!");
    
    //内存指针现在在内存块结点哪一个字节位置上
    size_t npos = m_position % m_baseSize;
    //当前结点剩余容量
    size_t ncap = m_cur->size - npos;
    //当前已经读取的数据量
    size_t bpos = 0;
    while(size > 0)
    {
        if(ncap >= size)
        {

            memcpy((char*)buf + bpos, m_cur->ptr + npos, size);


            //如果当前结点被读完
            if(m_cur->size == npos + size)
                m_cur = m_cur->next;

            m_position += size;
            bpos += size;
            size = 0;            

        }
        else    //当前结点剩余容量 < 要读取的长度
        {
            memcpy((char*)buf + bpos, m_cur->ptr + npos, ncap);
            m_position += ncap;
            bpos += ncap;
            size -= ncap;

            m_cur = m_cur->next;
            ncap = m_cur->size;
            npos = 0;
        }
        
    }


}

void ByteArray::read(void *buf, size_t size, size_t position) const
{
    if(size > getReadSize())
        throw std::out_of_range("memory pool not enough!!");
    
    size_t npos = position % m_baseSize;
    size_t ncap = m_cur->size - npos;
    size_t bpos = 0;
    Node* cur = m_cur;
    while(size > 0)
    {
        if(ncap >= size)
        {
            memcpy((char*)buf + bpos, cur->ptr + npos, size);
            //如果当前结点被读完
            if(cur->size == npos + size)
                cur = cur->next;

            position += size;
            bpos += size;
            size = 0;            

        }
        else
        {
            memcpy((char*)buf + bpos, cur->ptr + npos, ncap);
            position += ncap;
            bpos += ncap;
            size -= ncap;

            cur = cur->next;
            ncap = m_cur->size;
            npos = 0;

        }
        
    }

}

//设定内存位置
void ByteArray::setPosition(size_t val)
{
    //设置内存指针位置超出了当前总容量大小 抛出异常
    if(val > m_capacity)
        throw std::out_of_range("set position out of range");
    
    m_position = val;
    //检查内存指针位置 > 使用内存空间大小要进行更新 
    m_size = m_position > m_size ? m_position : m_size;
    m_cur = m_root;

    /*移动当前可用结点指针m_cur*/
    //只要设定值val比单个结点容量大小大 认为没达到预期设定位置
    while(val >= m_cur->size)
    {
        val -= m_cur->size;
        m_cur = m_cur->next;
    }


}


//当前内存池数据写入到文件
bool ByteArray::writeToFile(const std::string& name) const
{
    std::ofstream ofs;
    //先清除内容再打开 以二进制方式写入
    ofs.open(name, std::ios::trunc | std::ios::binary);
    if(!ofs)
    {
        KIT_LOG_ERROR(g_logger) << "ByteArray::writeToFile open error, errno=" << errno << ", is:" 
            << strerror(errno);
        
        return false;
    }

    //获取剩余可读取的数据量
    int64_t remain_size = getReadSize();
    //拿到当前内存指针位置 作为一个副本
    int64_t pos = m_position;
    Node * cur = m_cur;
    while(remain_size > 0)
    {
        //计算得到当前内存指针所处内存块的偏移量位置
        int diff = pos % m_baseSize;

        //计算能写入的长度 = min(剩余可读数据量, 单个结点容量) - 内存指针偏移量
        int64_t len = (remain_size > (int64_t)m_baseSize ? m_baseSize : remain_size) - diff;

        ofs.write(cur->ptr + diff, len);
        cur = cur->next;
        pos += len;
        remain_size -= len;
    }

    return true;
}

//从文件把数据读入到内存池
bool ByteArray::readFromFile(const std::string& name)
{
    std::ifstream ifs;
    //以二进制打开
    ifs.open(name, std::ios::binary);
    if(!ifs)
    {
        KIT_LOG_ERROR(g_logger) << "ByteArray::readFromFile open error, errno=" << errno << ", is:" 
            << strerror(errno);
        
        return false;
    }

    //用智能指针定义一个char数组 指定析构函数管理temp空间
    std::shared_ptr<char> buf(new char[m_baseSize], [](char *ptr){ 
        delete[] ptr;
    });

    while(!ifs.eof())   //没有到文件末尾就一直读入
    {
        //从文件读取到buf
        ifs.read(buf.get(), m_baseSize);
        //从buf写回到内存池  gcount()是真正的从文件读取到的长度
        write(buf.get(), ifs.gcount());
        
    }

    return true;

}

void ByteArray::addCapacity(size_t size)
{
    if(size == 0)
        return;
    
    //获取剩余的容量
    size_t remain_cap = getRemainCapacity();
    if(remain_cap >= size)
        return;
    
    
    //减去原有的容量
    size = size - remain_cap;
    //计算需额外添加结点的数量 向上取整
    size_t count = ceil(1.0 * size / m_baseSize);

    //找到尾部内存块结点的位置
    Node *temp = m_root;
    while(temp->next)
    {
        temp = temp->next;
    }

    //从尾部开始尾插法添加新结点
    Node *first = nullptr;
    for(size_t i = 0;i < count;++i)
    {
        temp->next = new Node(m_baseSize);
        //将新加入的第一个结点记录一下便于连接
        if(first == nullptr)
        {
            first = temp->next;
        }

        temp = temp->next;
        m_capacity += m_baseSize;
    }

    //如果原来的容量恰好用完 要把m_cur置到第一个新结点上
    if(remain_cap == 0)
        m_cur = first;

}



//是否是小端字节序
bool ByteArray::isSmallEndian() const
{
    return m_endian == KIT_SMALL_ENDIAN;
}

//设置为小端字节序
void ByteArray::setSmallEndian(bool val)
{
    if(val)
        m_endian = KIT_SMALL_ENDIAN;
    else
        m_endian = KIT_BIG_ENDIAN;
}

//输出为可显示文本
std::string ByteArray::toString() const
{
    std::string s;
    s.resize(getReadSize());
    if(!s.size())
        return s;
    
    //不影响内存指针位置 仅仅是显示
    read(&s[0], s.size(), m_position);

    return s;
}

//输出为十六进制文本
std::string ByteArray::toHexString() const 
{
    std::string s = toString();
    std::stringstream ss;

    for(size_t i = 0;i < s.size();++i)
    {
        //一行显示32个字符
        if(i > 0 && i % 32 == 0)
            ss << std::endl;
        
        //一次显示一个字节 2个字符，不足的要用0补齐
        ss << std::setw(2) << std::setfill('0') << std::hex
            << (int)(uint8_t)s[i] << " ";
    }
    
    return ss.str();

}

uint64_t ByteArray::getReadBuf(std::vector<struct iovec>& buffers, uint64_t len) const
{
    len = len > getReadSize() ? getReadSize() : len;
    if(len == 0)
        return 0;
    
    uint64_t size = len;

    size_t npos = m_position % m_baseSize;
    size_t ncap = m_cur->size - npos;

    Node* cur = m_cur;
    while(len > 0)
    {
        struct iovec iov;
        //当前结点剩余容量大小 > 读取的长度
        if(ncap > len)
        {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = len;
            len = 0;
        }
        else // 当前结点剩余容量大小 <= 读取的长度
        {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = ncap;
            len -= ncap;

            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }

        buffers.push_back(iov);
    }

    return size;
}

uint64_t ByteArray::getReadBuf(std::vector<struct iovec>& buffers, uint64_t len, uint64_t position) const
{
    len = len > getReadSize() ? getReadSize() : len;
    if(len == 0)
        return 0;
    
    uint64_t size = len;

    size_t npos = position % m_baseSize;

    //找到指定位置所在的内存节点
    size_t count = position / m_baseSize;
    Node* cur = m_root;
    while(count > 0)
    {
        cur = cur->next;
        --count;
    }

    size_t ncap = cur->size - npos;


    while(len > 0)
    {
        struct iovec iov;
        if(ncap > len)
        {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = len;
            len = 0;
        }
        else
        {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = ncap;
            len -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }

        buffers.push_back(iov);
    }

    return size;
}


uint64_t ByteArray::getWriteBuf(std::vector<struct iovec>& buffers, uint64_t len)
{
    if(len == 0)
        return 0;
    
    //写入前先扩展容量
    addCapacity(len);
    size_t size = len;

    size_t npos = m_position % m_baseSize;
    size_t ncap = m_cur->size - npos;
    struct iovec iov;
    Node *cur = m_cur;
    while(len > 0)
    {
        if(ncap >= len)
        {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = len;
            len = 0;
        }
        else
        {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = ncap;

            len -= ncap;
            cur = cur->next;
            npos = 0;
            ncap = cur->size;
        }

        buffers.push_back(iov);
    }

    return size;
}


/***************************************write********************************************/
void ByteArray::writeFint8(int8_t value)
{
    write(&value, sizeof(value));
}

void ByteArray::writeFuint8(uint8_t value)
{
    write(&value, sizeof(value));
}

void ByteArray::writeFint16(int16_t value)
{
    //如果当前字节序和本机字节序不符 需要swap
    if(m_endian != KIT_BYTE_ORDER)
        value = byteswap(value);

    write(&value, sizeof(value));
}

void ByteArray::writeFuint16(uint16_t value)
{
    if(m_endian != KIT_BYTE_ORDER)
        value = byteswap(value);

    write(&value, sizeof(value));
}


void ByteArray::writeFint32(int32_t value)
{
    if(m_endian != KIT_BYTE_ORDER)
        value = byteswap(value);

    write(&value, sizeof(value));
}


void ByteArray::writeFuint32(uint32_t value)
{
    if(m_endian != KIT_BYTE_ORDER)
        value = byteswap(value);

    write(&value, sizeof(value));
}


void ByteArray::writeFint64(int64_t value)
{
    if(m_endian != KIT_BYTE_ORDER)
        value = byteswap(value);

    write(&value, sizeof(value));
}


void ByteArray::writeFuint64(uint64_t value)
{
    if(m_endian != KIT_BYTE_ORDER)
        value = byteswap(value);

    write(&value, sizeof(value));
}


/**
 * @brief  int32_t---------->uint32_t
 * @param[in] v  传入的int32_t数据
 * @return uint32_t 
 */
static uint32_t EncodeZigzag32(const int32_t &v)
{
    //不转换的话 -1压缩一定要消耗5个字节
    if(v < 0)
    {
        return ((uint32_t)(-v)) * 2 - 1;
    }

    return v * 2;
}

/**
 * @brief  int64_t---------->uint64_t
 * @param[in] v  传入的int64_t数据
 * @return uint64_t 
 */
static uint64_t EncodeZigzag64(const int64_t &v)
{
    //不转换的话 -1压缩一定要消耗10个字节
    if(v < 0)
    {
        return ((uint64_t)(-v)) * 2 - 1;
    }

    return v * 2;
}

/**
 * @brief uint32_t---------->int32_t
 * @param[in] v  传入的uint32_t数据
 * @return int32_t 
 */
static int32_t DecodeZigzag32(const uint32_t &v)
{
    return (v >> 1) ^ -(v & 1);
}

/**
 * @brief int64_t---------->uint64_t
 * @param[in] v  传入的int64_t数据
 * @return int64_t 
 */
static int64_t DecodeZigzag64(const uint64_t &v)
{
    return (v >> 1) ^ -(v & 1);
}

void ByteArray::writeInt32(int32_t value)
{
    writeUint32(EncodeZigzag32(value));
}

void ByteArray::writeUint32(uint32_t value)
{
    //uint32_t 压缩后1~5字节的大小
    uint8_t temp[5];
    uint8_t i = 0;
    //varint编码 msp等于1 就认为数据还没读完
    while(value >= 0x80)
    {   
        //取低7位 + msp==1 组成新的编码数据
        temp[i++] = (value & 0x7F) | 0x80;
        value >>= 7;
    }

    temp[i++] = value;

    write(temp, i);
}

void ByteArray::writeInt64(int64_t value)
{
    writeUint64(EncodeZigzag64(value));
}

void ByteArray::writeUint64(uint64_t value)
{
    uint8_t temp[10];
    uint8_t i = 0;
    while(value >= 0x80)
    {
        temp[i++] = (value & 0x7F) | 0x80;
        value >>= 7;
    }
    
    temp[i++] = value;
    write(temp, i);

}

void ByteArray::writeFloat(float value)
{
    uint32_t v;
    memcpy(&v, &value, sizeof(value));
    writeFuint32(v);
}

void ByteArray::writeDouble(double value)
{
    uint64_t v;
    memcpy(&v, &value, sizeof(value));
    writeFuint64(v);
}

//指定写入string 的长度
void ByteArray::writeStringF16(const std::string& value)
{
    //先写入一个长度 在写入具体数据
    writeFuint16(value.size());
    write(value.c_str(), value.size());
}

void ByteArray::writeStringF32(const std::string& value)
{
    //先写入一个长度 在写入具体数据
    writeFuint32(value.size());
    write(value.c_str(), value.size());
}

void ByteArray::writeStringF64(const std::string& value)
{
    //先写入一个长度 在写入具体数据
    writeFuint64(value.size());
    write(value.c_str(), value.size());
}

//压缩
void ByteArray::writeStringVint(const std::string& value)
{
    writeUint64(value.size());
    write(value.c_str(), value.size());
}

void ByteArray::writeStringWithoutLen(const std::string& value)
{
    write(value.c_str(), value.size());
}


/***************************************read********************************************/
int8_t ByteArray::readFint8()
{
    int8_t v;
    read(&v, sizeof(v));
    return v;
}


uint8_t ByteArray::readFuint8()
{
    uint8_t v;
    read(&v, sizeof(v));
    return v;
}

#define READ_XX(type)\
    type v;\
    read(&v, sizeof(v));\
    if(m_endian !=  KIT_BYTE_ORDER)\
        v = byteswap(v);\
    return v;


int16_t ByteArray::readFint16()
{
    READ_XX(int16_t);
}

uint16_t ByteArray::readFuint16()
{
    READ_XX(uint16_t);
}

int32_t ByteArray:: readFint32()
{
    READ_XX(int32_t);
}

uint32_t ByteArray::readFuint32()
{
    READ_XX(uint32_t);
}

int64_t ByteArray::readFint64()
{
    READ_XX(int64_t);
}

uint64_t ByteArray::readFuint64()
{
    READ_XX(uint64_t);
}


int32_t ByteArray::readInt32()
{
    return DecodeZigzag32(readUint32());
}

uint32_t ByteArray::readUint32()
{
    //最终得到一个uint32_t型数据
    uint32_t result = 0;
    //读取次数 = 32 / 7 + 1 组
    for(int i = 0;i < 32;i += 7)
    {
        //一次读取8位
        uint8_t b = readFuint8();
        //msp==0 说明这是该数据的最后一个字节
        if(b < 0x80)
        {
            //最后一个字节直接或运算 不用去msp位
            result |= ((uint32_t)b) << i;
            break;
        }
        else    //msp==1 说明后面还有字节没有取 要去掉头部的msp位
            result |= ((uint32_t)(b & 0x7F)) << i;
    }

    return result;
}

int64_t ByteArray::readInt64()
{
    return DecodeZigzag64(readUint64());
}

uint64_t ByteArray::readUint64()
{
    //最终得到一个uint32_t型数据
    uint64_t result = 0;
    //读取次数 = 64 / 7 + 1 组
    for(int i = 0;i < 64;i += 7)
    {
        uint8_t b = readFuint8();
        //msp==0 说明这是该数据的最后一个字节
        if(b < 0x80)
        {
            //最后一个字节直接或运算 不用去msp位
            result |= ((uint64_t)b) << i;
            break;
        }
        else    //msp==1 说明后面还有字节没有取 要去掉头部的msp位
            result |= ((uint64_t)(b & 0x7F)) << i;
    }

    return result;
}

float ByteArray::readFloat()
{
    uint32_t v = readFuint32();
    float value;
    memcpy(&value, &v, sizeof(v));

    return value;
    
}

double ByteArray::readDouble()
{
    uint64_t v = readFuint64();
    double value;
    memcpy(&value, &v, sizeof(v));

    return value;
}

std::string ByteArray::readStringF16()
{
    uint16_t len = readFuint16();
    std::string buf;
    buf.resize(len);
    read(&buf[0], len);
    return buf;
}

std::string ByteArray::readStringF32()
{
    uint32_t len = readFuint32();
    std::string buf;
    buf.resize(len);
    read(&buf[0], len);
    return buf;
}

std::string ByteArray::readStringF64()
{
    uint64_t len = readFuint64();
    std::string buf;
    buf.resize(len);
    read(&buf[0], len);
    return buf;
}

std::string ByteArray::readStringVint()
{
    uint64_t len = readUint64();
    std::string buf;
    buf.resize(len);
    read(&buf[0], len);
    return buf;
}

}