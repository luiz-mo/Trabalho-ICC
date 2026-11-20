#ifndef SOLVER_H
#define SOLVER_H

#include "jacobiana.h"

/*
 * Resolve J*x = b para J tridiagonal (Jacobiana de Broyden) por eliminação
 * de Gauss com pivoteamento parcial entre linhas adjacentes. O(n).
 * J->d, J->c, J->du2 e b são modificados; J->a não é.
 * Retorna 0 em sucesso e -1 se encontrar pivô nulo/muito pequeno.
 * (Resultados não finitos são detectados pelo chamador.)
 */
int resolveTriDiag(TriDiag *J, double *restrict b, double *restrict x);

#endif
