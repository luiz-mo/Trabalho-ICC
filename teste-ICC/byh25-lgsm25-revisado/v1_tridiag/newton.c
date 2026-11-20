#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include "newton.h"
#include "funcao.h"
#include "jacobiana.h"
#include "solver.h"
#include "utils.h"

/* Retorna 1 se todos os elementos do vetor forem finitos. */
static int vetorFinito(const double *v, int n)
{
    for (int i = 0; i < n; ++i)
        if (!isfinite(v[i])) return 0;
    return 1;
}

/* Calcula a norma infinito de um vetor finito. */
static double normaMax(const double *v, int n)
{
    double m = 0.0;
    for (int i = 0; i < n; ++i) {
        const double a = fabs(v[i]);
        if (a > m) m = a;
    }
    return m;
}

/*
 * newton: resolve J(X^k) Delta^k = -F(X^k) e atualiza X^(k+1).
 * Os tempos acumulados incluem somente a construção das Jacobianas e a
 * resolução dos sistemas lineares, respectivamente. A impressão fica fora
 * da região medida pelo marcador NEWTON.
 */
double *newton(int n, double x0, double eps, int max, int *nIter,
               double *tTotal, double *tJac, double *tSL, int *status)
{
    if (status) *status = NEWTON_ERRO_ARGUMENTO;
    if (n < 2 || max < 0 || !isfinite(x0) || !isfinite(eps) || eps < 0.0 ||
        !nIter || !tTotal || !tJac || !tSL || !status)
        return NULL;

    *nIter = 0;
    *tTotal = *tJac = *tSL = 0.0;

    /* Verifica estouro nos tamanhos das alocações. */
    if ((size_t)n > SIZE_MAX / sizeof(double) ||
        ((size_t)max + 1) > SIZE_MAX / ((size_t)n * sizeof(double))) {
        *status = NEWTON_ERRO_MEMORIA;
        return NULL;
    }

    TriDiag J = {0, NULL, NULL, NULL};
    double *F = malloc((size_t)n * sizeof(double));
    double *b = malloc((size_t)n * sizeof(double));
    double *delta = malloc((size_t)n * sizeof(double));
    size_t cap = (size_t)max + 1;
    double *hist = malloc(cap * (size_t)n * sizeof(double));

    if (!F || !b || !delta || !hist || alocaTriDiag(&J, n) != 0) {
        free(F); free(b); free(delta); free(hist);
        liberaTriDiag(&J);
        *status = NEWTON_ERRO_MEMORIA;
        return NULL;
    }

    for (int j = 0; j < n; ++j) hist[j] = x0;

    int k = 0;
    double tj = 0.0, ts = 0.0;
    const double t0 = timestamp();
    LIKWID_MARKER_START("NEWTON");

    for (int i = 0; i < max; ++i) {
        double *x = hist + (size_t)i * (size_t)n;
        calculaF(x, F, n);
        if (!vetorFinito(F, n)) {
            *status = NEWTON_ERRO_NUMERICO;
            goto falha;
        }
        if (normaMax(F, n) < eps) break;

        double t = timestamp();
        LIKWID_MARKER_START("JACOBIANA");
        calculaJacobiana(x, &J);
        LIKWID_MARKER_STOP("JACOBIANA");
        tj += timestamp() - t;

        for (int j = 0; j < n; ++j) b[j] = -F[j];

        t = timestamp();
        LIKWID_MARKER_START("SISTEMA_LINEAR");
        const int solver_ok = resolveTriDiag(&J, b, delta);
        LIKWID_MARKER_STOP("SISTEMA_LINEAR");
        ts += timestamp() - t;
        if (solver_ok != 0 || !vetorFinito(delta, n)) {
            *status = NEWTON_ERRO_NUMERICO;
            goto falha;
        }

        double *xn = hist + (size_t)(i + 1) * (size_t)n;
        for (int j = 0; j < n; ++j) {
            xn[j] = x[j] + delta[j];
            if (!isfinite(xn[j])) {
                *status = NEWTON_ERRO_NUMERICO;
                goto falha;
            }
        }
        k = i + 1;
        if (normaMax(delta, n) < eps) break;
    }

    LIKWID_MARKER_STOP("NEWTON");
    *tTotal = timestamp() - t0;
    *tJac = tj;
    *tSL = ts;
    *nIter = k;
    *status = NEWTON_OK;
    free(F); free(b); free(delta);
    liberaTriDiag(&J);
    return hist;

falha:
    LIKWID_MARKER_STOP("NEWTON");
    *tTotal = timestamp() - t0;
    *tJac = tj;
    *tSL = ts;
    *nIter = k;
    free(F); free(b); free(delta); free(hist);
    liberaTriDiag(&J);
    return NULL;
}
