#define _GNU_SOURCE
#include <stdio.h>
#include <ucontext.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

// Constants
#define MAX_TASKS 10
#define STACK_SIZE 16384
#define TIME_SLICE_USEC 50000 // 50ms

// Implemented Schedulers
typedef enum
{
    SCHEDULER_ROUND_ROBIN,
    SCHEDULER_LOTTERY
} scheduler_type_t;

/* Task structure */
typedef struct
{
    ucontext_t context;
    char *stack;
    unsigned id;
    unsigned tickets;    // For lottery scheduling
    int task_terminated; // Boolean flag instead of enum state
} task_t;

/* Scheduler structure - forward declaration */
typedef struct scheduler_s scheduler_t;

/* Scheduler structure definition */
struct scheduler_s
{
    task_t *tasks[MAX_TASKS];
    unsigned task_count;
    unsigned current_task;
    scheduler_type_t type;
    unsigned active_tasks; // Track number of active tasks

    // Function pointer for selecting next task based on scheduling algorithm
    unsigned (*select_next_task)(scheduler_t *);
};

/* Global scheduler */
static scheduler_t scheduler;
static ucontext_t main_context;

// Function execution counters
static int func1_count = 0;
static int func2_count = 0;
static int func3_count = 0;
static int func4_count = 0;

/* Scheduling algorithm implementations */
unsigned round_robin_select(scheduler_t *sched)
{
    unsigned next = (sched->current_task + 1) % sched->task_count;
    unsigned initial_next = next;

    // Find the next non-terminated task
    while (sched->tasks[next]->task_terminated)
    {
        next = (next + 1) % sched->task_count;
        // If we've checked all tasks and come back to where we started,
        // all tasks are terminated
        if (next == initial_next)
        {
            break;
        }
    }

    // If task has tickets, decrement them
    if (!sched->tasks[next]->task_terminated)
    {
        sched->tasks[next]->tickets--;

        // If tickets reach zero, mark task as terminated
        if (sched->tasks[next]->tickets == 0)
        {
            printf("Task %u has run out of tickets, terminating\n", sched->tasks[next]->id);
            sched->tasks[next]->task_terminated = 1;
            sched->active_tasks--;
        }
    }

    return next;
}

unsigned lottery_select(scheduler_t *sched)
{
    // Lottery scheduling implementation
    unsigned total_tickets = 0;

    // Count total tickets of ready tasks
    for (unsigned i = 0; i < sched->task_count; i++)
    {
        if (!sched->tasks[i]->task_terminated)
        {
            total_tickets += sched->tasks[i]->tickets;
        }
    }

    if (total_tickets == 0)
    {
        // No tickets left, fall back to round-robin but just to find next available task
        return round_robin_select(sched);
    }

    // Draw a winning ticket
    unsigned winning_ticket = rand() % total_tickets;
    unsigned ticket_counter = 0;

    // Find the task with the winning ticket
    for (unsigned i = 0; i < sched->task_count; i++)
    {
        if (!sched->tasks[i]->task_terminated)
        {
            ticket_counter += sched->tasks[i]->tickets;
            if (ticket_counter > winning_ticket)
            {
                // Decrement ticket for the selected task
                sched->tasks[i]->tickets--;

                // If tickets reach zero, mark task as terminated
                if (sched->tasks[i]->tickets == 0)
                {
                    printf("Task %u has run out of tickets, terminating\n", sched->tasks[i]->id);
                    sched->tasks[i]->task_terminated = 1;
                    sched->active_tasks--;
                }

                return i;
            }
        }
    }

    // Fallback
    return round_robin_select(sched);
}

/* Signal handler for task switching */
void scheduler_tick(int signum)
{
    printf("SWITCHING CONTEXT\n");

    // Check if all tasks have terminated
    if (scheduler.active_tasks == 0)
    {
        printf("\n--- All tasks have terminated ---\n");
        printf("Function execution summary:\n");
        printf("func1 executed %d times\n", func1_count);
        printf("func2 executed %d times\n", func2_count);
        printf("func3 executed %d times\n", func3_count);
        printf("func4 executed %d times\n", func4_count);
        exit(0);
    }

    // Save context of current task
    task_t *current = scheduler.tasks[scheduler.current_task];

    // Get next task using scheduler's selection function
    unsigned next_task = scheduler.select_next_task(&scheduler);

    // If next task is terminated, try to find another one
    if (scheduler.tasks[next_task]->task_terminated)
    {
        // We already tried to find a non-terminated task in select_next_task,
        // if we're here, it means all tasks are terminated
        printf("\n--- All tasks have terminated ---\n");
        printf("Function execution summary:\n");
        printf("func1 executed %d times\n", func1_count);
        printf("func2 executed %d times\n", func2_count);
        printf("func3 executed %d times\n", func3_count);
        printf("func4 executed %d times\n", func4_count);
        exit(0);
    }

    // Update scheduler state
    scheduler.current_task = next_task;

    // Reset the timer for the next interval
    struct itimerval it;
    it.it_value.tv_sec = 0;
    it.it_value.tv_usec = TIME_SLICE_USEC;
    it.it_interval.tv_sec = 0;
    it.it_interval.tv_usec = TIME_SLICE_USEC;
    setitimer(ITIMER_VIRTUAL, &it, NULL);

    // Switch to next task
    if (swapcontext(&current->context, &scheduler.tasks[next_task]->context) == -1)
    {
        perror("swapcontext");
        exit(1);
    }
}

/* Initialize scheduler */
void scheduler_init(scheduler_type_t type)
{
    memset(&scheduler, 0, sizeof(scheduler));
    scheduler.task_count = 0;
    scheduler.current_task = 0;
    scheduler.type = type;
    scheduler.active_tasks = 0;

    // Set the appropriate scheduling function based on type
    switch (type)
    {
    case SCHEDULER_ROUND_ROBIN:
        scheduler.select_next_task = round_robin_select;
        break;
    case SCHEDULER_LOTTERY:
        scheduler.select_next_task = lottery_select;
        srand(time(NULL)); // Initialize random seed for lottery scheduling
        break;
    default:
        scheduler.select_next_task = round_robin_select; // Default to round-robin
    }

    // Set up the timer signal
    signal(SIGVTALRM, scheduler_tick);
}

/* Create a new task */
unsigned scheduler_create_task(void (*func)(void), unsigned tickets)
{
    if (scheduler.task_count >= MAX_TASKS)
    {
        fprintf(stderr, "Maximum number of tasks reached\n");
        return -1;
    }

    // Allocate task structure
    task_t *task = (task_t *)malloc(sizeof(task_t));
    if (!task)
    {
        perror("malloc");
        return -1;
    }

    // Initialize task
    task->id = scheduler.task_count;
    task->task_terminated = 0; // Not terminated initially
    task->tickets = tickets;

    // Allocate stack
    task->stack = (char *)malloc(STACK_SIZE);
    if (!task->stack)
    {
        perror("malloc");
        free(task);
        return -1;
    }

    // Initialize context
    if (getcontext(&task->context) == -1)
    {
        perror("getcontext");
        free(task->stack);
        free(task);
        return -1;
    }

    // Set up the context
    task->context.uc_stack.ss_sp = task->stack;
    task->context.uc_stack.ss_size = STACK_SIZE;
    task->context.uc_link = &main_context; // Return to main context when done

    // Make the context
    makecontext(&task->context, func, 0);

    // Add to scheduler
    scheduler.tasks[scheduler.task_count] = task;
    scheduler.task_count++;
    scheduler.active_tasks++;

    return task->id;
}

/* Start the scheduler */
void scheduler_start()
{
    if (scheduler.task_count == 0)
    {
        fprintf(stderr, "No tasks to schedule\n");
        return;
    }

    // Set up the timer
    struct itimerval it;
    it.it_value.tv_sec = 0;
    it.it_value.tv_usec = TIME_SLICE_USEC;
    it.it_interval.tv_sec = 0;
    it.it_interval.tv_usec = TIME_SLICE_USEC;
    setitimer(ITIMER_VIRTUAL, &it, NULL);

    // Start execution with the first task
    if (swapcontext(&main_context, &scheduler.tasks[0]->context) == -1)
    {
        perror("swapcontext");
        exit(1);
    }

    printf("Scheduler finished\n");
}

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
    printf("Task 0 (func1): %u tickets\n", scheduler.tasks[0]->tickets);
    printf("Task 1 (func2): %u tickets\n", scheduler.tasks[1]->tickets);
    printf("Task 2 (func3): %u tickets\n", scheduler.tasks[2]->tickets);
    printf("Task 3 (func4): %u tickets\n", scheduler.tasks[3]->tickets);
    printf("\nStarting scheduler...\n\n");

    // Start the scheduler
    scheduler_start();

    printf("Not gonna RUN\n");
    return 0;
}