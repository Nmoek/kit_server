#include <iostream>
#include <string>


using namespace std;




void test(int v)
{
    cout << "test1 run, v=" << v << endl;
}

void test(int &v)
{
    cout << "test2 run, v=" << v << endl;
}

void test(const int &&v)
{
    cout << "test3 run, v=" << v << endl;
}

// void test(const int &v)
// {
//     cout << "test4 run, v=" << v << endl;
// }


template<class T>
void RunTest(T &&v)
{
    test(std::forward<T>(v));
}


int main()
{
    int a = 1;
    int b = 2;
    const int c = 3;
    const int d = 4;

    RunTest(a);                 //作为左值引用被转发
    RunTest(std::move(b));      //作为右值引用被转发
    RunTest(c);                 //作为const左值引用被转发
    RunTest(std::move(d));      //作为const右值引用被转发
    



    return 0;
}
