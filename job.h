#ifndef JOB_H
#define JOB_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <signal.h>
#include <sys/types.h>

#define	FIFO "/tmp/server"

#ifndef DEBUG
 #define DEBUG
#endif

#undef DEBUG

#define BUFLEN 100
#define GLOBALFILE "screendump"

// 作业状态枚举
enum jobstate
{
  READY,    // 就绪
  RUNNING,  // 运行中
  DONE      // 完成
};

// 命令类型枚举
enum cmdtype
{
  ENQ = -1,   // 入队
  DEQ = -2,   // 出队
  STAT = -3   // 查询状态
};

/* 通过FIFO传递的数据结构，表示作业命令 */
struct jobcmd {
  enum	cmdtype type;   // 命令类型
  int	argnum;         // 参数数量
  int	owner;          // 作业所有者
  int	defpri;         // 默认优先级
  int   duration;       // 预计运行时间
  char	data[BUFLEN];   // 额外数据
};

#define DATALEN sizeof(struct jobcmd)
#define error_sys printf

// 作业信息结构体
struct jobinfo {
  int    jid;                 // 作业ID
  int    pid;                 // 进程ID
  char** cmdarg;              // 执行命令及参数
  int    defpri;              // 默认优先级
  int    curpri;              // 当前优先级
  int    ownerid;             // 作业所有者ID
  int    wait_time_hrrf;      // HRRF算法等待时间
  int    wait_time;           // 等待队列中的等待时间
  time_t create_time;         // 创建时间
  time_t arrival_time;        // 到达时间
  int    run_time;            // 已运行时间
  int    duration;            // 预计总运行时间
  enum   jobstate state;      // 作业状态
};

// 等待队列节点（双向链表）
struct waitqueue {
  struct waitqueue *next;    // 下一个等待作业
  struct jobinfo *job;       // 当前作业信息
};

// 调度函数声明
void schedule(); // 调度主函数
void sig_handler(int sig, siginfo_t *info, void *notused); // 信号处理函数
int  allocjid(); // 分配作业ID

void do_enq(struct jobinfo *newjob, struct jobcmd enqcmd); // 入队操作
void do_deq(struct jobcmd deqcmd); // 出队操作
void do_stat(); // 显示作业状态
void updateall(); // 更新所有作业信息

struct waitqueue* (*jobselect)(); // 作业选择函数指针
void jobswitch(); // 作业切换

#endif