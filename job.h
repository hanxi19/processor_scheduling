#ifndef _JOB_H
#define _JOB_H

#include <sys/types.h>
#include <time.h>
#include <signal.h>

#define DATALEN 1024
#define BUFLEN 1024
#define FIFO "/tmp/jobfifo"

// 作业状态定义
#define READY 0
#define RUNNING 1
#define DONE 2

// 作业命令类型定义
#define ENQ 1
#define DEQ 2
#define STAT 3

// 作业信息结构体
struct jobinfo {
    int jid;                // 作业ID
    int pid;                // 进程ID
    int ownerid;            // 所有者ID
    int defpri;             // 默认优先级
    int curpri;             // 当前优先级
    int priority;           // 多级反馈队列中的优先级
    int state;              // 作业状态
    int run_time;           // 运行时间
    int wait_time;          // 等待时间
    int wait_time_hrrf;     // HRRF等待时间
    time_t create_time;     // 创建时间
    time_t arrival_time;    // 到达时间
    int duration;           // 预计运行时间
    int remaining_time;     // 剩余运行时间
    char **cmdarg;          // 命令行参数
};

// 等待队列节点结构体
struct waitqueue {
    struct jobinfo *job;    // 作业信息
    struct waitqueue *next; // 下一个节点
};

// 作业命令结构体
struct jobcmd {
    int type;               // 命令类型
    int owner;              // 所有者ID
    int defpri;             // 默认优先级
    int argnum;             // 参数数量
    int duration;           // 预计运行时间
    char data[DATALEN];     // 数据
};

// 函数声明
void error_sys(const char *msg);
void schedule(void);
void updateall(void);
void jobswitch(void);
void do_enq(struct jobinfo *newjob, struct jobcmd enqcmd);
void do_deq(struct jobcmd deqcmd);
void do_stat(void);
int allocjid(void);

// 调度算法函数指针
extern struct waitqueue* (*jobselect)(void);

#endif