#include "uthread.h"
#include <stdio.h>

/* Task functions */
static void func1(void)
{
    printf("func1: started\n");
    while (1)
    {
        printf("func1: tick: %d\n", ++func1_count);
    }
}

static void func2(void)
{
    printf("func2: started\n");
    while (1)
    {
        printf("func2: tick: %d\n", ++func2_count);
    }
}

static void func3(void)
{
    printf("func3: started\n");
    while (1)
    {
        printf("func3: tick: %d\n", ++func3_count);
    }
}

static void func4(void)
{
    printf("func4: started\n");
    while (1)
    {
        printf("func4: tick: %d\n", ++func4_count);
    }
}

/* Main function */
int main()
{
    // Choose scheduling type (can be changed to try different schedulers)
    scheduler_type_t sched_type = SCHEDULER_LOTTERY; // or SCHEDULER_ROUND_ROBIN

    // Initialize scheduler
    scheduler_init(sched_type);

    printf("Using scheduler type: %s\n",
           sched_type == SCHEDULER_ROUND_ROBIN ? "Round Robin" : "Lottery");

    // Create tasks with different ticket amounts
    scheduler_create_task(func1, 50);
    scheduler_create_task(func2, 100);
    scheduler_create_task(func3, 200);
    scheduler_create_task(func4, 400);

    // Print initial ticket allocation
    printf("Initial ticket allocation:\n");
    for (int i = 0; i < 4; i++)
    {
        printf("Task %d (func%d): %u tickets\n", i, i + 1,
               (i == 0) ? 50 : (i == 1) ? 100
                           : (i == 2)   ? 200
                                        : 400);
    }

    printf("\nStarting scheduler...\n\n");

    // Start the scheduler
    scheduler_start();

    printf("Not gonna RUN\n");
    return 0;
}