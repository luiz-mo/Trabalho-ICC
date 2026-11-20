#include <stdlib.h>
#include <sys/time.h>
#include "utils.h"
 
/*
 * timestamp: retorna o tempo atual (gettimeofday) em milissegundos.
 * O tempo de um trecho de código é a diferença entre dois timestamps.
 */
double timestamp(void)
{
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return ((double)tp.tv_sec * 1000.0 + (double)tp.tv_usec / 1000.0);
}