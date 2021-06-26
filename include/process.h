//
// Created by Wenxin Zheng on 2021/3/5.
//

#ifndef ACMOS_SPR21_PROCESS_H
#define ACMOS_SPR21_PROCESS_H

#include <list.h>
#include <pagetable.h>
#include "lock.h"

typedef enum state { UNUSED, USED, SLEEPING, RUNNABLE, RUNNING, ZOMBIE, IDLE } process_state_t;
typedef enum file_type{PUTC} file_type_t;

typedef struct context{
    uint64 ra;
    uint64 sp;

    uint64 s0;
    uint64 s1;
    uint64 s2;
    uint64 s3;
    uint64 s4;
    uint64 s5;
    uint64 s6;
    uint64 s7;
    uint64 s8;
    uint64 s9;
    uint64 s10;
    uint64 s11;
} context_t;

typedef struct trapframe {
    uint64 kernel_satp;   // kernel page table
    uint64 kernel_sp;     // top of process's kernel stack
    uint64 kernel_trap;   // usertrap()
    uint64 epc;           // saved user program counter
    uint64 kernel_hartid; // saved kernel tp
    uint64 ra;
    uint64 sp;
    uint64 gp;
    uint64 tp;
    uint64 t0;
    uint64 t1;
    uint64 t2;
    uint64 s0;
    uint64 s1;
    uint64 a0;
    uint64 a1;
    uint64 a2;
    uint64 a3;
    uint64 a4;
    uint64 a5;
    uint64 a6;
    uint64 a7;
    uint64 s2;
    uint64 s3;
    uint64 s4;
    uint64 s5;
    uint64 s6;
    uint64 s7;
    uint64 s8;
    uint64 s9;
    uint64 s10;
    uint64 s11;
    uint64 t3;
    uint64 t4;
    uint64 t5;
    uint64 t6;
} trapframe_t;



typedef struct process {
    struct thread *thread;
    process_state_t process_state;

    // 以下部分请根据自己的需要自行填充
    int killed;

    //uint64 kstack;
    pagetable_t pagetable;

    int pid;
} process_t;

typedef struct cpu{
    struct thread *thread;
    context_t context;
    int intena;
    //todo
} cpu_t;

// 状态可以根据自己的需要进行修改
typedef process_state_t thread_state_t;

typedef struct thread {

    struct list_head process_list_thread_node;
    thread_state_t thread_state;
    struct list_head sched_list_thread_node;
    // 以下部分请根据自己的需要自行填充
    context_t context;
    uint64 kstack;
    struct lock lock_;
    process_t *process;
    trapframe_t *trapframe;

} thread_t;

process_t *alloc_proc(const char* bin, thread_t *thr);
bool load_thread(file_type_t type);
void sched_enqueue(thread_t *target_thread);
thread_t *sched_dequeue();
bool sched_empty();
void sched_start();
void sched_init();
void proc_init();
void trap_init_vec();

//swtch.s
void swtch(context_t*,context_t*);
void sched(void);
void yield(void);
cpu_t* mycpu(void);
thread_t * mythre();
#endif  // ACMOS_SPR21_PROCESS_H
