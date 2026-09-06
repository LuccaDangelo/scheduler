#ifndef SCHEDULER_H
#define SCHEDULER_H

#define MAX_TASKS    64
#define MAX_NAME     32
#define MAX_SEGMENTS 4096
#define OUT_SUFFIX   "lada"   /* iniciais do e-mail, usado em rate_lada.out / edf_lada.out */

typedef enum { POLICY_RATE, POLICY_EDF } Policy;

/* Tarefa como lida do arquivo (imutavel durante a simulacao) */
typedef struct {
    char name[MAX_NAME];
    int  period;
    int  deadline;
    int  burst;
    int  order;        /* posicao no arquivo, usada para desempate */
} Task;

/* Estado de execucao de uma tarefa durante a simulacao */
typedef struct {
    int remaining;      /* rajada restante da instancia atual */
    int abs_deadline;   /* deadline absoluto da instancia atual */
    int active;         /* 1 se ha instancia pendente, 0 caso contrario */
    int lost;           /* contador LOST DEADLINES */
    int complete;       /* contador COMPLETE EXECUTION */
    int killed;         /* contador KILLED */
} TaskState;

/* Segmento consolidado do traco de execucao */
typedef struct {
    int  task;   /* indice da tarefa, ou -1 para idle */
    int  length; /* duracao em unidades de tempo */
    char tag;    /* 'F' terminou, 'H' preemptada, 'L' perdeu deadline, 0 se idle */
} Segment;

int  parse_input(const char *path, Task *tasks, int *n_tasks, int *total_time);
int  validate_tasks(const Task *tasks, int n_tasks);
void run_simulation(const Task *tasks, int n_tasks, int total_time,
                    Policy policy, Segment *trace, int *n_segments,
                    TaskState *states);
int  write_report(Policy policy, const Task *tasks, int n_tasks,
                  const Segment *trace, int n_segments, const TaskState *states);
void fail(const char *msg);   /* escreve em stderr e encerra com codigo != 0 */

#endif /* SCHEDULER_H */
