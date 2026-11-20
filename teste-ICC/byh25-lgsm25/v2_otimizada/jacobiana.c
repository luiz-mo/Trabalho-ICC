#include <stdlib.h>
#include <string.h>
#include "jacobiana.h"

/*
 * alocaAlinhado: aligned_alloc exige tamanho múltiplo do alinhamento,
 * então o tamanho em bytes é arredondado para cima a múltiplos de 64.
 */
double *alocaAlinhado(size_t n)
{
    size_t bytes = (n * sizeof(double) + 63u) & ~(size_t)63u;
    if (bytes == 0) bytes = 64;
    return (double *)aligned_alloc(64, bytes);
}

/*
 * alocaTriDiag: aloca os 4 vetores alinhados e preenche a subdiagonal
 * constante (-1) uma única vez.
 */
int alocaTriDiag(TriDiag *J, int n)
{
    J->n = n;
    J->a   = alocaAlinhado((size_t)n);
    J->d   = alocaAlinhado((size_t)n);
    J->c   = alocaAlinhado((size_t)n);
    J->du2 = alocaAlinhado((size_t)n);

    if (!J->a || !J->d || !J->c || !J->du2) {
        liberaTriDiag(J);
        return -1;
    }

    for (int i = 0; i < n; ++i)
        J->a[i] = -1.0;
    memset(J->du2, 0, (size_t)n * sizeof(double));
    return 0;
}

/*
 * liberaTriDiag: libera a memória alocada por alocaTriDiag.
 */
void liberaTriDiag(TriDiag *J)
{
    free(J->a);
    free(J->d);
    free(J->c);
    free(J->du2);
    J->a = J->d = J->c = J->du2 = NULL;
}

/*
 * calculaJacobiana: dois laços simples (vetorizáveis, passo unitário):
 *   d[i] = -4 x_i + 3  (i = 0..n-1)
 *   c[i] = -2          (i = 0..n-2)   [reescrito pois o solver o altera]
 */
void calculaJacobiana(const double *restrict x, TriDiag *J)
{
    const int n = J->n;
    double *restrict d = J->d;
    double *restrict c = J->c;

    for (int i = 0; i < n; ++i)
        d[i] = 3.0 - 4.0 * x[i];

    for (int i = 0; i < n - 1; ++i)
        c[i] = -2.0;
}
