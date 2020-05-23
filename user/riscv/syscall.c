#include "syscall.h"

static aux_t *aux;

typedef struct
{
    uint8_t *buff;
    size_t cap;
    uintptr_t pos;
} stream_t;

stream_t files[NFILES];

int main(int argc, char **argv, char **envp);

void init(int argc, char **argv, char **envp, aux_t *auxp)
{
    aux = auxp;

    files[STDIN_FILENO].buff = aux->stdin_buff;
    files[STDIN_FILENO].cap = aux->stdin_len;
    files[STDIN_FILENO].pos = 0;
    files[STDOUT_FILENO].buff = aux->stdout_buff;
    files[STDOUT_FILENO].cap = aux->stdout_cap;
    files[STDOUT_FILENO].pos = 0;
    files[STDERR_FILENO].buff = NULL;
    files[STDERR_FILENO].cap = 0;
    files[STDERR_FILENO].pos = 0;

    // FIXME test:
    aux->stdout_buff[0] = aux->stdin_buff[0];
    aux->stdout_buff[1] = aux->stdin_buff[1];
    aux->stdout_buff[2] = aux->stdin_buff[2];
    files[STDOUT_FILENO].pos = 3;

    int ret = main(argc, argv, envp);

    aux->stdout_len = files[STDOUT_FILENO].pos;

    exit(ret);
}
