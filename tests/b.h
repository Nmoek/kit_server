
#include <iostream>
using namespace std;

class A
{
public:

    void func()
    {
       cout << 666666 << endl;
    }

protected:  
    int getA() const {return this->a;}
 
private:
    int a;
      void setA(int a) {a = a;}
};