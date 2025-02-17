#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <errno.h>

// a system default specifying the number of bytes that would be used to cover the usual case when manually allocating an alternate stack area.
// #define STACK_SIZE SIGSTKSZ // Linter is throwing error for this line

// Alternative based from https://stackoverflow.com/questions/40299849/context-switching-is-makecontext-and-swapcontext-working-here-osx
#define STACK_SIZE (1 << 15) // 32KB

#define N 5

// Initializing an array of ucontext_t structures
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
  // Initializing an array of stack structures
  stack *stacks = (stack *)(malloc(STACK_SIZE * N));

  // Create a parent context to switch from and back to
  ucontext_t parent;

  for (int i = 0; i < N; i++)
  {
    // Get the user context and store it in context[i]
    getcontext(&(contexts[i]));

    // Pointer to the context that will be resumed when the context is terminated
    contexts[i].uc_link = &parent;

    contexts[i].uc_stack.ss_sp = &(stacks[i]);
    contexts[i].uc_stack.ss_size = STACK_SIZE;
    contexts[i].uc_stack.ss_flags = 0;

    // Make the context - pass the pointer to the function that needs to be executed
    // 0 - the third parameter is the number of arguments that the function takes
    makecontext(&(contexts[i]), func, 0);

    // Printing Stanard Error if any error occured
    fprintf(stderr, "made context %d\n", i);
  }

  for (int i = 0; i < N; i++)
  {
    printf("%d\n", i);

    // Switching from parent to context[i]
    swapcontext(&parent, &(contexts[i]));
  }

  return 0;
}
