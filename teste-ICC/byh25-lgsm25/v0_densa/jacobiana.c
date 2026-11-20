#include <stdlib.h>
#include "jacobiana.h"

/*
 * alocaDensa: aloca o vetor de ponteiros e, separadamente, cada linha.
 * (Layout propositalmente ingênuo: é a versão base de comparação.)
 */
int alocaDensa(Densa *J, int n)
{
    J->n = n;
    J->A = (double **)calloc((size_t)n, sizeof(double *));
    if (!J->A)
        return -1;

    for (int i = 0; i < n; ++i) {
        J->A[i] = (double *)malloc((size_t)n * sizeof(double));
        if (!J->A[i]) {
            liberaDensa(J);
            return -1;
        }
    }
    return 0;
}

/*
 * liberaDensa: libera cada linha e o vetor de ponteiros.
 */
void liberaDensa(Densa *J)
{
    if (J->A) {
        for (int i = 0; i < J->n; ++i)
            free(J->A[i]);
        free(J->A);
    }
    J->A = NULL;
}

/*
 * calculaJacobiana: preenche J com as derivadas parciais analíticas de F:
 *   df_i/dx_{i-1} = -1, df_i/dx_i = -4 x_i + 3, df_i/dx_{i+1} = -2.
 * Todos os n^2 elementos são reescritos a cada chamada.
 */
void calculaJacobiana(const double *x, Densa *J)
{
    const int n = J->n;

    for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j)
            J->A[i][j] = 0.0;

    for (int i = 0; i < n; ++i) {
        J->A[i][i] = -4.0 * x[i] + 3.0;
        if (i > 0)     J->A[i][i - 1] = -1.0;
        if (i < n - 1) J->A[i][i + 1] = -2.0;
    }
}
