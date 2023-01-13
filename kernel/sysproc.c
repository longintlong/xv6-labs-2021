#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
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

uint64
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
  backtrace();
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64 sys_sigreturn(void) {
  struct proc *p = myproc();
  p->tickelapsed = 0;
  if(p->alarminterval != 0)
    p->alarminterval = -p->alarminterval;
  uint64* start = (uint64* )&(p->alarm_trapframe);
  for(int i = 0; i < (sizeof(struct trapframe) / sizeof(uint64)); ++i) {
    if(i == 0 || i == 1 || i == 2 || i == 4)
      continue;
    ((uint64*)p->trapframe)[i] = start[i];
  }
  return 0;
}

uint64 sys_sigalarm(void) {
  int n;
  uint64 func_ptr;

  if(argint(0, &n) < 0)
    return -1;
  if(argaddr(1, &func_ptr) < 0)
    return -1;
  struct proc *p = myproc();
  if(n == 0 && func_ptr == 0) {
    // sigalarm(0, 0) means the kernel should stop 
    // generating periodic alarm calls.
    p->alarminterval = 0;
    p->handler = 0;
  } else {
    p->alarminterval = n;
    p->handler = (void(*)())func_ptr;
  }

  return 0;
}