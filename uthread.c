#define _GNU_SOURCE
#include "uthread.h"
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

// Global Scheduler
static scheduler_t scheduler;
static ucontext_t main_context;

// Function execution counters
int func1_count = 0;
int func2_count = 0;
int func3_count = 0;
int func4_count = 0;

// Deciding the next task to run for Round Robin
static unsigned round_robin_select(scheduler_t *sched)
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

// Deciding the next task to run for Lottery
static unsigned lottery_select(scheduler_t *sched)
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
}

/* Signal handler for task switching */
static void scheduler_tick(int signum)
{
    // printf("SWITCHING CONTEXT\n");

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

// Initialize Scheduler
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

// Creating Threads based on the number of tickets
unsigned scheduler_create_task(void (*func)(void), unsigned tickets)
{
    // Creatiuon of a new thread
    task_t *task = (task_t *)malloc(sizeof(task_t));

    // Initialize task with default values
    task->id = scheduler.task_count;
    task->task_terminated = 0; // Not terminated initially
    task->tickets = tickets;   // Number of alloted tickets;

    // Allocate task to stack
    task->stack = (char *)malloc(STACK_SIZE);

    // Initialize context
    getcontext(&task->context);

    // Set up the context
    task->context.uc_stack.ss_sp = task->stack;
    task->context.uc_stack.ss_size = STACK_SIZE;
    // Return to main context when done
    task->context.uc_link = &main_context;

    // Make the context
    makecontext(&task->context, func, 0);

    // Add to scheduler
    scheduler.tasks[scheduler.task_count] = task;
    scheduler.task_count++;
    scheduler.active_tasks++;

    return task->id;
}

// Starting Scheduler
void scheduler_start()
{
    // Set up the timer
    struct itimerval it;
    it.it_value.tv_sec = 0;
    it.it_value.tv_usec = TIME_SLICE_USEC;
    it.it_interval.tv_sec = 0;
    it.it_interval.tv_usec = TIME_SLICE_USEC;
    setitimer(ITIMER_VIRTUAL, &it, NULL);

    // Swapping from main context to the first task
    swapcontext(&main_context, &scheduler.tasks[0]->context);
}