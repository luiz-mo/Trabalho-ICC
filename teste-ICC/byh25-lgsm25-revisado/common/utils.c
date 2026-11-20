#include <stdlib.h>
#include <time.h>
#include "utils.h"

/*
 * timestamp: retorna o tempo monotônico atual em milissegundos.
 * O tempo de um trecho de código é a diferença entre dois timestamps.
 */
double timestamp(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    return ((double)ts.tv_sec * 1000.0 +
            (double)ts.tv_nsec / 1000000.0);
}
