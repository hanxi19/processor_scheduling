/**
 * @file deq.c
 * @brief 作业出队命令实现
 * @details 实现从调度器中移除指定作业的功能
 */

#include <unistd.h>      // 提供系统调用接口
#include <string.h>      // 字符串处理函数
#include <sys/types.h>   // 基本系统数据类型
#include <sys/stat.h>    // 文件状态
#include <sys/ipc.h>     // IPC机制
#include <fcntl.h>       // 文件控制
#include "job.h"         // 作业相关定义

/**
 * @brief 显示命令使用说明
 * @details 当用户输入参数不正确时调用此函数
 */
void usage()
{
	printf("Usage:  deq jid\n"
		"\tjid\t\t the job id\n");
}

/**
 * @brief 主函数
 * @param argc 命令行参数数量
 * @param argv 命令行参数数组
 * @return 0表示成功，1表示失败
 * @details 实现作业出队命令的主要逻辑
 */
int main(int argc,char *argv[])
{
	struct jobcmd deqcmd;  // 定义作业命令结构体
	int fd;                // 文件描述符

	// 检查命令行参数数量是否正确
	if (argc != 2)
	{
		usage();
		return 1;
	}

	// 初始化作业命令结构体
	deqcmd.type = DEQ;           // 设置命令类型为出队
	deqcmd.defpri = 0;           // 设置默认优先级
	deqcmd.owner = getuid();     // 获取当前用户ID作为命令所有者
	deqcmd.argnum = 1;           // 设置参数数量为1

	// 将作业ID复制到命令数据中
	strcpy(deqcmd.data,*++argv);
	printf("jid %s\n",deqcmd.data);

	// 打开FIFO管道进行通信
	if ((fd = open(FIFO,O_WRONLY)) < 0)
		error_sys("deq open fifo failed");

	// 将命令写入FIFO管道
	if (write(fd,&deqcmd,DATALEN)< 0)
		error_sys("deq write failed");

	// 关闭FIFO管道
	close(fd);
	return 0;
}
