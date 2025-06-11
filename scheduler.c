/**
 * @file scheduler.c
 * @brief 进程调度器实现
 * @details 实现了一个支持多种调度算法的进程调度器，包括HPF、FCFS和SJF算法
 */

#include <stdio.h>      // 标准输入输出
#include <unistd.h>     // 系统调用接口
#include <sys/types.h>  // 基本系统数据类型
#include <sys/stat.h>   // 文件状态
#include <sys/time.h>   // 时间相关
#include <sys/wait.h>   // 进程等待
#include <string.h>     // 字符串处理
#include <signal.h>     // 信号处理
#include <fcntl.h>      // 文件控制
#include <time.h>       // 时间函数
#include "job.h"        // 作业相关定义

// 全局变量定义
int jobid = 0;          // 作业ID计数器
int siginfo = 1;        // 信号处理标志
int fifo;               // FIFO文件描述符
int globalfd;           // 全局文件描述符

// 作业队列相关指针
struct waitqueue *head = NULL;      // 等待队列头指针
struct waitqueue *next = NULL;      // 下一个要运行的作业
struct waitqueue *current = NULL;   // 当前运行的作业

/**
 * @brief 调度器核心函数
 * @details 处理作业命令、更新作业状态、选择下一个要运行的作业
 */
void schedule()
{
	struct jobinfo *newjob = NULL;
	struct jobcmd cmd;
	int count = 0;

	// 清空命令结构体并读取新命令
	bzero(&cmd, DATALEN);
	if ((count = read(fifo, &cmd, DATALEN)) < 0)
		error_sys("read fifo failed");

#ifdef DEBUG
	// 调试信息输出
	if (count) {
		printf("cmd cmdtype\t%d\n"
			"cmd defpri\t%d\n"
			"cmd data\t%s\n",
			cmd.type, cmd.defpri, cmd.data);
	}
#endif

	// 根据命令类型执行相应操作
	switch (cmd.type) {
	case ENQ:    // 作业入队
		do_enq(newjob,cmd);
		break;
	case DEQ:    // 作业出队
		do_deq(cmd);
		break;
	case STAT:   // 状态查询
		do_stat(cmd);
		break;
	default:
		break;
	}

	// 更新所有作业状态
	updateall();

	// 选择下一个要运行的作业
	next = (*jobselect)();

	// 执行作业切换
	jobswitch();
}

/**
 * @brief 分配新的作业ID
 * @return 新分配的作业ID
 */
int allocjid()
{
	return ++jobid;
}

/**
 * @brief 更新所有作业的状态
 * @details 更新运行中作业的运行时间和等待中作业的等待时间
 */
void updateall()
{
	struct waitqueue *p;

	// 更新运行中作业的运行时间
	if (current)
		current->job->run_time += 1;    // 增加1表示100ms

	// 更新等待中作业的等待时间和优先级
	for (p = head; p != NULL; p = p->next) {
		p->job->wait_time += 1;

		// 如果等待时间超过1个时间片且当前优先级小于3，则提升优先级
		if (p->job->wait_time > 1 && p->job->curpri < 3)
			p->job->curpri++;
	}
}

/**
 * @brief 高优先级优先(HPF)调度算法
 * @return 选中的作业
 * @details 选择当前优先级最高的作业，如果优先级相同则选择等待时间最长的作业
 */
struct waitqueue* jobselect_HPF()
{
	struct waitqueue *p, *selected = NULL;
	int highest_pri = -1;  // 最高优先级初始化为-1

	// 遍历等待队列，找到优先级最高的作业
	for (p = head; p != NULL; p = p->next) {
		// 如果找到更高优先级的作业，或者优先级相同但等待时间更长
		if (p->job->curpri > highest_pri || 
			(p->job->curpri == highest_pri && 
			 p->job->wait_time > selected->job->wait_time)) {
			highest_pri = p->job->curpri;
			selected = p;
		}
	}

	return selected;
}

/**
 * @brief 先来先服务(FCFS)调度算法
 * @return 选中的作业
 * @details 选择等待时间最长的作业
 */
struct waitqueue* jobselect_FCFS()
{
	struct waitqueue *p, *selected = NULL;
	int longest_wait = -1;  // 最长等待时间初始化为-1

	// 遍历等待队列，找到等待时间最长的作业
	for (p = head; p != NULL; p = p->next) {
		if (p->job->wait_time > longest_wait) {
			longest_wait = p->job->wait_time;
			selected = p;
		}
	}

	return selected;
}

/**
 * @brief 短作业优先(SJF)调度算法
 * @return 选中的作业
 * @details 非抢占式实现，选择预计运行时间最短的作业
 */
struct waitqueue* jobselect_SJF()
{
	struct waitqueue *p, *selected = NULL;
	int shortest_duration = 0x7fffffff;  // 最短运行时间初始化为最大整数

	// 遍历等待队列，找到预计运行时间最短的作业
	for (p = head; p != NULL; p = p->next) {
		// 如果找到运行时间更短的作业，或者运行时间相同但等待时间更长
		if (p->job->duration < shortest_duration || 
			(p->job->duration == shortest_duration && 
			 p->job->wait_time > selected->job->wait_time)) {
			shortest_duration = p->job->duration;
			selected = p;
		}
	}

	return selected;
}

/**
 * @brief 时间片轮转调度算法
 * @return 选中的作业
 * @details 选择等待时间最长的作业
 */
struct waitqueue* jobselect_RR() {
    struct waitqueue* selected = NULL;
    struct waitqueue* current = head;
    
    // 如果队列为空，返回NULL
    if (current == NULL) {
        return NULL;
    }
    
    // 选择队首作业
    selected = current;
    
    // 将选中的作业移到队尾
    if (current->next != NULL) {
        head = current->next;
        struct waitqueue* tail = head;
        for (; tail->next != NULL; tail = tail->next);
        tail->next = selected;
        selected->next = NULL;
    }
    
    return selected;
}

/**
 * @brief 最高响应比优先调度算法
 * @return 选中的作业
 * @details 选择响应比最高的作业
 */
struct waitqueue* jobselect_HRRN() {
    struct waitqueue* selected = NULL;
    struct waitqueue* current = head;
    float highest_ratio = -1.0;
    struct waitqueue* prev = NULL;
    struct waitqueue* selected_prev = NULL;
    
    // 如果队列为空，返回NULL
    if (current == NULL) {
        return NULL;
    }
    
    // 计算当前时间
    struct timeval current_time;
    gettimeofday(&current_time, NULL);
    
    // 遍历队列找到响应比最高的作业
    while (current != NULL) {
        float wait_time = (current_time.tv_sec - current->job->arrival_time.tv_sec) +
                         (current_time.tv_usec - current->job->arrival_time.tv_usec) / 1000000.0;
        float response_ratio = (wait_time + current->job->execution_time) / current->job->execution_time;
        
        if (response_ratio > highest_ratio) {
            highest_ratio = response_ratio;
            selected = current;
            selected_prev = prev;
        }
        
        prev = current;
        current = current->next;
    }
    
    // 从队列中移除选中的作业
    if (selected_prev != NULL) {
        selected_prev->next = selected->next;
    } else {
        head = selected->next;
    }
    selected->next = NULL;
    
    return selected;
}

/**
 * @brief 多级反馈队列调度算法
 * @return 选中的作业
 * @details 选择优先级最高的作业
 */
struct waitqueue* jobselect_MLFQ() {
    struct waitqueue* selected = NULL;
    struct waitqueue* current = head;
    
    // 如果队列为空，返回NULL
    if (current == NULL) {
        return NULL;
    }
    
    // 从当前队列开始查找
    for (int i = 0; i < MAX_QUEUES; i++) {
        int queue_index = (current_queue + i) % MAX_QUEUES;
        struct waitqueue* current = head;
        struct waitqueue* prev = NULL;
        
        // 在当前队列中查找作业
        while (current != NULL) {
            if (current->job->priority == queue_index) {
                selected = current;
                
                // 从队列中移除选中的作业
                if (prev != NULL) {
                    prev->next = selected->next;
                } else {
                    head = selected->next;
                }
                selected->next = NULL;
                
                // 如果作业未完成，降低其优先级
                if (selected->job->remaining_time > TIME_QUANTUM) {
                    selected->job->priority = (queue_index + 1) % MAX_QUEUES;
                }
                
                // 更新当前队列索引
                current_queue = (queue_index + 1) % MAX_QUEUES;
                return selected;
            }
            
            prev = current;
            current = current->next;
        }
    }
    
    return NULL;
}

/**
 * @brief 公平共享调度算法
 * @return 选中的作业
 * @details 选择资源使用率最低的作业
 */
struct waitqueue* jobselect_FairShare() {
    struct waitqueue* selected = NULL;
    struct waitqueue* current = head;
    float lowest_share = 1.0;
    struct waitqueue* prev = NULL;
    struct waitqueue* selected_prev = NULL;
    
    // 如果队列为空，返回NULL
    if (current == NULL) {
        return NULL;
    }
    
    // 计算每个用户的资源使用情况
    while (current != NULL) {
        float user_share = (float)current->job->cpu_usage / current->job->max_cpu_usage;
        
        if (user_share < lowest_share) {
            lowest_share = user_share;
            selected = current;
            selected_prev = prev;
        }
        
        prev = current;
        current = current->next;
    }
    
    // 从队列中移除选中的作业
    if (selected_prev != NULL) {
        selected_prev->next = selected->next;
    } else {
        head = selected->next;
    }
    selected->next = NULL;
    
    return selected;
}

/**
 * @brief 作业切换函数
 * @details 处理作业的切换、终止和启动
 */
void jobswitch()
{
    struct waitqueue *p;
    int i;
    
    // 处理已完成的作业
    if (current && current->job->state == DONE) {
        // 释放作业资源
        for (i = 0; (current->job->cmdarg)[i] != NULL; i++) {
            free((current->job->cmdarg)[i]);
            (current->job->cmdarg)[i] = NULL;
        }
        
        free(current->job->cmdarg);
        free(current->job);
        free(current);
        
        current = NULL;
    }
    
    // 处理作业切换的不同情况
    if (next == NULL && current == NULL)          // 没有作业要运行
        return;
    
    else if (next != NULL && current == NULL) {   // 启动新作业
        printf("begin start new job\n");
        current = next;
        next = NULL;
        current->job->state = RUNNING;
        kill(current->job->pid, SIGCONT);
        return;
        
    } else if (next != NULL && current != NULL) { // 执行作业切换
        // 暂停当前作业
        kill(current->job->pid, SIGSTOP);
        current->job->state = READY;
        
        // 启动新作业
        current = next;
        next = NULL;
        current->job->state = RUNNING;
        kill(current->job->pid, SIGCONT);
        
        printf("\nbegin switch: current jid=%d, pid=%d\n",
               current->job->jid, current->job->pid);
        return;
        
    } else {    // 不需要切换
        return;		
    }
}

/**
 * @brief 信号处理函数
 * @param sig 信号类型
 * @param info 信号信息
 * @param notused 未使用的参数
 */
void sig_handler(int sig, siginfo_t *info, void *notused)
{
	int status;
	int ret;

	switch (sig) {
	case SIGVTALRM:  // 定时器信号
		schedule();
		return;

	case SIGCHLD:    // 子进程状态变化信号
		ret = waitpid(-1, &status, WNOHANG);
		if (ret == 0 || ret == -1)
			return;

		// 处理子进程的不同退出状态
		if (WIFEXITED(status)) {  // 正常退出
			current->job->state = DONE;
			printf("normal termation, exit status = %d\tjid = %d, pid = %d\n\n",
				WEXITSTATUS(status), current->job->jid, current->job->pid);

		}  else if (WIFSIGNALED(status)) {  // 被信号终止
		    printf("abnormal termation, signal number = %d\tjid = %d, pid = %d\n\n",
				WTERMSIG(status), current->job->jid, current->job->pid);

		} else if (WIFSTOPPED(status)) {  // 被信号停止
		    printf("child stopped, signal number = %d\tjid = %d, pid = %d\n\n",
				WSTOPSIG(status), current->job->jid, current->job->pid);
		}
		return;

	default:
		return;
	}
}

/**
 * @brief 作业入队处理函数
 * @param newjob 新作业信息
 * @param enqcmd 入队命令
 */
void do_enq(struct jobinfo *newjob, struct jobcmd enqcmd)
{
	struct	waitqueue *newnode, *p;
	int		i=0, pid;
	char	*offset, *argvec, *q;
	char	**arglist;
	sigset_t	zeromask;

	sigemptyset(&zeromask);

	// 初始化作业信息结构体
	newjob = (struct jobinfo *)malloc(sizeof(struct jobinfo));
	newjob->jid = allocjid();
	newjob->defpri = enqcmd.defpri;
	newjob->curpri = enqcmd.defpri;
    newjob->duration = enqcmd.duration;
	newjob->ownerid = enqcmd.owner;
	newjob->state = READY;
	newjob->create_time = time(NULL);
    newjob->wait_time = 0;
	newjob->run_time = 0;

	// 处理命令行参数
	arglist = (char**)malloc(sizeof(char*)*(enqcmd.argnum+1));
	newjob->cmdarg = arglist;
	offset = enqcmd.data;
	argvec = enqcmd.data;
	while (i < enqcmd.argnum) {
		if (*offset == ':') {
			*offset++ = '\0';
			q = (char*)malloc(offset - argvec);
			strcpy(q,argvec);
			arglist[i++] = q;
			argvec = offset;
		} else
			offset++;
	}
	arglist[i] = NULL;

#ifdef DEBUG
	// 调试信息输出
	printf("enqcmd argnum %d\n",enqcmd.argnum);
	for (i = 0; i < enqcmd.argnum; i++)
		printf("parse enqcmd:%s\n",arglist[i]);
#endif

	// 将新作业添加到等待队列
	newnode = (struct waitqueue*)malloc(sizeof(struct waitqueue));
	newnode->next = NULL;
	newnode->job = newjob;

	if (head) {
		for (p = head; p->next != NULL; p = p->next);
		p->next = newnode;
	} else
		head = newnode;

	// 创建子进程运行作业
	if ((pid = fork()) < 0)
		error_sys("enq fork failed");

	if (pid == 0) {  // 子进程
		newjob->pid = getpid();
		raise(SIGSTOP);  // 暂停等待调度

#ifdef DEBUG
		printf("begin running\n");
		for (i = 0; arglist[i] != NULL; i++)
			printf("arglist %s\n",arglist[i]);
#endif

		// 重定向输出并执行程序
		dup2(globalfd,1);
		if (execv(arglist[0],arglist) < 0)
			printf("exec failed\n");

		exit(1);
	} else {  // 父进程
		newjob->pid = pid;
		printf("\nnew job: jid=%d, pid=%d\n", newjob->jid, newjob->pid);
	}
}

/**
 * @brief 作业出队处理函数
 * @param deqcmd 出队命令
 */
void do_deq(struct jobcmd deqcmd)
{
    int deqid, i;
    struct waitqueue *p, *prev, *select, *selectprev;

    deqid = atoi(deqcmd.data);

#ifdef DEBUG
    printf("deq jid %d\n", deqid);
#endif

    // 在等待队列中查找要终止的作业
    select = NULL;
    selectprev = NULL;
    for (p = head, prev = NULL; p != NULL; prev = p, p = p->next) {
        if (p->job->jid == deqid) {
            select = p;
            selectprev = prev;
            break;
        }
    }

    // 如果找到要终止的作业
    if (select != NULL) {
        // 从等待队列中移除
        if (selectprev == NULL) {
            head = select->next;
        } else {
            selectprev->next = select->next;
        }

        // 终止作业进程
        kill(select->job->pid, SIGKILL);
        
        // 释放资源
        for (i = 0; (select->job->cmdarg)[i] != NULL; i++) {
            free((select->job->cmdarg)[i]);
        }
        free(select->job->cmdarg);
        free(select->job);
        free(select);

        printf("terminate job %d\n", deqid);
    }
}

/**
 * @brief 作业状态查询函数
 * @details 显示所有作业的详细信息
 */
void do_stat()
{
	struct waitqueue *p;
	char timebuf[BUFLEN];

	// 打印表头
	printf("JID\tPID\tOWNER\tRUNTIME\tWAITTIME\tCREATTIME\tSTATE\tDEFPRI\tCURPRI\n");

	// 显示当前运行作业的信息
	if (current) {
		strcpy(timebuf,ctime(&(current->job->create_time)));
		timebuf[strlen(timebuf) - 1] = '\0';
		printf("%d\t%d\t%d\t%d\t%d\t%s\t%s\t%d\t%d\n",
			current->job->jid,
			current->job->pid,
			current->job->ownerid,
			current->job->run_time,
			current->job->wait_time,
			timebuf,
			"RUNNING",
            current->job->defpri,
            current->job->curpri
            );
	}

	// 显示等待队列中作业的信息
	for (p = head; p != NULL; p = p->next) {
		strcpy(timebuf,ctime(&(p->job->create_time)));
		timebuf[strlen(timebuf) - 1] = '\0';
		printf("%d\t%d\t%d\t%d\t%d\t%s\t%s\t%d\t%d\n",
			p->job->jid,
			p->job->pid,
			p->job->ownerid,
			p->job->run_time,
			p->job->wait_time,
			timebuf,
			"READY",
            p->job->defpri,
            p->job->curpri
            );
	}

	printf("\n");
}

/**
 * @brief 主函数
 * @details 初始化调度器，设置信号处理，启动调度循环
 */
int main()
{
	struct timeval interval;
	struct itimerval new, old;
	struct stat statbuf;
	struct sigaction newact, oldact1, oldact2;

	// 初始化FIFO
	if (stat(FIFO, &statbuf) == 0) {
		if (remove(FIFO) < 0)
			error_sys("remove failed");
	}

	if (mkfifo(FIFO, 0666) < 0)
		error_sys("mkfifo failed");

	// 以非阻塞方式打开FIFO
	if ((fifo = open(FIFO, O_RDONLY|O_NONBLOCK)) < 0)
		error_sys("open fifo failed");

	// 打开全局输出文件
	if ((globalfd = open("/dev/null", O_WRONLY)) < 0)
		error_sys("open global file failed");

    // 选择调度算法
    printf("=====Choose algorithm of Select_Job=====\n");
    printf("(1) HPF\n");
    printf("(2) FCFS\n");
    printf("(3) SJF\n");
    printf("(4) RR\n");
    printf("(5) HRRN\n");
    printf("(6) MLFQ\n");
    printf("(7) FairShare\n");
    int tmp_choose;
    scanf("%d", &tmp_choose);
    switch(tmp_choose) {
        case 1:
            jobselect = jobselect_HPF;
            break;
        case 2:
            jobselect = jobselect_FCFS;
            break;
        case 3:
            jobselect = jobselect_SJF;
            break;
        case 4:
            jobselect = jobselect_RR;
            break;
        case 5:
            jobselect = jobselect_HRRN;
            break;
        case 6:
            jobselect = jobselect_MLFQ;
            break;
        case 7:
            jobselect = jobselect_FairShare;
            break;
        default:
            printf("Invalidly Input!");
            exit(0);
    }

    // 设置信号处理器
    newact.sa_sigaction = sig_handler;
    newact.sa_flags = SA_SIGINFO;
    sigemptyset(&newact.sa_mask);

    // 安装SIGVTALRM信号处理器
    if (sigaction(SIGVTALRM, &newact, &oldact1) < 0)
        error_sys("sigaction SIGVTALRM failed");

    // 安装SIGCHLD信号处理器
    if (sigaction(SIGCHLD, &newact, &oldact2) < 0)
        error_sys("sigaction SIGCHLD failed");

    // 设置定时器
    interval.tv_sec = 1;
    interval.tv_usec = 0;  // 响应机制为1s

    new.it_interval = interval;
    new.it_value = interval;
    setitimer(ITIMER_VIRTUAL, &new, &old);

    printf("OK! Scheduler is starting now!!\n");

    // 主循环
    while (siginfo == 1);

    // 清理资源
    close(fifo);
    close(globalfd);
    return 0;
}
