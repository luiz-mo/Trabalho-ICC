#ifndef SOLVER_H
#define SOLVER_H

#include "jacobiana.h"

/*
 * Resolve J*x = b (J densa) por Eliminação de Gauss com pivoteamento parcial
 * e retrossubstituição. Custo O(n^3). J e b são modificados (as trocas de
 * linhas são feitas trocando ponteiros de linha).
 * Retorna 0 em sucesso e -1 se encontrar pivô nulo/muito pequeno ou
 * resultado não finito.
 */
int resolveDensa(Densa *J, double *b, double *x);

#endif
