#include "uthread.h"
#include <stdio.h>

// Task functions
static void func1(void)
{
    printf("FUNC 1: STARTED\n");
    while (1)
    {
        func1_count++;
        // printf("func1: tick: %d\n", ++func1_count);
    }
}

static void func2(void)
{
    printf("FUNC 2: STARTED\n");
    while (1)
    {
        func2_count++;
        // printf("func2: tick: %d\n", ++func2_count);
    }
}

static void func3(void)
{
    printf("FUNC 3: STARTED\n");
    while (1)
    {
        func3_count++;
        // printf("func3: tick: %d\n", func3_count);
    }
}

static void func4(void)
{
    printf("FUNC 4: STARTED\n");
    while (1)
    {
        func4_count++;
        // printf("func4: tick: %d\n", func4_count);
    }
}

int main()
{
    // Choose scheduling type (SCHEDULER_ROUND_ROBIN or SCHEDULER_LOTTERY)
    scheduler_type_t sched_type = SCHEDULER_ROUND_ROBIN;

    // Initialize scheduler
    scheduler_init(sched_type);

    printf("Using scheduler type: %s\n",
           sched_type == SCHEDULER_ROUND_ROBIN ? "Round Robin" : "Lottery");

    // Create tasks with different ticket amounts
    scheduler_create_task(func1, 10);
    scheduler_create_task(func2, 20);
    scheduler_create_task(func3, 30);
    scheduler_create_task(func4, 40);

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

    // Exits after printing results
    printf("Not gonna RUN\n");
    return 0;
}