
#include <stdlib.h>
#include "jacobiana.h"
 
/*
 * alocaTriDiag: aloca os três vetores que representam a matriz tridiagonal.
 */
int alocaTriDiag(TriDiag *J, int n)
{
    J->n = n;
    J->a = (double *)malloc((n - 1) * sizeof(double));
    J->d = (double *)malloc(n * sizeof(double));
    J->c = (double *)malloc((n - 1) * sizeof(double));
 
    if (!J->a || !J->d || !J->c) {
        liberaTriDiag(J);
        return -1;
    }
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
    J->a = J->d = J->c = NULL;
}
 
/*
 * calculaJacobiana: preenche J com as derivadas parciais analíticas de F em x.
 *
 *   df_i/dx_{i-1} = -1     (subdiagonal,   i >= 2)
 *   df_i/dx_i     = -4 x_i + 3  (diagonal)
 *   df_i/dx_{i+1} = -2     (superdiagonal, i <= n-1)
 */
void calculaJacobiana(const double *x, TriDiag *J)
{
    int n = J->n;
 
    J->d[0] = -4.0 * x[0] + 3.0;
    J->c[0] = -2.0;
 
    for (int i = 1; i < n - 1; ++i) {
        J->a[i - 1] = -1.0;
        J->d[i]     = -4.0 * x[i] + 3.0;
        J->c[i]     = -2.0;
    }
 
    J->a[n - 2] = -1.0;
    J->d[n - 1] = -4.0 * x[n - 1] + 3.0;
}
 