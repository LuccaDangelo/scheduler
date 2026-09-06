#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scheduler.h"

/* ======================================================================
 * PARSING
 * ====================================================================== */

int parse_input(const char *path, Task *tasks, int *n_tasks, int *total_time)
{
    (void)path;
    (void)tasks;
    (void)n_tasks;
    (void)total_time;
    return 0;
}

/* ======================================================================
 * VALIDACAO
 * ====================================================================== */

int validate_tasks(const Task *tasks, int n_tasks)
{
    (void)tasks;
    (void)n_tasks;
    return 0;
}

/* ======================================================================
 * POLITICA
 * ====================================================================== */

/* Selecao da proxima tarefa segundo rate-monotonic / EDF. */

/* ======================================================================
 * MOTOR DE SIMULACAO
 * ====================================================================== */

void run_simulation(const Task *tasks, int n_tasks, int total_time,
                    Policy policy, Segment *trace, int *n_segments,
                    TaskState *states)
{
    (void)tasks;
    (void)n_tasks;
    (void)total_time;
    (void)policy;
    (void)trace;
    (void)n_segments;
    (void)states;
}

/* ======================================================================
 * TRACO
 * ====================================================================== */

/* Consolidacao dos segmentos de execucao. */

/* ======================================================================
 * RELATORIO
 * ====================================================================== */

int write_report(Policy policy, const Task *tasks, int n_tasks,
                 const Segment *trace, int n_segments, const TaskState *states)
{
    (void)policy;
    (void)tasks;
    (void)n_tasks;
    (void)trace;
    (void)n_segments;
    (void)states;
    return 0;
}

void fail(const char *msg)
{
    fprintf(stderr, "%s\n", msg);
    exit(1);
}

/* ======================================================================
 * MAIN
 * ====================================================================== */

int main(void)
{
    return 0;
}
