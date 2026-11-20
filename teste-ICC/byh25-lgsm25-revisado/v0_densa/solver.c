#include <float.h>
#include <math.h>
#include "solver.h"

/*
 * resolveDensa: Eliminação de Gauss com pivoteamento parcial (slides da
 * Parte 3) seguida de retrossubstituição. Implementação direta, sem
 * nenhuma otimização, usada como linha de base.
 */
int resolveDensa(Densa *J, double *b, double *x)
{
    if (!J || !J->A || !b || !x || J->n < 1)
        return -1;

    const int n = J->n;
    double **A = J->A;

    /* Escala = maior |a_ij|, usada no teste de pivô "muito pequeno". */
    double escala = 0.0;
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j) {
            const double v = fabs(A[i][j]);
            if (!isfinite(v)) return -1;
            if (v > escala) escala = v;
        }
    if (escala == 0.0)
        return -1;
    const double limite = 16.0 * DBL_EPSILON * escala;

    /* Triangularização. */
    for (int k = 0; k < n - 1; ++k) {
        int p = k;
        for (int i = k + 1; i < n; ++i)
            if (fabs(A[i][k]) > fabs(A[p][k]))
                p = i;

        if (fabs(A[p][k]) <= limite)
            return -1;

        if (p != k) {
            double *tmpL = A[p]; A[p] = A[k]; A[k] = tmpL;
            const double tmpb = b[p]; b[p] = b[k]; b[k] = tmpb;
        }

        for (int i = k + 1; i < n; ++i) {
            const double m = A[i][k] / A[k][k];
            A[i][k] = 0.0;
            for (int j = k + 1; j < n; ++j)
                A[i][j] -= m * A[k][j];
            b[i] -= m * b[k];
        }
    }

    if (fabs(A[n - 1][n - 1]) <= limite)
        return -1;

    /* Retrossubstituição. */
    for (int i = n - 1; i >= 0; --i) {
        x[i] = b[i];
        for (int j = i + 1; j < n; ++j)
            x[i] -= A[i][j] * x[j];
        x[i] /= A[i][i];
        if (!isfinite(x[i]))
            return -1;
    }
    return 0;
}
