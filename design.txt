# User Level Threads

User level threads are implemented in the form of round robin and lottery scheduler. This program manages threads entirely on the user space using ucontext for context switching and timer signals for preemption. This program includes a main module, where all threads (4 taken) are declared. Then, these 4 functions are scheduled based on the selected scheduler.

Threads and tasks are named interchangabily used through out the program.

The core of the program `uthread.h` has 4 major function implementation  
    -`scheduler_init`: It initializes the global scheduler struct and as well as other variables like task_count, current_task, type and active_tasks. This function also decides the function for selecting the next context based on the scheduler selected. Upon selecting round-robin, the `select_next_task` will be assigned value from `round_robin_select` and for lottery it will be selected from `lottery_select`.
    - `scheduler_create_task`: It creates a new thread based on the number of the tickets assigned. (⚠️ Even though tickets are still passed for round robin, only the sum of all the tickets is used to terminate the program after ticket runs out. Single ticket is used/deducted everytime it runs a thread.). For a specifc thread (task), contexts are set and task_id is returned.
    - `scheduler_start`: This functions actually sets the timer for swap and swaps the context from main context to the first thread (task).
    -`round_robin_select`: This function is used to select the next thread(task) scheduled using round robin scheduler.
    -`lottery_select`: This function return the index of the next task context scheduled using lottery scheduler.

## How to run?

1. Simply run `make` on the root of the project. It should generate `main` file in the root of the projet.

2. Then run `make run` on the root of the project to actually run the project.

(In the main function, we use sched_type to decide which scheduler to run. If we replace the SCHEDULER_LOTTERY with SCHEDULER_ROUND_ROBIN in the source code, it should use the code and generate runtimes for both input)


## Schedulers

### Round Robin Scheduling
It preempts threads by assigning all the threads equal time slice.

### Lottery Scheduling
It schedules the threads based on randomly selecting tickets.

## Key Design Decisions

Ticket as Run Count: Both schedulers treat tickets as a fixed count of allowed runs, not as a proportional-share resource. Each selection decrements the task’s tickets, ensuring tasks terminate after exactly N runs.

## Testing







