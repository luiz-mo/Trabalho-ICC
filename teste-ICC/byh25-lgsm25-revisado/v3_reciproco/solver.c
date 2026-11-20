#include <float.h>
#include <math.h>
#include "solver.h"

/*
 * resolveTriDiag (versão 3): parte da versão 2 e troca divisões por
 * multiplicações pelo inverso do pivô.
 *
 * Motivo: a divisão em ponto flutuante tem latência alta (~13-20 ciclos) e
 * pipeline pouco profundo (slides de pipelining: ">> 10" estágios para DIV
 * contra 2-5 para MULT/ADD/FMA). A retrossubstituição é uma cadeia de
 * dependências (x[i] depende de x[i+1] e x[i+2]), então a latência da divisão
 * ficava no caminho crítico. Calculando inv[i] = 1/pivô durante a fatoração
 * (fora dessa cadeia), a retrossubstituição fica só com multiplicações/somas.
 *
 * Observação: x*(1/d) pode diferir de x/d em 1 ulp; a diferença é
 * desprezível frente à tolerância do método de Newton.
 */
int resolveTriDiag(TriDiag *J, double *restrict b, double *restrict x)
{
    const int n = J->n;
    const double *restrict dl = J->a;
    double *restrict d   = J->d;
    double *restrict du  = J->c;
    double *restrict du2 = J->du2;
    double *restrict inv = J->inv;

    double escala = 2.0;   /* max(|a|,|c|) = 2 na Jacobiana de Broyden */
    for (int i = 0; i < n; ++i) {
        const double v = fabs(d[i]);
        if (v > escala) escala = v;
    }
    const double limite = 16.0 * DBL_EPSILON * escala;

    for (int i = 0; i < n - 1; ++i) {
        if (fabs(d[i]) >= fabs(dl[i])) {
            if (fabs(d[i]) <= limite)
                return -1;

            const double iv = 1.0 / d[i];
            inv[i] = iv;
            const double fator = dl[i] * iv;
            d[i + 1] -= fator * du[i];
            b[i + 1] -= fator * b[i];
            if (i < n - 2) du2[i] = 0.0;
        } else {
            if (fabs(dl[i]) <= limite)
                return -1;

            const double iv = 1.0 / dl[i];      /* novo pivô = dl[i] */
            inv[i] = iv;
            const double fator = d[i] * iv;
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

    if (fabs(d[n - 1]) <= limite)
        return -1;
    inv[n - 1] = 1.0 / d[n - 1];

    /* Retrossubstituição só com multiplicações. */
    x[n - 1] = b[n - 1] * inv[n - 1];
    x[n - 2] = (b[n - 2] - du[n - 2] * x[n - 1]) * inv[n - 2];

    for (int i = n - 3; i >= 0; --i)
        x[i] = (b[i] - du[i] * x[i + 1] - du2[i] * x[i + 2]) * inv[i];
    return 0;
}
