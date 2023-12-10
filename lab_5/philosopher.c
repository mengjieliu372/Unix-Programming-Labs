#include "include/apue.h"
#include "include/lock.h"
#include <wait.h>
#define N 5

// declare
void philosopher(int i);
void takeFork(int i);
void putFork(int i);
void thinking(int i, int time);
void eating(int i, int time);

// 定义5个文件，分别表示5个叉子
static char *forks[N] = {"fork0", "fork1", "fork2", "fork3", "fork4"};
static int nsecs = 2;      // 缺省值 2s

int main(int argc, char *argv[])
{
    // 参数 check
    if (argc != 1 && argc != 3){
        err_quit("argv error: ./philosopher [ -t <time> ]");
    } else if (argc == 3){
        if (strcmp(argv[1], "-t") != 0){
            err_quit("argv error: ./philosopher -t <time>");
        } else{
            nsecs = atoi(argv[2]);
        }
    }
    // 初始化
    int i;
    for (i = 0; i < N;i++){
        initlock(forks[i]);
    }
    // 生成 5 个哲学家进程
    pid_t pid;
    for (i = 0; i < N; i++) {
        if ((pid = fork()) < 0){
            err_quit("fork error");
        }
        else if (pid == 0){     // 父进程
            philosopher(i);
        }
        
    }
    wait(NULL);
    return 0;
}

// 哲学家的行为
void philosopher(int i)
{
    while (1)
    {
        thinking(i, nsecs); // 哲学家i思考nsecs秒
        takeFork(i);        // 哲学家i拿起叉子
        eating(i, nsecs);   // 哲学家i进餐nsecs秒
        putFork(i);         // 哲学家i放下叉子
    }
}

// 拿起叉子
void takeFork(int i) {
    if (i == N - 1) {
        lock(forks[0]);
        lock(forks[i]);
    }
    else {
        lock(forks[i]);
        lock(forks[i + 1]);
    }
}

// 放下叉子
void putFork(int i) {
    if (i == N - 1) {
        unlock(forks[0]);
        unlock(forks[i]);
    }
    else {
        unlock(forks[i]);
        unlock(forks[i + 1]);
    }
}

void thinking(int i, int nsecs)
{
    printf("philosopher %d is thinking\n", i);
    sleep(nsecs);
}

void eating(int i, int nsecs)
{
    printf("philosopher %d is eating\n", i);
    sleep(nsecs);
}
