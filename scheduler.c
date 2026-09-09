#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scheduler.h"

#define LINE_BUF 512
#define MSG_BUF  256

/* ======================================================================
 * PARSING
 * ====================================================================== */

/* 1 se a linha e vazia ou composta apenas de espacos em branco. */
static int is_blank(const char *line)
{
    for (; *line != '\0'; line++) {
        if (*line != ' ' && *line != '\t' && *line != '\n' && *line != '\r')
            return 0;
    }
    return 1;
}

/* Converte um token em inteiro positivo; encerra o programa em caso de erro. */
static int parse_positive_int(const char *tok, int line)
{
    char msg[MSG_BUF];
    char *end;
    long value;

    errno = 0;
    value = strtol(tok, &end, 10);

    if (end == tok || *end != '\0') {
        snprintf(msg, sizeof msg,
                 "erro: linha %d: valor nao numerico: '%s'", line, tok);
        fail(msg);
    }
    if (errno == ERANGE || value > INT_MAX || value < INT_MIN) {
        snprintf(msg, sizeof msg,
                 "erro: linha %d: valor fora do intervalo: '%s'", line, tok);
        fail(msg);
    }
    if (value <= 0) {
        snprintf(msg, sizeof msg,
                 "erro: linha %d: valor deve ser positivo: '%s'", line, tok);
        fail(msg);
    }
    return (int)value;
}

int parse_input(const char *path, Task *tasks, int *n_tasks, int *total_time)
{
    char msg[MSG_BUF];
    char line[LINE_BUF];
    FILE *f;
    int lineno = 0;
    int count = 0;
    int have_total = 0;

    f = fopen(path, "r");
    if (f == NULL) {
        snprintf(msg, sizeof msg,
                 "erro: nao foi possivel abrir o arquivo '%s'", path);
        fail(msg);
    }

    while (fgets(line, sizeof line, f) != NULL) {
        char *tok;
        char *fields[5];
        size_t len;
        int n_fields = 0;

        lineno++;

        if (strchr(line, '\n') == NULL && !feof(f)) {
            fclose(f);
            snprintf(msg, sizeof msg, "erro: linha %d: linha muito longa", lineno);
            fail(msg);
        }

        if (is_blank(line))
            continue;

        if (!have_total) {
            /* primeira linha util: tempo total da simulacao */
            tok = strtok(line, " \t\r\n");
            if (tok == NULL || strtok(NULL, " \t\r\n") != NULL) {
                fclose(f);
                snprintf(msg, sizeof msg,
                         "erro: linha %d: esperado apenas o tempo total da simulacao",
                         lineno);
                fail(msg);
            }
            *total_time = parse_positive_int(tok, lineno);
            have_total = 1;
            continue;
        }

        /* linha de tarefa: NOME PERIODO DEADLINE BURST */
        for (tok = strtok(line, " \t\r\n"); tok != NULL;
             tok = strtok(NULL, " \t\r\n")) {
            if (n_fields < 5)
                fields[n_fields] = tok;
            n_fields++;
        }

        if (n_fields != 4) {
            fclose(f);
            snprintf(msg, sizeof msg,
                     "erro: linha %d: esperados 4 campos, encontrados %d",
                     lineno, n_fields);
            fail(msg);
        }

        if (count >= MAX_TASKS) {
            fclose(f);
            snprintf(msg, sizeof msg,
                     "erro: linha %d: numero de tarefas excede o maximo (%d)",
                     lineno, MAX_TASKS);
            fail(msg);
        }

        len = strlen(fields[0]);
        if (len > (size_t)(MAX_NAME - 1)) {
            fclose(f);
            snprintf(msg, sizeof msg,
                     "erro: linha %d: nome da tarefa excede %d caracteres: '%s'",
                     lineno, MAX_NAME - 1, fields[0]);
            fail(msg);
        }

        memcpy(tasks[count].name, fields[0], len + 1);
        tasks[count].period   = parse_positive_int(fields[1], lineno);
        tasks[count].deadline = parse_positive_int(fields[2], lineno);
        tasks[count].burst    = parse_positive_int(fields[3], lineno);
        tasks[count].order    = count;
        count++;
    }

    if (ferror(f)) {
        fclose(f);
        snprintf(msg, sizeof msg, "erro: falha ao ler o arquivo '%s'", path);
        fail(msg);
    }
    fclose(f);

    if (!have_total) {
        snprintf(msg, sizeof msg,
                 "erro: arquivo '%s' vazio: falta o tempo total da simulacao", path);
        fail(msg);
    }
    if (count == 0) {
        snprintf(msg, sizeof msg,
                 "erro: arquivo '%s' nao contem nenhuma tarefa", path);
        fail(msg);
    }

    *n_tasks = count;
    return 0;
}

/* ======================================================================
 * VALIDACAO
 * ====================================================================== */

int validate_tasks(const Task *tasks, int n_tasks)
{
    char msg[MSG_BUF];
    int i;

    for (i = 0; i < n_tasks; i++) {
        if (tasks[i].deadline > tasks[i].period) {
            snprintf(msg, sizeof msg,
                     "erro: tarefa '%s': deadline (%d) maior que periodo (%d)",
                     tasks[i].name, tasks[i].deadline, tasks[i].period);
            fail(msg);
        }
        if (tasks[i].burst > tasks[i].deadline) {
            snprintf(msg, sizeof msg,
                     "erro: tarefa '%s': burst (%d) maior que deadline (%d)",
                     tasks[i].name, tasks[i].burst, tasks[i].deadline);
            fail(msg);
        }
    }
    return 0;
}

/* ======================================================================
 * POLITICA
 * ====================================================================== */

/* Selecao da proxima tarefa segundo rate-monotonic / EDF.
 *
 * Retorna 1 se a tarefa 'a' tem prioridade maior que a tarefa 'b'.
 * O desempate, em qualquer politica, e sempre pela ordem de aparicao no
 * arquivo de entrada: vence o menor campo 'order'. */
static int higher_priority(const Task *tasks, const TaskState *states,
                           int a, int b, Policy policy)
{
    switch (policy) {
    case POLICY_EDF:
        /* so ha deadline absoluto valido em instancia ativa; pick_task() so
         * compara tarefas ativas, mas nao dependemos disso aqui */
        if (states[a].active != states[b].active)
            return states[a].active;
        if (states[a].active && states[a].abs_deadline != states[b].abs_deadline)
            return states[a].abs_deadline < states[b].abs_deadline;
        break;

    case POLICY_RATE:
    default:
        if (tasks[a].period != tasks[b].period)
            return tasks[a].period < tasks[b].period;
        break;
    }

    return tasks[a].order < tasks[b].order;
}

/* Indice da tarefa pronta de maior prioridade, ou -1 se nenhuma esta ativa. */
static int pick_task(const Task *tasks, const TaskState *states, int n_tasks,
                     Policy policy)
{
    int best = -1;
    int i;

    for (i = 0; i < n_tasks; i++) {
        if (!states[i].active)
            continue;
        if (best < 0 || higher_priority(tasks, states, i, best, policy))
            best = i;
    }
    return best;
}

/* ======================================================================
 * TRACO
 * ====================================================================== */

/* Consolidacao dos segmentos de execucao.
 *
 * Um segmento permanece aberto enquanto a mesma tarefa (ou o ocioso) ocupa
 * unidades de tempo contiguas; so no instante em que ele encerra e que se
 * conhece a sua tag. 'open' vale SEG_NONE quando nao ha segmento aberto,
 * SEG_IDLE para um segmento ocioso, e o indice da tarefa caso contrario. */
#define SEG_NONE (-2)
#define SEG_IDLE (-1)

/* Fecha o segmento aberto com a tag informada e o acrescenta ao traco. */
static void close_segment(Segment *trace, int *n_segments, int *open,
                          int *length, char tag)
{
    if (*open == SEG_NONE)
        return;

    if (*n_segments >= MAX_SEGMENTS)
        fail("erro: traco de execucao excede o numero maximo de segmentos");

    trace[*n_segments].task   = *open;
    trace[*n_segments].length = *length;
    trace[*n_segments].tag    = (*open == SEG_IDLE) ? 0 : tag;
    (*n_segments)++;

    *open   = SEG_NONE;
    *length = 0;
}

/* ======================================================================
 * MOTOR DE SIMULACAO
 * ====================================================================== */

void run_simulation(const Task *tasks, int n_tasks, int total_time,
                    Policy policy, Segment *trace, int *n_segments,
                    TaskState *states)
{
    int open   = SEG_NONE;   /* segmento em construcao */
    int length = 0;          /* duracao acumulada do segmento aberto */
    int t;
    int i;

    for (i = 0; i < n_tasks; i++) {
        states[i].remaining    = 0;
        states[i].abs_deadline = 0;
        states[i].active       = 0;
        states[i].lost         = 0;
        states[i].complete     = 0;
        states[i].killed       = 0;
    }
    *n_segments = 0;

    for (t = 0; t < total_time; t++) {
        int chosen;

        /* 1. chegadas: a nova instancia substitui a anterior, se houver */
        for (i = 0; i < n_tasks; i++) {
            if (t % tasks[i].period == 0) {
                states[i].remaining    = tasks[i].burst;
                states[i].abs_deadline = t + tasks[i].deadline;
                states[i].active       = 1;
            }
        }

        /* 2. deadlines vencidos: a rajada restante e descartada e a tarefa
         *    so volta a concorrer na proxima chegada */
        for (i = 0; i < n_tasks; i++) {
            if (states[i].active && states[i].abs_deadline == t) {
                states[i].active = 0;
                states[i].lost++;
                if (open == i)
                    close_segment(trace, n_segments, &open, &length, 'L');
            }
        }

        /* 3. escolha refeita a cada tick: e o que implementa a preempcao */
        chosen = pick_task(tasks, states, n_tasks, policy);

        if (open != SEG_NONE && open != chosen)
            close_segment(trace, n_segments, &open, &length, 'H');

        if (open == SEG_NONE) {
            open   = chosen;   /* 5. chosen == -1 abre um segmento ocioso */
            length = 0;
        }
        length++;

        /* 4. execucao de uma unidade de tempo */
        if (chosen >= 0 && --states[chosen].remaining == 0) {
            states[chosen].active = 0;
            states[chosen].complete++;
            close_segment(trace, n_segments, &open, &length, 'F');
        }
    }

    /* o ultimo segmento nao encerra por nenhum dos tres eventos: fica sem tag */
    close_segment(trace, n_segments, &open, &length, 0);

    /* instancias ainda pendentes nao terminaram nem perderam o deadline */
    for (i = 0; i < n_tasks; i++) {
        if (states[i].active) {
            states[i].active = 0;
            states[i].killed++;
        }
    }
}

/* ======================================================================
 * RELATORIO
 * ====================================================================== */

/* Contador da tarefa correspondente a uma das tres secoes do relatorio. */
static int counter_of(const TaskState *state, int section)
{
    switch (section) {
    case 0:  return state->lost;
    case 1:  return state->complete;
    default: return state->killed;
    }
}

/* Escreve uma secao de contadores, na ordem de aparicao no arquivo de entrada. */
static void write_counters(FILE *f, const char *title, const Task *tasks,
                           int n_tasks, const TaskState *states, int section)
{
    int i;

    fprintf(f, "\n%s\n", title);
    for (i = 0; i < n_tasks; i++)
        fprintf(f, "[%s] %d\n", tasks[i].name, counter_of(&states[i], section));
}

int write_report(Policy policy, const Task *tasks, int n_tasks,
                 const Segment *trace, int n_segments, const TaskState *states)
{
    char msg[MSG_BUF];
    char path[MAX_NAME];
    FILE *f;
    int i;

    snprintf(path, sizeof path, "%s_%s.out",
             (policy == POLICY_EDF) ? "edf" : "rate", OUT_SUFFIX);

    f = fopen(path, "w");
    if (f == NULL) {
        snprintf(msg, sizeof msg,
                 "erro: nao foi possivel escrever o arquivo '%s'", path);
        fail(msg);
    }

    fprintf(f, "EXECUTION BY %s\n\n", (policy == POLICY_EDF) ? "EDF" : "RATE");

    for (i = 0; i < n_segments; i++) {
        if (trace[i].task < 0)
            fprintf(f, "idle for %d units\n", trace[i].length);
        else if (trace[i].tag != 0)
            fprintf(f, "[%s] for %d units - %c\n",
                    tasks[trace[i].task].name, trace[i].length, trace[i].tag);
        else
            fprintf(f, "[%s] for %d units\n",
                    tasks[trace[i].task].name, trace[i].length);
    }

    write_counters(f, "LOST DEADLINES", tasks, n_tasks, states, 0);
    write_counters(f, "COMPLETE EXECUTION", tasks, n_tasks, states, 1);
    write_counters(f, "KILLED", tasks, n_tasks, states, 2);

    if (fclose(f) != 0) {
        snprintf(msg, sizeof msg, "erro: falha ao escrever o arquivo '%s'", path);
        fail(msg);
    }
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

int main(int argc, char *argv[])
{
    char msg[MSG_BUF];
    Task tasks[MAX_TASKS];
    TaskState states[MAX_TASKS];
    static Segment trace[MAX_SEGMENTS];
    Policy policy;
    int n_tasks = 0;
    int total_time = 0;
    int n_segments = 0;

    if (argc != 3)
        fail("erro: uso: ./scheduler <rate|edf> <arquivo>");

    if (strcmp(argv[1], "rate") == 0) {
        policy = POLICY_RATE;
    } else if (strcmp(argv[1], "edf") == 0) {
        policy = POLICY_EDF;
    } else {
        snprintf(msg, sizeof msg,
                 "erro: algoritmo invalido: '%s' (esperado 'rate' ou 'edf')",
                 argv[1]);
        fail(msg);
        return 1; /* nao alcancado: fail() encerra o programa */
    }

    parse_input(argv[2], tasks, &n_tasks, &total_time);
    validate_tasks(tasks, n_tasks);

    run_simulation(tasks, n_tasks, total_time, policy, trace, &n_segments,
                   states);
    write_report(policy, tasks, n_tasks, trace, n_segments, states);

    return 0;
}
