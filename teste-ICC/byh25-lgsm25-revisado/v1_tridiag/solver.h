#ifndef SOLVER_H
#define SOLVER_H

#include "jacobiana.h"

/*
 * Resolve J*x=b para J tridiagonal usando eliminação de Gauss com
 * pivoteamento. J e b são modificados durante a fatoração.
 * Retorna 0 em sucesso e -1 se encontrar pivô nulo/muito pequeno ou
 * resultado não finito.
 */
int resolveTriDiag(TriDiag *J, double *b, double *x);

#endif
