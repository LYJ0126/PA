#include <memory.h>

static void *pf = NULL;
//extern PGSIZE;
void* new_page(size_t nr_page) {
  //return NULL;
  pf += PGSIZE * nr_page;
  return pf;
}

#ifdef HAS_VME
static void* pg_alloc(int n) {
  //return NULL;
  assert(n % PGSIZE == 0);
  void *p = new_page(n / PGSIZE);
  void *head = p - n;
  memset(head, 0, n);
  return head;
}
#endif

void free_page(void *p) {
  panic("not implement yet");
}

/* The brk() system call handler. */
int mm_brk(uintptr_t brk) {
  return 0;
}

void init_mm() {
  pf = (void *)ROUNDUP(heap.start, PGSIZE);
  Log("free physical pages starting from %p", pf);

#ifdef HAS_VME
  vme_init(pg_alloc, free_page);
#endif
}
