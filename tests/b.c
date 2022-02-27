


#define __USE_GNU
#include <unistd.h>
#include <sched.h>
#include <sys/syscall.h>
#include <stdlib.h>

#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include <pthread.h>
#include <setjmp.h>
#include <sys/types.h>

//pthread_mutex_t m_mutex;
pthread_spinlock_t m_mutex;

//线程局部变量
pthread_key_t key;      //C写法

//thread_local key;     //C++写法

int inc(int *value, int add)
{
    int old;
    __asm__ volatile (
        "lock; xaddl %2, %1;"
        : "=a" (old)
        : "m" (*value), "a"(add)
        : "cc", "memory"
    );

    return old;
}


void*func(void* arg)
{
    int *count = (int *)arg;

    int i = 0;
    while(i++ < 100000)
    {
#if 0
        pthread_spin_lock(&m_mutex);
        (*count)++;
        pthread_spin_unlock(&m_mutex);
#else 

        inc(count, 1);


#endif

        usleep(1);
    }
}


typedef void* (*thread_cb)(void *);


struct m_pair
{
    int a;
    int b;
};


void print_thread1_key()
{
    int *p = (int *)pthread_getspecific(key);
    printf("thread1 key = %d, key address:%p\n", *p, p);
}

void print_thread2_key()
{
    char *p = (char *)pthread_getspecific(key);
    printf("thread1 key = %c, key address:%p\n", *p, p);
}

void print_thread3_key()
{
    struct m_pair *p = (struct m_pair *)pthread_getspecific(key);
    printf("thread1 key = %d %d, key address:%p\n", p->a, p->b, p);
}

void *pro_func1(void *arg)
{
    int i = 5;
    pthread_setspecific(key, &i);
    print_thread1_key();

}

void *pro_func2(void *arg)
{
    char i = 'a';
    pthread_setspecific(key, &i);
    print_thread2_key();
}



void *pro_func3(void *arg)
{
    struct m_pair p = {1 , 2};
    pthread_setspecific(key, &p);
    print_thread3_key();
}


#define Try c = setjmp(env);if(c == 0)
#define Catch(type) else if(c == type)
#define Throw   longjmp(env, type)
#define Finally

jmp_buf env;
int c = 0;


void jmp_func(int idx)
{
    longjmp(env, idx);
}



void process_affinity(int num)
{
    pid_t selfid = syscall(__NR_gettid);
    cpu_set_t mask;
    

    //CPU_ZERO(&mask);

    //CPU_SET(selfid%num, &mask);

    sched_setaffinity(selfid, sizeof(mask), &mask);

    while(1);
}

int main()
{
    int num = sysconf(_SC_NPROCESSORS_CONF);
    printf("核心=%d\n", num);
    pid_t pid;
    for(int i = 0;i < num / 2;i++)
    {
        pid = fork();

        if(pid <= 0)
        {
            break;
        }
    }

    if(pid == 0)
        process_affinity(num);
    
    while(1) usleep(1);

    // pthread_t t[10] = {0};

    // c = setjmp(env);
    // if(c == 0)
    // {
    //     printf("c = %d\n", c);
    //     jmp_func(++c);
        
    // }
    // else if(c == 1)
    // {
    //     printf("c = %d\n", c);
    //     jmp_func(++c);
    // }
    // else if(c == 2)
    // {
    //     printf("c = %d\n", c);
    //     jmp_func(++c);
    // }
    // else if(c == 3)
    // {
    //     printf("c = %d\n", c);
    //     jmp_func(++c);
    // }
    // else 
    //     printf("other\n");

    // Try{
    //     printf("c = %d\n", c);
    //     jmp_func(++c);
    // }Catch(1){

    //     printf("c = %d\n", c);
    //     jmp_func(++c);
    // }Catch(2){
    //     printf("c = %d\n", c);
    //     jmp_func(++c);
    // }Catch(3){
    //     printf("c = %d\n", c);
    //     jmp_func(++c);
    // }Finally{
    //     printf("other\n");
    // }

    

    //pthread_mutex_init(&m_mutex, NULL);
    // pthread_spin_init(&m_mutex, 0);

    // pthread_key_create(&key, NULL);

    // int count = 0;
    // for(int i = 0;i < 10;i++)
    // {
    //     pthread_create(&t[i], NULL, func, &count);
    // }

    // for(int i = 0;i < 100;++i)
    // {
    //     std::cout << count << std::endl;
    //     sleep(1);
    // }

    // thread_cb cb[3] = {
    //     pro_func1,
    //     pro_func2,
    //     pro_func3
    // };

    // int count = 0;
    // int idx = 0;
    // for(int i = 0;i < 3;i++)
    // {
    //     pthread_create(&t[i], NULL, cb[i], &count);
    // }

    // for(int i = 0;i < 3;i++)
    // {
    //     pthread_join(t[i], NULL);
    // }

    return 0;
}