#include <iostream>
#include <sys/uio.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <fstream>
#include <stdarg.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <byteswap.h>
#include <iomanip>
#include <algorithm>
#include <vector>
#include <netdb.h>
#include <netinet/tcp.h>

using namespace std;

class A
{
public:
    A(int a):a(a)
    {
        cout << "A" << endl;
    }

private:
    int a;
};

class B
{
public:
    B()
    {
        cout << "B" << endl;
    }
private:
    static A m_a;
};

int main()
{




    return 0;
}