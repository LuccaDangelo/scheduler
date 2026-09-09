#ifndef SCHEDULER_H
#define SCHEDULER_H

#define MAX_TASKS    64
#define MAX_NAME     32
#define MAX_SEGMENTS 4096
#define OUT_SUFFIX   "lada"

typedef enum { POLICY_RATE, POLICY_EDF } Policy;

typedef struct {
    char name[MAX_NAME];
    int  period;
    int  deadline;
    int  burst;
    int  order;
} Task;

typedef struct {
    int remaining;
    int abs_deadline;
    int active;
    int lost;
    int complete;
    int killed;
} TaskState;

typedef struct {
    int  task;
    int  length;
    char tag;
} Segment;

int  parse_input(const char *path, Task *tasks, int *n_tasks, int *total_time);
int  validate_tasks(const Task *tasks, int n_tasks);
void run_simulation(const Task *tasks, int n_tasks, int total_time,
                    Policy policy, Segment *trace, int *n_segments,
                    TaskState *states);
int  write_report(Policy policy, const Task *tasks, int n_tasks,
                  const Segment *trace, int n_segments, const TaskState *states);
void fail(const char *msg);

#endif
