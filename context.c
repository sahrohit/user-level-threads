#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <errno.h>

#define STACK_SIZE SIGSTKSZ

#define N 5

int which = 0;
ucontext_t contexts[N];

typedef struct stack
{
  char mem[STACK_SIZE];
} stack;

void func()
{
  fprintf(stderr, "in func()\n");
}

int main()
{
  stack *stacks = (stack *)(malloc(STACK_SIZE * N));
  ucontext_t parent;
  for (int i = 0; i < N; i++)
  {
    getcontext(&(contexts[i]));
    contexts[i].uc_link = &parent;
    contexts[i].uc_stack.ss_sp = &(stacks[i]);
    contexts[i].uc_stack.ss_size = STACK_SIZE;
    contexts[i].uc_stack.ss_flags = 0;

    makecontext(&(contexts[i]), func, 0);
    fprintf(stderr, "made context %d\n", i);
  }

  for (int i = 0; i < N; i++)
  {
    printf("%d\n", i);
    swapcontext(&parent, &(contexts[i]));
  }
  return 0;
}
