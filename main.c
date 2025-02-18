#define _GNU_SOURCE

#include <stdio.h>
#include <ucontext.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

static unsigned ctx_id = 0;
static ucontext_t ctx[4];

struct itimerval it;

void signal_action()
{
    printf("SWITCH\n");
    unsigned ctx_cur = ctx_id;
    unsigned ctx_next = (ctx_cur + 1) & 0x03;
    ctx_id = (ctx_cur + 1) & 0x03;
    if (swapcontext(&ctx[ctx_cur], &ctx[ctx_next]) == -1)
    {
        perror("swapcontext");
        exit(1);
    }
}

static void func1(void)
{
    printf("func1: started\n");
    while (1)
    {
        printf("func1: tick\n");
        usleep(100000);
    }
}

static void func2(void)
{
    printf("func2: started\n");
    while (1)
    {
        printf("func2: tick\n");
        usleep(100000);
    }
}

static void func3(void)
{
    printf("func3: started\n");
    while (1)
    {
        printf("func3: tick\n");
        usleep(100000);
    }
}

static void func4(void)
{
    printf("func4: started\n");
    while (1)
    {
        printf("func4: tick\n");
        usleep(100000);
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

    // struct sigaction action;
    // memset(&action, 0, sizeof(action));
    // sigemptyset(&(action.sa_mask));
    // action.sa_sigaction = signal_action;
    // action.sa_flags = SA_SIGINFO;
    // sigaction(SIGALRM, &action, NULL);

    signal(SIGVTALRM, signal_action);

    // struct itimerval tv;
    // tv.it_value.tv_sec = 0;
    // tv.it_value.tv_usec = 0;
    // tv.it_interval.tv_sec = 1;
    // tv.it_interval.tv_usec = 0;
    // setitimer(ITIMER_REAL, &tv, NULL);

    // Setting the timer to 500ms
    it.it_value.tv_sec = 1;
    it.it_value.tv_usec = 500000; // 500ms

    setitimer(ITIMER_VIRTUAL, &it, NULL);

    swapcontext(&ctx_main, &ctx[0]);

    printf("Not gonna RUN\n");

    return 0;
}