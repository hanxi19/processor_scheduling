#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "job.h"

// 示例1：高优先级优先调度
void example_priority_scheduling() {
    printf("\n=== 高优先级优先调度示例 ===\n");
    
    // 创建几个不同优先级的作业
    job_t *job1 = create_job(1, 5, 3);  // 优先级3
    job_t *job2 = create_job(2, 3, 1);  // 优先级1
    job_t *job3 = create_job(3, 4, 2);  // 优先级2
    
    // 将作业加入队列
    enq_job(job1);
    enq_job(job2);
    enq_job(job3);
    
    // 设置调度算法为高优先级优先
    set_scheduling_algorithm(PRIORITY);
    
    // 启动调度器
    start_scheduler();
    
    // 等待所有作业完成
    sleep(15);
    
    // 停止调度器
    stop_scheduler();
    
    // 清理资源
    free_job(job1);
    free_job(job2);
    free_job(job3);
}

// 示例2：先来先服务调度
void example_fcfs_scheduling() {
    printf("\n=== 先来先服务调度示例 ===\n");
    
    // 创建几个不同到达时间的作业
    job_t *job1 = create_job(1, 5, 1);  // 到达时间1
    job_t *job2 = create_job(2, 3, 2);  // 到达时间2
    job_t *job3 = create_job(3, 4, 3);  // 到达时间3
    
    // 将作业加入队列
    enq_job(job1);
    enq_job(job2);
    enq_job(job3);
    
    // 设置调度算法为先来先服务
    set_scheduling_algorithm(FCFS);
    
    // 启动调度器
    start_scheduler();
    
    // 等待所有作业完成
    sleep(15);
    
    // 停止调度器
    stop_scheduler();
    
    // 清理资源
    free_job(job1);
    free_job(job2);
    free_job(job3);
}

// 示例3：短作业优先调度
void example_sjf_scheduling() {
    printf("\n=== 短作业优先调度示例 ===\n");
    
    // 创建几个不同执行时间的作业
    job_t *job1 = create_job(1, 2, 1);  // 执行时间2
    job_t *job2 = create_job(2, 5, 1);  // 执行时间5
    job_t *job3 = create_job(3, 3, 1);  // 执行时间3
    
    // 将作业加入队列
    enq_job(job1);
    enq_job(job2);
    enq_job(job3);
    
    // 设置调度算法为短作业优先
    set_scheduling_algorithm(SJF);
    
    // 启动调度器
    start_scheduler();
    
    // 等待所有作业完成
    sleep(15);
    
    // 停止调度器
    stop_scheduler();
    
    // 清理资源
    free_job(job1);
    free_job(job2);
    free_job(job3);
}

// 示例4：动态添加作业
void example_dynamic_jobs() {
    printf("\n=== 动态添加作业示例 ===\n");
    
    // 创建初始作业
    job_t *job1 = create_job(1, 3, 1);
    job_t *job2 = create_job(2, 4, 2);
    
    // 将作业加入队列
    enq_job(job1);
    enq_job(job2);
    
    // 设置调度算法为高优先级优先
    set_scheduling_algorithm(PRIORITY);
    
    // 启动调度器
    start_scheduler();
    
    // 等待一段时间后添加新作业
    sleep(5);
    job_t *job3 = create_job(3, 2, 3);
    enq_job(job3);
    
    // 再等待一段时间后添加新作业
    sleep(3);
    job_t *job4 = create_job(4, 5, 1);
    enq_job(job4);
    
    // 等待所有作业完成
    sleep(15);
    
    // 停止调度器
    stop_scheduler();
    
    // 清理资源
    free_job(job1);
    free_job(job2);
    free_job(job3);
    free_job(job4);
}

int main() {
    // 运行所有示例
    example_priority_scheduling();
    example_fcfs_scheduling();
    example_sjf_scheduling();
    example_dynamic_jobs();
    
    return 0;
} 