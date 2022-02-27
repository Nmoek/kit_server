#ifndef _KIT_ENDIAN_H_
#define _KIT_ENDIAN_H_

#include <byteswap.h>
#include <stdint.h>
#include <algorithm>



#define KIT_SMALL_ENDIAN 1  //小端
#define KIT_BIG_ENDIAN 2    //大端

namespace kit_server
{

/**
 * @brief 如果传入类型T为 无符号64位 则使用类型T作为返回值
 * @tparam T 传入类型 uint64_t
 * @param value 具体数值
 * @return 返回 uint64_t 交换后的数据
 */
template<class T>
typename std::enable_if<sizeof(T) == sizeof(uint64_t), T>::type
byteswap(T value)
{
    return (T)bswap_64((uint64_t)value);
}


/**
 * @brief 如果传入类型T为 无符号32位 则使用类型T作为返回值
 * @tparam T 传入类型 uint32_t
 * @param value 具体数值
 * @return 返回 uint32_t 交换后的数据
 */
template<class T>
typename std::enable_if<sizeof(T) == sizeof(uint32_t), T>::type
byteswap(T value)
{
    return (T)bswap_32((uint32_t)value);
}

/**
 * @brief 如果传入类型T为 无符号16位 则使用类型T作为返回值
 * @tparam T 传入类型 uint16_t
 * @param value 具体数值
 * @return 返回 uint16_t 交换后的数据
 */
template<class T>
typename std::enable_if<sizeof(T) == sizeof(uint16_t), T>::type
byteswap(T value)
{
    return (T)bswap_16((uint16_t)value);
}

//判断物理机大小端
#if BYTE_ORDER == BIG_ENDIAN
    #define KIT_BYTE_ORDER KIT_BIG_ENDIAN
#else 
    #define KIT_BYTE_ORDER KIT_SMALL_ENDIAN
#endif

#if KIT_BYTE_ORDER == KIT_BIG_ENDIAN
/**
 * @brief 得到大端 大端机器 什么都不用操作
 */
template<class T>
T byteswapOnSmallEndian(T t)
{
    return t;
}

/**
 * @brief 得到小端 大端机器 大端----->小端
 */
template<class T>
T byteswapOnBigEndian(T t)
{
    return byteswap(t);
}
#else 
/**
 * @brief 得到大端 小端机器 小端------->大端
 */
template<class T>
T byteswapOnSmallEndian(T t)
{
    return byteswap(t);
}

/**
 * @brief 得到小端 小端机器 什么都不用做
 */
template<class T>
T byteswapOnBigEndian(T t)
{
    return t;
}
#endif


}




#endif 