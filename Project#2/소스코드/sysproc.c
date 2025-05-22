#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

int sys_set_proc_info(void) {
    int q_level, cpu_burst, cpu_wait_time, io_wait_time, end_time;

    //cprintf("Set process info start\n");

    if (argint(0, &q_level) < 0 ||
        argint(1, &cpu_burst) < 0 ||
        argint(2, &cpu_wait_time) < 0 ||
        argint(3, &io_wait_time) < 0 ||
        argint(4, &end_time) < 0)
        return -1;

    struct proc *p = myproc();
    
    if(p)
    {
      // Remove process from its current queue
      remove_from_queue(p->q_level, p);

      // Set new process parameters
      p->q_level = q_level;
      p->end_time = end_time;  // Ensure the process has the correct end_time
      p->cpu_burst = cpu_burst;    // Init CPU burst time  
      p->cpu_wait = cpu_wait_time;     // Init CPU wait time
      p->io_wait_time = io_wait_time; // Init I/O wait time
      p->total_ticks = 0;  // Init total ticks used by the process

      // Add process to the correct queue
      add_to_queue(p->q_level, p); 
      
      #ifdef DEBUG
      if(p->pid > 2 && strncmp(p->name, "sh", 2))
      {
        cprintf("Set process %d's info complete\n", p->pid);
      }
      #endif
    }

    return 0;
}


