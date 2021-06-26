#include "process.h"
#include "lock.h"
#include "pagetable.h"
#include "elf.h"
#include "riscv.h"
#include "memlayout.h"


extern const char binary_putc_start;
extern char trampoline[];
cpu_t cpus[NCPU];
process_t proc[NPROC];
thread_t thre[NPROC];
thread_t *running[NCPU];
struct list_head sched_list[NCPU];
struct lock pidlock, tidlock, schedlock;
int next_pid=1, next_tid=1;

int alloc_pid(){
    int pid;
    acquire(&pidlock);
    pid=next_pid;
    next_pid++;
    release(&pidlock);
    return pid;
}

int alloc_tid(){
    int tid;
    acquire(&tidlock);
    tid=next_tid;
    next_tid++;
    release(&tidlock);
    return tid;
}

cpu_t* mycpu(void){
    int id=cpuid();
    cpu_t *c=&cpus[id];
    return c;
}

thread_t * mythre(void){
    //todo
    cpu_t *c=mycpu();
    thread_t *t=c->thread;
    return t;
}

uint64 uvmalloc(pagetable_t pagetable, uint64 oldsz, uint64 newsz){
    if(oldsz >= newsz)
        return oldsz;
    oldsz = PGROUNDUP(oldsz);
    for(uint64 va=oldsz; va<newsz; va+=PGSIZE){
        char *mem=mm_kalloc();
        if(mem==0){
            BUG("uvmalloc");
        }
        memset(mem,0,PGSIZE);
        if(pt_map_pages(pagetable,va,(uint64)mem,PGSIZE,PTE_W|PTE_X|PTE_R|PTE_U)!=0)
            BUG("uvmalloc");
    }
    return newsz;
}

static void loadseg(pagetable_t pagetable, uint64 va,const char *bin, uint offset, uint sz){
    if(va%PGSIZE)
        BUG("loadseg");
    for(uint i=0; i<sz; i+=PGSIZE){
        uint64 pa=pt_query_address(pagetable,va+i);
        if(pa==0)
            BUG("loadseg");
        uint64 len;
        if(sz-i<PGSIZE)
            len=sz-i;
        else
            len=PGSIZE;
        memcpy((void *) pa, bin+offset+i, len);
    }
}

// 将ELF文件映射到给定页表的地址空间，返回pc的数值
// 关于 ELF 文件，请参考：https://docs.oracle.com/cd/E19683-01/816-1386/chapter6-83432/index.html
static uint64 load_binary(pagetable_t *target_page_table, const char *bin){
	struct elf_file *elf;
    int i;
    uint64 seg_sz, p_vaddr, seg_map_sz;
	elf = elf_parse_file(bin);
	
	/* load each segment in the elf binary */
	uint64 sz = 0;
	for (i = 0; i < elf->header.e_phnum; ++i) {
		if (elf->p_headers[i].p_type == PT_LOAD) {
            // 根据 ELF 文件格式做段映射
            // 从ELF中获得这一段的段大小
            seg_sz = elf->p_headers[i].p_memsz;
            // 对应段的在内存中的虚拟地址
            p_vaddr = elf->p_headers[i].p_vaddr;
            // 对映射大小做页对齐
			seg_map_sz = ROUNDUP(seg_sz + p_vaddr, PGSIZE) - PGROUNDDOWN(p_vaddr);
            // 接下来代码的期望目的：将程序代码映射/复制到对应的内存空间
            // 一种可能的实现如下：
            /* 
             * 在 target_page_table 中分配一块大小
             * 通过 memcpy 将某一段复制进入这一块空间
             * 页表映射修改
             */
            uint64 sz1;
            sz1=uvmalloc(*target_page_table,sz,p_vaddr+seg_sz);
		    if(sz1==0)
                BUG("load_binary");
		    sz=sz1;
            loadseg(*target_page_table,p_vaddr,bin,elf->p_headers[i].p_offset,elf->p_headers[i].p_filesz);
		}
	}
	/* PC: the entry point */
	return elf->header.e_entry;
}

/* 分配一个进程，需要至少完成以下目标：
 * 
 * 分配一个主线程
 * 创建一张进程页表
 * 分配pid、tid
 * 设置初始化线程上下文
 * 设置初始化线程返回地址寄存器ra，栈寄存器sp
 * 
 * 这个函数传入参数为一个二进制的代码和一个线程指针(具体传入规则可以自己修改)
 * 此外程序首次进入用户态之前，应该设置好trap处理向量为usertrap（或者你自定义的）
 */
pagetable_t uvmcreate(){
    pagetable_t pagetable;
    pagetable=mm_kalloc();
    if(pagetable==0)
        return 0;
    memset(pagetable,0,PGSIZE);
    return pagetable;
}

pagetable_t proc_pagetable(process_t *p){
    pagetable_t pagetable;
    pagetable=uvmcreate();
    if(pagetable==0)
        return 0;
    if(pt_map_pages(pagetable, TRAMPOLINE, (uint64)trampoline, PGSIZE, PTE_R|PTE_X)<0)
        BUG("proc_pagetable");
    if(pt_map_pages(pagetable, TRAPFRAME, (uint64)(p->thread->trapframe), PGSIZE, PTE_R|PTE_W)<0)
        BUG("proc_pagetable");
    return pagetable;
}

process_t *alloc_proc(const char* bin, thread_t *thr){
    thr = thre;
    process_t *p;
    p=proc;
    //todo
    lock_init(&(thr->lock_));
    //init_lock?
    thr->kstack= KSTACK(thr-thre);
    if(p->process_state==UNUSED){
        goto found;
    }
    BUG("alloc_proc");
    return NULL;

found:
    p->pid=alloc_pid();
    p->process_state=USED;
    thr->tid=alloc_tid();
    thr->thread_state=USED;
    if((thr->trapframe=mm_kalloc())==0)
        BUG("alloc_proc");
    p->pagetable= proc_pagetable(p);
    if(p->pagetable==0)
        BUG("alloc_proc");
    memset(&(thr->context), 0, sizeof(thr->context));
    //todo
    //thr->context.ra=??
    thr->context.sp=thr->kstack+PGSIZE;
    load_binary(&(p->pagetable),bin);
    p->thread=thr;
    return p;
}

bool load_thread(file_type_t type){
    if(type == PUTC){
        thread_t *t = NULL;
        process_t *p = alloc_proc(&binary_putc_start, t);
        if(!t) return false;
        sched_enqueue(t);
    } else {
        BUG("Not supported");
    }

}

// sched_enqueue和sched_dequeue的主要任务是加入一个任务到队列中和删除一个任务
// 这两个函数的展示了如何使用list.h中可的函数（加入、删除、判断空、取元素）
// 具体可以参考：Stackoverflow上的回答
// https://stackoverflow.com/questions/15832301/understanding-container-of-macro-in-the-linux-kernel
void sched_enqueue(thread_t *target_thread){
    if(target_thread->thread_state == RUNNING) BUG("Running Thread cannot be scheduled.");
    list_add(&target_thread->sched_list_thread_node, &(sched_list[cpuid()]));
}

thread_t *sched_dequeue(){
    if(list_empty(&(sched_list[cpuid()]))) BUG("Scheduler List is empty");
    thread_t *head = container_of(&(sched_list[cpuid()]), thread_t, sched_list_thread_node);
    list_del(&head->sched_list_thread_node);
    return head;
}

bool sched_empty(){
    return list_empty(&(sched_list[cpuid()]));
}

// 开始运行某个特定的函数
void thread_run(thread_t *target){
    cpu_t *c=mycpu();
    acquire(&(target->lock_));
    if(target->thread_state==RUNNABLE){
        target->thread_state=RUNNING;
        c->thread=target;
        swtch(&(c->context),&(target->context));
    }
}

// sched_start函数启动调度，按照调度的队列开始运行。
void sched_start(){
    while(1){
        if(sched_empty()) BUG("Scheduler list empty, no app loaded");
        thread_t *next = sched_dequeue();
        thread_run(next);
    }
}

void sched_init(){
    // 初始化调度队列锁
    lock_init(&schedlock);
    // 初始化队列头
    init_list_head(&(sched_list[cpuid()]));
}

void proc_init(){
    // 初始化pid、tid锁
    lock_init(&pidlock);
    lock_init(&tidlock);
    // 接下来代码期望的目的：映射第一个用户线程并且插入调度队列
    if(!load_thread(PUTC)) BUG("Load failed");
}

void sched(void){
    //todo
    int intena_backup;
    thread_t *t=mythre();
    if(!holding_lock(&(t->lock_)))
        BUG("sched");
    //todo
    if(t->process->process_state==RUNNING)
        BUG("sched");
    if(intr_get())
        BUG("sched");
    intena_backup=mycpu()->intena;
    swtch(&(t->context),&(mycpu()->context));
    mycpu()->intena=intena_backup;
}

void yield(void){
    thread_t *t=mythre();
    acquire(&t->lock_);
    t->process->process_state = RUNNABLE;
    sched();
    release(&t->lock_);
}

