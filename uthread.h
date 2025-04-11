#ifndef UTHREAD_H
#define UTHREAD_H

#include <ucontext.h>

// Constants
#define MAX_TASKS 10
#define STACK_SIZE 16384
#define TIME_SLICE_USEC 50000 // 500ms

// Implemented Schedulers
typedef enum
{
    SCHEDULER_ROUND_ROBIN,
    SCHEDULER_LOTTERY
} scheduler_type_t;

// Task structure
typedef struct
{
    ucontext_t context;
    char *stack;
    unsigned id;
    unsigned tickets;    // For lottery scheduling
    int task_terminated; // Boolean flag instead of enum state
} task_t;

// Scheduler structure - forward declaration
typedef struct scheduler_s scheduler_t;

// Scheduler Structure Definition
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

// External function declarations
void scheduler_init(scheduler_type_t type);
unsigned scheduler_create_task(void (*func)(void), unsigned tickets);
void scheduler_start(void);

// External function execution counters
extern int func1_count;
extern int func2_count;
extern int func3_count;
extern int func4_count;

#endif /* UTHREAD_H */