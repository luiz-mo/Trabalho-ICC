#ifndef SOLVER_H
#define SOLVER_H

#include "jacobiana.h"

/*
 * Resolve J*x = b para J tridiagonal por eliminação de Gauss com pivoteamento
 * parcial entre linhas adjacentes. O(n).
 * VERSÃO 3: calcula uma única divisão por linha (inv[i] = 1/pivô) e usa
 * multiplicações no fator de eliminação e na retrossubstituição.
 * J->d, J->c, J->du2, J->inv e b são modificados; J->a não é.
 * Retorna 0 em sucesso e -1 se encontrar pivô nulo/muito pequeno.
 */
int resolveTriDiag(TriDiag *J, double *restrict b, double *restrict x);

#endif
