#ifndef _A_H_
#define _A_H_

#include <memory>
#include <map>
#include <string>

class ABase
{
public:
    typedef std::shared_ptr<ABase> ptr;

    virtual ~ABase(){}

};

template <class T>
class A :public ABase
{
public:
    typedef std::shared_ptr<A> ptr;

private:


};


class B
{

public:
    typedef std::map<std::string, ABase::ptr> Map;
    
    template <class T>
    static typename A<T>::ptr test(const std::string &str)
    {

        typename A<T>::ptr v(new A<T>);

        m_map[str] = v;
        return v;


    }


private:
    static Map m_map;

};

B::Map B::m_map;


#endif 