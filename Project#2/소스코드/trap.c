#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "traps.h"
#include "spinlock.h"

extern void wakeup1(void *chan);

#define NQUEUE 4  //우선순위 큐 개수

// Interrupt descriptor table (shared by all CPUs).
struct gatedesc idt[256];
extern uint vectors[];  // in vectors.S: array of 256 entry pointers
struct spinlock tickslock;
uint ticks;

void
tvinit(void)
{
  int i;

  for(i = 0; i < 256; i++)
    SETGATE(idt[i], 0, SEG_KCODE<<3, vectors[i], 0);
  SETGATE(idt[T_SYSCALL], 1, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);

  initlock(&tickslock, "time");
}

void
idtinit(void)
{
  lidt(idt, sizeof(idt));
}

//PAGEBREAK: 41
void
trap(struct trapframe *tf)
{
  if(tf->trapno == T_SYSCALL){
    if(myproc()->killed)
      exit();
    myproc()->tf = tf;
    syscall();
    if(myproc()->killed)
      exit();
    return;
  }

  switch(tf->trapno){
  // 1 tick
  case T_IRQ0 + IRQ_TIMER:
    if(cpuid() == 0){
      acquire(&tickslock);
      ticks++;
      wakeup(&ticks);
      release(&tickslock);
    }
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE:
    ideintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE+1:
    // Bochs generates spurious IDE1 interrupts.
    break;
  case T_IRQ0 + IRQ_KBD:
    kbdintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_COM1:
    uartintr();
    lapiceoi();
    break;
  case T_IRQ0 + 7:
  case T_IRQ0 + IRQ_SPURIOUS:
    cprintf("cpu%d: spurious interrupt at %x:%x\n",
            cpuid(), tf->cs, tf->eip);
    lapiceoi();
    break;

  //PAGEBREAK: 13
  default:
    if(myproc() == 0 || (tf->cs&3) == 0){
      // In kernel, it must be our mistake.
      cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
              tf->trapno, cpuid(), tf->eip, rcr2());
      panic("trap");
    }
    // In user space, assume process misbehaved.
    cprintf("pid %d %s: trap %d err %d on cpu %d "
            "eip 0x%x addr 0x%x--kill proc\n",
            myproc()->pid, myproc()->name, tf->trapno,
            tf->err, cpuid(), tf->eip, rcr2());
    myproc()->killed = 1;
  }

  // Force process exit if it has been killed and is in user space.
  // (If it is still executing in the kernel, let it keep running
  // until it gets to the regular system call return.)
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();

  // Force process to give up CPU on clock tick.
  // If interrupts were on while locks held, would need to check nlock.
  if(myproc() && myproc()->state == RUNNING && tf->trapno == T_IRQ0+IRQ_TIMER)
  {
      struct proc *p = myproc();
      p->cpu_burst += 1;

      increment_io_wait_time(p);
      increment_cpu_wait_time(p);
      
      aging_processes();
      
      int time_quantum[NQUEUE] = {10, 20, 40, 80};  //각 큐의 Time Quantum

      // Terminate process if it reaches end_time
      if(p->total_ticks >= p->end_time && p->end_time != -1)
      {
        int ticks_used = p->cpu_burst;
        if (p->total_ticks + ticks_used >= p->end_time) {
          ticks_used = p->end_time - p->total_ticks;
        }
        #if defined(DEBUG) || defined(ANALYZE)
        if(p->pid > 2 && strncmp(p->name, "sh", 2))
        {
          cprintf("PID: %d used %d ticks. terminated\n", p->pid, p->end_time);
        }
        #endif

        exit();
      }
      
      // If ticks already passed time quantum -> print accordingly
      if(ticks - p->start_ticks >= time_quantum[p->q_level])
      {
        // Reset burst and track total ticks used
        int ticks_used = p->cpu_burst;
        if (p->total_ticks + ticks_used >= p->end_time && p->end_time != -1) {
          ticks_used = p->end_time - p->total_ticks;
          
          #ifdef DEBUG
          if(p->pid > 2 && strncmp(p->name, "sh", 2))
          {
            cprintf("PID: %d uses %d ticks in mlfq[%d], total(%d/%d)\n", p->pid, ticks_used, p->q_level, p->end_time, p->end_time);
            cprintf("PID: %d, used %d ticks. terminated\n", p->pid, p->end_time);
          }
          #endif

          exit();
        }
        p->total_ticks += ticks_used;
        
        #if defined(DEBUG) || defined(ANALYZE)
        if(p->pid > 2 && strncmp(p->name, "sh", 2))
        {
          cprintf("PID: %d uses %d ticks in mlfq[%d], total(%d/%d)\n", p->pid, ticks_used, p->q_level, p->total_ticks, p->end_time);
        }
        #endif

        // Remove from current queue
        remove_from_queue(p->q_level, p);

        // Move to the next lower queue (unless it's already in the lowest)
        if(p->q_level + 1 < NQUEUE)
        {
          p->cpu_wait = 0;
          p->io_wait_time = 0;
          add_to_queue(p->q_level + 1, p);
        }
        else
        {
          add_to_queue(p->q_level, p);
        }
        
        p->cpu_burst = 0;
        yield();
      }
  }
  
  // Check if the process has been killed since we yielded
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();
}
