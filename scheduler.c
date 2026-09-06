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

int main(int argc, char *argv[])
{
    char msg[MSG_BUF];
    Task tasks[MAX_TASKS];
    Policy policy;
    int n_tasks = 0;
    int total_time = 0;

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

    (void)policy;
    /* TODO: commits seguintes */

    return 0;
}
