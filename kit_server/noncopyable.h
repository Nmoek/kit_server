#ifndef _KIT_NONCOPYABLE_H_
#define _KIT_NONCOPYABLE_H_


namespace kit_server
{

class Noncopyable
{

public:
    Noncopyable() = default;
    ~Noncopyable() = default;
    Noncopyable(const Noncopyable &) = delete;
    Noncopyable& operator=(const Noncopyable&) = delete;

};

}




#endif 