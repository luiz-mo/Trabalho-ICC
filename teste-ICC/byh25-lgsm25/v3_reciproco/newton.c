#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include "newton.h"
#include "funcao.h"
#include "jacobiana.h"
#include "solver.h"
#include "utils.h"

/*
 * Passada única sobre v[0..n-1] que calcula a norma do máximo e verifica se
 * todos os elementos são finitos (fusão de laços: antes eram 2 passadas).
 * O teste de finitude é sem desvio (fabs(v) <= DBL_MAX é falso para NaN/Inf).
 * Retorna a norma em *norma e 1 se todos forem finitos, 0 caso contrário.
 */
static int normaEFinito(const double *restrict v, int n, double *norma)
{
    double m = 0.0;
    int ok = 1;
    for (int i = 0; i < n; ++i) {
        const double a = fabs(v[i]);
        ok &= (a <= DBL_MAX);
        if (a > m) m = a;
    }
    *norma = m;
    return ok;
}

/*
 * atualizaX: xn = x + delta, norma do máximo de delta e checagem de finitude
 * de xn, tudo em UMA passada sobre os vetores (fusão de laços: a versão 1
 * fazia 3 passadas). Retorna 1 se xn é todo finito, 0 caso contrário.
 */
static int atualizaX(const double *restrict x, const double *restrict delta,
                     double *restrict xn, int n, double *normaDelta)
{
    double m = 0.0;
    int ok = 1;
    for (int j = 0; j < n; ++j) {
        const double v = x[j] + delta[j];
        const double a = fabs(delta[j]);
        xn[j] = v;
        ok &= (fabs(v) <= DBL_MAX);
        if (a > m) m = a;
    }
    *normaDelta = m;
    return ok;
}

/*
 * newton: resolve J(X^k) Delta^k = -F(X^k) e atualiza X^(k+1).
 * Os tempos acumulados incluem somente a construção das Jacobianas e a
 * resolução dos sistemas lineares, respectivamente. A impressão fica fora
 * da região medida pelo marcador NEWTON.
 *
 * Otimizações desta versão (ver README e relatório):
 *  - b = -F(x) calculado diretamente (sem vetor F, sem cópia/negação);
 *  - norma e checagem de finitude de b fundidas em uma só passada;
 *  - atualização x + delta, norma de delta e checagem de finitude de x
 *    fundidas em uma só passada (atualizaX);
 *  - vetores de trabalho alinhados em 64 bytes;
 *  - nenhuma alocação dentro do laço principal.
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

    TriDiag J = {0, NULL, NULL, NULL, NULL, NULL};
    double *b = alocaAlinhado((size_t)n);
    double *delta = alocaAlinhado((size_t)n);
    const size_t cap = (size_t)max + 1;
    double *hist = alocaAlinhado(cap * (size_t)n);

    if (!b || !delta || !hist || alocaTriDiag(&J, n) != 0) {
        free(b); free(delta); free(hist);
        liberaTriDiag(&J);
        *status = NEWTON_ERRO_MEMORIA;
        return NULL;
    }

    for (int j = 0; j < n; ++j) hist[j] = x0;

    int k = 0;
    double tj = 0.0, ts = 0.0, norma;
    const double t0 = timestamp();
    LIKWID_MARKER_START("NEWTON");

    for (int i = 0; i < max; ++i) {
        const double *restrict x = hist + (size_t)i * (size_t)n;
        double *restrict xn = hist + (size_t)(i + 1) * (size_t)n;

        calculaMenosF(x, b, n);
        if (!normaEFinito(b, n, &norma)) {
            *status = NEWTON_ERRO_NUMERICO;
            goto falha;
        }
        if (norma < eps) break;

        double t = timestamp();
        LIKWID_MARKER_START("JACOBIANA");
        calculaJacobiana(x, &J);
        LIKWID_MARKER_STOP("JACOBIANA");
        tj += timestamp() - t;

        t = timestamp();
        LIKWID_MARKER_START("SISTEMA_LINEAR");
        const int solver_ok = resolveTriDiag(&J, b, delta);
        LIKWID_MARKER_STOP("SISTEMA_LINEAR");
        ts += timestamp() - t;
        if (solver_ok != 0) {
            *status = NEWTON_ERRO_NUMERICO;
            goto falha;
        }

        double normaDelta;
        if (!atualizaX(x, delta, xn, n, &normaDelta)) {
            *status = NEWTON_ERRO_NUMERICO;
            goto falha;
        }
        k = i + 1;
        if (normaDelta < eps) break;
    }

    LIKWID_MARKER_STOP("NEWTON");
    *tTotal = timestamp() - t0;
    *tJac = tj;
    *tSL = ts;
    *nIter = k;
    *status = NEWTON_OK;
    free(b); free(delta);
    liberaTriDiag(&J);
    return hist;

falha:
    LIKWID_MARKER_STOP("NEWTON");
    *tTotal = timestamp() - t0;
    *tJac = tj;
    *tSL = ts;
    *nIter = k;
    free(b); free(delta); free(hist);
    liberaTriDiag(&J);
    return NULL;
}
