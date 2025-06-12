/**
 * @file enq.c
 * @brief 作业入队命令实现
 * @details 实现向调度器提交新作业的功能，支持设置作业优先级和持续时间
 */

#include <unistd.h>      // 提供系统调用接口
#include <string.h>      // 字符串处理函数
#include <sys/types.h>   // 基本系统数据类型
#include <sys/stat.h>    // 文件状态
#include <sys/ipc.h>     // IPC机制
#include <fcntl.h>       // 文件控制
#include "job.h"         // 作业相关定义
#include <stdio.h>       // 标准输入输出
#include <stdlib.h>      // 动态内存分配

/**
 * @brief 显示命令使用说明
 * @details 当用户输入参数不正确时调用此函数，显示完整的命令格式和参数说明
 */
void usage()
{
	printf("Usage:  enq [-p num] [-d dur] e_file args\n"
		"\t-p num\t\t specify the job priority\n"    // 指定作业优先级
        "\t-d dur\t\t specify the job duration\n"    // 指定作业持续时间
        "\te_file\t\t the absolute path of the exefile\n"  // 可执行文件的绝对路径
		"\targs\t\t the args passed to the e_file\n");     // 传递给可执行文件的参数
}

/**
 * @brief 主函数
 * @param argc 命令行参数数量
 * @param argv 命令行参数数组
 * @return 0表示成功，1表示失败
 * @details 实现作业入队命令的主要逻辑，包括参数解析和命令发送
 */
int main(int argc,char *argv[])
{
	int	p = 0, d = 0;    // p: 优先级, d: 持续时间
	int	fd;              // FIFO文件描述符
	char	c, *offset;    // c: 选项字符, offset: 数据缓冲区偏移量
	struct jobcmd enqcmd; // 作业命令结构体

	// 检查是否有参数
	if (argc == 1) {
		usage();
		return 1;
	}

	// 解析命令行选项
	while (--argc > 0 && (*++argv)[0] == '-') {
		while ((c = *++argv[0]))
			switch (c) {
			case 'p':  // 处理优先级选项
				p = atoi(*(++argv));
				argc--;
				break;
            case 'd':  // 处理持续时间选项
				d = atoi(*(++argv));
				argc--;
				break;
			default:   // 处理非法选项
				printf("Illegal option %c\n",c);
				return 1;
			}
	}

	// 验证优先级范围（0-3）
	if (p < 0 || p > 3) {
		printf("invalid priority: must between 0 and 3\n");
		return 1;
	}
    // 验证持续时间范围（0-65535）
    if (d < 0 || d > 65535) {
		printf("invalid duration: must between 0 and 65535\n");
		return 1;
	}

	// 初始化作业命令结构体
	enqcmd.type = ENQ;           // 设置命令类型为入队
	enqcmd.defpri = p;           // 设置作业优先级
    enqcmd.duration = d;         // 设置作业持续时间
	enqcmd.owner = getuid();     // 获取当前用户ID作为作业所有者
	enqcmd.argnum = argc;        // 设置参数数量
	offset = enqcmd.data;        // 初始化数据缓冲区偏移量

	// 将命令行参数复制到命令数据中，用冒号分隔
	while (argc-- > 0) {
		strcpy(offset,*argv);    // 复制参数
		strcat(offset,":");      // 添加分隔符
		offset = offset + strlen(*argv) + 1;  // 更新偏移量
		argv++;
	}

#ifdef DEBUG
	// 调试信息输出
	printf("enqcmd cmdtype\t%d\n"
		"enqcmd owner\t%d\n"
		"enqcmd defpri\t%d\n"
		"enqcmd data\t%s\n",
		enqcmd.type, enqcmd.owner, enqcmd.defpri, enqcmd.data);
#endif

	// 打开FIFO管道进行通信
	if ((fd = open(FIFO,O_WRONLY)) < 0)
		error_sys("enq open fifo failed");

	// 将命令写入FIFO管道
	if (write(fd,&enqcmd,DATALEN)< 0)
		error_sys("enq write failed");

	// 关闭FIFO管道
	close(fd);
	return 0;
}
