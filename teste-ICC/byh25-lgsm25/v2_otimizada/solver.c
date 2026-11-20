#include <float.h>
#include <math.h>
#include "solver.h"

/*
 * resolveTriDiag: mesma matemática da versão 1, com as seguintes mudanças:
 *  - du2 vem pré-alocado em J (sem calloc/free a cada iteração);
 *  - uma única passada para a escala (a versão 1 faz 3 passadas com isfinite);
 *  - sem escrita inútil em dl[i] (a[] é constante e reaproveitada);
 *  - ponteiros restrict, sem verificações isfinite dentro do laço;
 *  - sem testes de pivô redundantes na retrossubstituição.
 *
 * Escala: |a| = 1 e |c| = 2 na Jacobiana de Broyden, logo
 * max|J_ij| = max(2, max|d_i|), idêntico ao cálculo da versão 1.
 */
int resolveTriDiag(TriDiag *J, double *restrict b, double *restrict x)
{
    const int n = J->n;
    const double *restrict dl = J->a;
    double *restrict d   = J->d;
    double *restrict du  = J->c;
    double *restrict du2 = J->du2;

    double escala = 2.0;
    for (int i = 0; i < n; ++i) {
        const double v = fabs(d[i]);
        if (v > escala) escala = v;
    }
    const double limite = 16.0 * DBL_EPSILON * escala;

    /* Fatoração com pivoteamento parcial (O(n)). */
    for (int i = 0; i < n - 1; ++i) {
        if (fabs(d[i]) >= fabs(dl[i])) {
            /* Sem troca de linhas (caso comum). */
            if (fabs(d[i]) <= limite)
                return -1;

            const double fator = dl[i] / d[i];
            d[i + 1] -= fator * du[i];
            b[i + 1] -= fator * b[i];
            if (i < n - 2) du2[i] = 0.0;
        } else {
            /* Troca das linhas i e i+1. */
            if (fabs(dl[i]) <= limite)
                return -1;

            const double fator = d[i] / dl[i];
            const double diag_antiga = d[i + 1];

            d[i] = dl[i];
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
    }

    /* Os pivôs d[0..n-2] já foram testados na fatoração; falta o último. */
    if (fabs(d[n - 1]) <= limite)
        return -1;

    /* Retrossubstituição. */
    x[n - 1] = b[n - 1] / d[n - 1];
    if (n >= 2)
        x[n - 2] = (b[n - 2] - du[n - 2] * x[n - 1]) / d[n - 2];

    for (int i = n - 3; i >= 0; --i)
        x[i] = (b[i] - du[i] * x[i + 1] - du2[i] * x[i + 2]) / d[i];
    return 0;
}
