#include <iostream>


using namespace std;

template<class T, class F = double>
class A
{
public:
    void test(T a, F b)
    {
        cout << "无特化:" << a + b << endl;
    }

};


template<>
class A<int, int>
{
public:
    void test(int a, int b)
    {
        cout << "全特化:" << a + b << endl;
    }
};

template<class F>
class A<float, F>
{
public:
    void test(float a, F b)
    {
        cout << "偏特化:" << a + b << endl;
    }
};
    


int main()
{
    A<double, double> a1;
    A<float, int> a2;
    A<int, int> a3;
    A<int> a4;
    a1.test(1, 2.3);
    a2.test(1, 2.3);
    a3.test(1, 2.3);
    a4.test(1, 2.3);
    
    return 0;
}

