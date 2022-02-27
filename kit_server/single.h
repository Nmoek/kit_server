#ifndef _KIT_SINGLE_H_
#define _KIT_SINGLE_H_

#include <memory>

namespace kit_server
{
    
template<class T, class X = void, int N = 0>
class Single
{
public:
    static T* GetInstance()
    {
        static T v;//C++11 魔力static 保证线程安全
        return &v;
    }
};


template<class T, class X = void, int N = 0>
class SinglePtr
{
public:
    static std::shared_ptr<T> GetInstance()
    {
        static std::shared_ptr<T> v(new T);
        return v;
    }
};

}
#endif