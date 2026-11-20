#include <float.h>
#include <math.h>
#include <stdlib.h>
#include "solver.h"

/*
 * Resolve J*x=b por eliminacao de Gauss tridiagonal
 * com pivoteamento parcial entre linhas adjacentes.
 *
 * A fatoracao pode criar uma segunda superdiagonal (du2).
 * Complexidade: O(n) em tempo e memoria.
 *
 * J e b sao modificados.
 */
int resolveTriDiag(TriDiag *J, double *b, double *x)
{
    if (!J || !b || !x || J->n < 1 || !J->d ||
        (J->n > 1 && (!J->a || !J->c)))
        return -1;

    const int n = J->n;
    double *dl = J->a;
    double *d = J->d;
    double *du = J->c;

    double *du2 = calloc((size_t)n, sizeof(double));

    if (!du2)
        return -1;

    double escala = 0.0;

    for (int i = 0; i < n; ++i) {
        if (!isfinite(d[i]) || !isfinite(b[i])) {
            free(du2);
            return -1;
        }

        if (fabs(d[i]) > escala)
            escala = fabs(d[i]);
    }

    for (int i = 0; i < n - 1; ++i) {
        if (!isfinite(dl[i]) || !isfinite(du[i])) {
            free(du2);
            return -1;
        }

        if (fabs(dl[i]) > escala)
            escala = fabs(dl[i]);

        if (fabs(du[i]) > escala)
            escala = fabs(du[i]);
    }

    if (escala == 0.0) {
        free(du2);
        return -1;
    }

    const double limite = 16.0 * DBL_EPSILON * escala;

    /* Fatoracao com pivoteamento parcial. */
    for (int i = 0; i < n - 1; ++i) {

        if (fabs(d[i]) >= fabs(dl[i])) {

            if (fabs(d[i]) <= limite) {
                free(du2);
                return -1;
            }

            const double fator = dl[i] / d[i];

            d[i + 1] -= fator * du[i];
            b[i + 1] -= fator * b[i];

            dl[i] = 0.0;

        } else {

            if (fabs(dl[i]) <= limite) {
                free(du2);
                return -1;
            }

            const double fator = d[i] / dl[i];

            d[i] = dl[i];

            const double diag_antiga = d[i + 1];

            d[i + 1] = du[i] - fator * diag_antiga;

            if (i < n - 2) {
                du2[i] = du[i + 1];
                du[i + 1] = -fator * du[i + 1];
            }

            du[i] = diag_antiga;

            const double b_antigo = b[i];

            b[i] = b[i + 1];
            b[i + 1] = b_antigo - fator * b[i + 1];
        }

        if (!isfinite(d[i + 1]) ||
            !isfinite(b[i + 1]) ||
            (i < n - 2 && !isfinite(du[i + 1]))) {

            free(du2);
            return -1;
        }
    }

    if (fabs(d[n - 1]) <= limite ||
        !isfinite(d[n - 1])) {

        free(du2);
        return -1;
    }

    /* Retrossubstituicao. */

    x[n - 1] = b[n - 1] / d[n - 1];

    if (!isfinite(x[n - 1])) {
        free(du2);
        return -1;
    }

    if (n >= 2) {

        if (fabs(d[n - 2]) <= limite) {
            free(du2);
            return -1;
        }

        x[n - 2] =
            (b[n - 2] - du[n - 2] * x[n - 1])
            / d[n - 2];

        if (!isfinite(x[n - 2])) {
            free(du2);
            return -1;
        }
    }

    for (int i = n - 3; i >= 0; --i) {

        if (fabs(d[i]) <= limite) {
            free(du2);
            return -1;
        }

        x[i] =
            (b[i]
             - du[i] * x[i + 1]
             - du2[i] * x[i + 2])
            / d[i];

        if (!isfinite(x[i])) {
            free(du2);
            return -1;
        }
    }

    free(du2);

    return 0;
}