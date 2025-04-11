#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <ucontext.h>

static unsigned ctx_id = 0;
static ucontext_t ctx[4];

void signal_action(int signum)
{
    printf("SWITCH\n");
    unsigned ctx_cur = ctx_id;
    unsigned ctx_next = (ctx_cur + 1) % 4;
    ctx_id = ctx_next;

    // Reset the timer for the next interval
    struct itimerval it;
    it.it_value.tv_sec = 0;
    it.it_value.tv_usec = 500000; // 500ms
    it.it_interval.tv_sec = 0;
    it.it_interval.tv_usec = 500000; // 500ms
    if (setitimer(ITIMER_VIRTUAL, &it, NULL) == -1)
    {
        perror("setitimer");
        exit(1);
    }
    if (swapcontext(&ctx[ctx_cur], &ctx[ctx_next]) == -1)
    {
        perror("swapcontext");
        exit(1);
    }
}

static void busy_work()
{
    volatile unsigned long dummy = 0;
    for (unsigned long i = 0; i < 1000000; i++)
    {
        dummy += i;
    }
}

static void func1(void)
{
    printf("func1: started\n");
    while (1)
    {
        printf("func1: tick\n");
        busy_work(); // Consume CPU time instead of sleeping
    }
}

static void func2(void)
{
    printf("func2: started\n");
    while (1)
    {
        printf("func2: tick\n");
        busy_work();
    }
}

static void func3(void)
{
    printf("func3: started\n");
    while (1)
    {
        printf("func3: tick\n");
        busy_work();
    }
}

static void func4(void)
{
    printf("func4: started\n");
    while (1)
    {
        printf("func4: tick\n");
        busy_work();
    }
}

int main()
{
    static const unsigned STACK_SIZE = 16384;
    char stack[4][STACK_SIZE];
    void (*funcs[4])() = {func1, func2, func3, func4};
    ucontext_t ctx_main;

    for (unsigned c = 0; c < 4; c++)
    {
        unsigned c_next = (c + 1) & 0x3;
        memset(&ctx[c], 0, sizeof(ctx[c]));
        if (getcontext(&ctx[c]) == -1)
        {
            perror("getcontext");
            return 1;
        }
        ctx[c].uc_stack.ss_sp = stack[c];
        ctx[c].uc_stack.ss_size = STACK_SIZE;
        ctx[c].uc_link = &ctx[c_next];
        makecontext(&ctx[c], funcs[c], 0);
    }

    // Set the signal handler for SIGVTALRM
    signal(SIGVTALRM, signal_action);

    // Configure the virtual timer
    struct itimerval it;
    it.it_value.tv_sec = 0;
    it.it_value.tv_usec = 500000; // 500ms
    it.it_interval.tv_sec = 0;
    it.it_interval.tv_usec = 500000; // 500ms
    if (setitimer(ITIMER_VIRTUAL, &it, NULL) == -1)
    {
        perror("setitimer");
        return 1;
    }

    swapcontext(&ctx_main, &ctx[0]);
    printf("Not gonna RUN\n");
    return 0;
}
