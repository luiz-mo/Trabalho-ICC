#ifndef UTILS_H
#define UTILS_H
 
/*
 * Módulo utilitário: medição de tempo e macros de instrumentação LIKWID.
 *
 * Quando compilado com -DLIKWID_PERFMON, as macros LIKWID_MARKER_* vêm da
 * biblioteca do LIKWID. Caso contrário, viram "no-ops" e o programa compila
 * normalmente sem depender do LIKWID.
 */
 
#ifdef LIKWID_PERFMON
#include <likwid.h>
#else
#define LIKWID_MARKER_INIT
#define LIKWID_MARKER_THREADINIT
#define LIKWID_MARKER_REGISTER(regionTag)
#define LIKWID_MARKER_START(regionTag)
#define LIKWID_MARKER_STOP(regionTag)
#define LIKWID_MARKER_CLOSE
#endif
 
/* Retorna o instante atual (wall-clock) em MILISSEGUNDOS. */
double timestamp(void);
 
#endif