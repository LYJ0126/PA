#include <am.h>
#include <nemu.h>
#include <klib.h>

static AddrSpace kas = {};
static void* (*pgalloc_usr)(int) = NULL;
static void (*pgfree_usr)(void*) = NULL;
static int vme_enable = 0;

static Area segments[] = {      // Kernel memory mappings
  NEMU_PADDR_SPACE
};

#define USER_SPACE RANGE(0x40000000, 0x80000000)

static inline void set_satp(void *pdir) {
  uintptr_t mode = 1ul << (__riscv_xlen - 1);
  asm volatile("csrw satp, %0" : : "r"(mode | ((uintptr_t)pdir >> 12)));
}

static inline uintptr_t get_satp() {
  uintptr_t satp;
  asm volatile("csrr %0, satp" : "=r"(satp));
  return satp << 12;
}

bool vme_init(void* (*pgalloc_f)(int), void (*pgfree_f)(void*)) {
  pgalloc_usr = pgalloc_f;
  pgfree_usr = pgfree_f;

  kas.ptr = pgalloc_f(PGSIZE);

  int i;
  for (i = 0; i < LENGTH(segments); i ++) {
    void *va = segments[i].start;
    for (; va < segments[i].end; va += PGSIZE) {
      map(&kas, va, va, 0);
    }
  }

  set_satp(kas.ptr);
  vme_enable = 1;

  return true;
}

void protect(AddrSpace *as) {
  PTE *updir = (PTE*)(pgalloc_usr(PGSIZE));
  as->ptr = updir;
  as->area = USER_SPACE;
  as->pgsize = PGSIZE;
  // map kernel space
  memcpy(updir, kas.ptr, PGSIZE);
}

void unprotect(AddrSpace *as) {
}

void __am_get_cur_as(Context *c) {
  c->pdir = (vme_enable ? (void *)get_satp() : NULL);
}

void __am_switch(Context *c) {
  if (vme_enable && c->pdir != NULL) {
    set_satp(c->pdir);
  }
}


void map(AddrSpace *as, void *va, void *pa, int prot) {
  uintptr_t vaddr = (uintptr_t)va;
  uintptr_t paddr = (uintptr_t)pa;
  uint32_t ppn  = (paddr >> 12) & 0xfffff;//物理页号
  uint32_t vpn1 = (vaddr >> 22) & 0x3ff;//一级页表索引
  uint32_t vpn0 = (vaddr >> 12) & 0x3ff;//二级页表索引
  PTE *pdirbase = (PTE *)as->ptr;//页目录基址
  PTE *ptablebase = &pdirbase[vpn1];//页表基址
  if(*ptablebase == 0) {//页表不存在
    PTE *tempptablebase = (PTE *)pgalloc_usr(PGSIZE);//分配一个页表
    *ptablebase = (uintptr_t)tempptablebase | PTE_V;//将页表的基地址写入对应页目录
    PTE *ptabletarget = &tempptablebase[vpn0];//页表项地址
    *ptabletarget = (ppn << 12) | PTE_X | PTE_W | PTE_R | PTE_V;//将页表项的内容写入页表(物理页号+权限)
  }
  else {//页表存在
    PTE *realptablebase = (PTE *)(*ptablebase & 0xfffff000);//页表基址
    PTE *ptabletarget = &realptablebase[vpn0];//页表项地址
    *ptabletarget = (ppn << 12) | PTE_X | PTE_W | PTE_R | PTE_V;//将页表项的内容写入页表(物理页号+权限)
  }
  
}

Context *ucontext(AddrSpace *as, Area kstack, void *entry) {
  //return NULL;
  Context *c = (Context *)((uint8_t* )kstack.end - sizeof(Context));//kstack.end是栈顶指针,分配一个Context结构体大小的空间
  memset(c, 0, sizeof(Context));//将Context结构体清零
  //将栈顶指针保存在Context记录的sp寄存器对应的位
  //c->gpr[2] = (uintptr_t)kstack.end;
  //设置中断状态
  c->mstatus = 0x1800 | 0x80;
  //设置用户进程入口
  c->mepc = (uintptr_t)entry;
  return c;
}
