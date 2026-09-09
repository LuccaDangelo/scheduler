# Simulador de escalonamento de tempo real

Simulador discreto, tick a tick, de escalonamento preemptivo de tarefas
periodicas, com duas politicas: rate-monotonic e earliest-deadline-first.

## Arquivos

- `scheduler.c` — todo o codigo do simulador, dividido em secoes:
  - **Parsing** (`parse_input`): le o arquivo de entrada, ignora linhas em
    branco, extrai o tempo total da simulacao e as tarefas, e converte os
    campos numericos rejeitando o que nao for inteiro positivo.
  - **Validacao** (`validate_tasks`): verifica as restricoes do modelo em cada
    tarefa, ou seja, deadline nao maior que o periodo e burst nao maior que o
    deadline.
  - **Politica** (`higher_priority`, `pick_task`): concentra a unica diferenca
    entre os dois algoritmos. `higher_priority` decide qual de duas tarefas tem
    prioridade maior — menor periodo no rate-monotonic, menor deadline absoluto
    no EDF — e, em caso de empate em qualquer politica, vence a que aparece
    primeiro no arquivo de entrada. `pick_task` percorre as tarefas prontas e
    devolve a de maior prioridade.
  - **Motor de simulacao** (`run_simulation`): avanca de t=0 ate o tempo total,
    uma unidade de tempo por iteracao. Em cada tick, nesta ordem: registra as
    chegadas das novas instancias, descarta as instancias cujo deadline venceu,
    escolhe a tarefa de maior prioridade e executa uma unidade dela. Como a
    escolha e refeita a cada tick, a preempcao acontece naturalmente. Mantem os
    contadores de deadlines perdidos, execucoes completas e tarefas mortas.
  - **Traco** (`close_segment`): consolida o resultado em segmentos. Unidades de
    tempo contiguas da mesma tarefa formam um unico segmento, assim como ticks
    ociosos consecutivos. Cada segmento e fechado com a marca que explica seu
    encerramento: `F` se a rajada terminou, `H` se houve preempcao, `L` se o
    deadline venceu.
  - **Relatorio** (`write_report`): grava o arquivo de saida com o traco e as
    tres secoes de contadores.
- `scheduler.h` — tipos (`Task`, `TaskState`, `Segment`, `Policy`), constantes de
  limite e prototipos das funcoes publicas.
- `Makefile` — compilacao do projeto.
- `voo.txt` — exemplo do enunciado, com as tarefas ATT e NAV.
- `comparativo.txt` — cenario em que as duas politicas divergem: no
  rate-monotonic a tarefa LENTA perde os dois deadlines, enquanto no EDF nenhuma
  tarefa perde deadline.

## Sistema operacional

Implementado e testado em macOS, com o clang que acompanha o Xcode. Compila sem
alteracao em Linux com gcc: o codigo usa apenas C11 padrao e a biblioteca C, sem
dependencias externas.

## Como compilar

```
make          # gera o executavel scheduler
make clean    # remove o executavel e os arquivos objeto
```

## Como executar

```
./scheduler rate voo.txt
./scheduler edf voo.txt
```

O primeiro argumento e a politica (`rate` ou `edf`) e o segundo e o arquivo de
entrada. O resultado e gravado em `rate_lada.out` ou `edf_lada.out`, conforme a
politica, no diretorio a partir do qual o programa foi executado. Nada e
impresso em stdout em nenhuma circunstancia; mensagens de erro vao para stderr.

## Como testar

Caminho feliz — as quatro execucoes terminam com codigo 0 e geram o arquivo de
saida correspondente:

```
./scheduler rate voo.txt
./scheduler edf voo.txt
./scheduler rate comparativo.txt
./scheduler edf comparativo.txt
```

Com `voo.txt`, o rate-monotonic perde um deadline da tarefa NAV e o EDF nao
perde nenhum. Com `comparativo.txt`, o rate-monotonic perde os dois deadlines da
tarefa LENTA e o EDF nao perde nenhum.

Casos de erro. Em todos eles o programa escreve uma mensagem em stderr, encerra
com codigo de saida 1 e nao cria nenhum arquivo de saida:

```
./scheduler                        # numero de argumentos incorreto
./scheduler xyz voo.txt            # algoritmo invalido
./scheduler rate inexistente.txt   # arquivo inexistente ou ilegivel
```

Os demais casos dependem do conteudo do arquivo de entrada. Para exercita-los,
crie um arquivo temporario com a linha de tarefa indicada:

```
ATT 20 12       # campo faltando: esperados 4 campos, encontrados 3
ATT 20 doze 8   # valor nao numerico
ATT 20 0 8      # valor nao positivo
ATT 20 30 8     # D > P: deadline maior que o periodo
ATT 20 12 15    # C > D: burst maior que o deadline
```

## Formato do arquivo de entrada

A primeira linha util contem o tempo total da simulacao. Cada linha seguinte
descreve uma tarefa, com quatro campos separados por espaco:

```
NOME PERIODO DEADLINE BURST
```

Todos os valores numericos devem ser inteiros positivos e vale a restricao
C <= D <= P, isto e, o burst nao pode ser maior que o deadline e o deadline nao
pode ser maior que o periodo. Linhas em branco sao ignoradas. Todas as tarefas
chegam pela primeira vez em t=0 e voltam a chegar a cada `PERIODO` unidades de
tempo; cada instancia tem deadline absoluto igual ao instante de chegada mais o
deadline relativo.

Exemplo (`voo.txt`):

```
100
ATT 20 12 8
NAV 50 30 15
```
