#include <stddef.h>
#ifndef JACOBIANA_H
#define JACOBIANA_H

/*
 * VERSÃO 2 (OTIMIZADA): matriz tridiagonal n x n em quatro vetores
 * contíguos e alinhados em 64 bytes (linha de cache / AVX-512):
 *   a[i] = J[i+1][i]   (subdiagonal,   tamanho n-1)  -- CONSTANTE = -1
 *   d[i] = J[i][i]     (diagonal,      tamanho n)
 *   c[i] = J[i][i+1]   (superdiagonal, tamanho n-1)
 *   du2[i]             (2a superdiagonal criada pelo pivoteamento, tam. n)
 *
 * O solver NUNCA modifica a[]: por isso ela é preenchida uma única vez, na
 * alocação. d[] e c[] são reescritos a cada iteração de Newton.
 */
typedef struct {
    int n;
    double *a;
    double *d;
    double *c;
    double *du2;
} TriDiag;

/* Aloca um vetor de n doubles alinhado em 64 bytes (NULL em erro). */
double *alocaAlinhado(size_t n);

/* Aloca e inicializa a matriz (retorna 0 em sucesso, -1 em erro). */
int alocaTriDiag(TriDiag *J, int n);

/* Libera a memória da matriz tridiagonal. */
void liberaTriDiag(TriDiag *J);

/* Atualiza d[] e c[] com a Jacobiana de Broyden em x. */
void calculaJacobiana(const double *restrict x, TriDiag *J);

#endif
