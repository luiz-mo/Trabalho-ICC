#include "funcao.h"
 
/*
 * calculaF: avalia a função de Broyden no ponto x.
 * Entrada : x (n valores), n >= 2
 * Saída   : f (n valores) = F(x)
 */
void calculaF(const double *x, double *f, int n)
{
    f[0] = -2.0 * x[0] * x[0] + 3.0 * x[0] - 2.0 * x[1] + 1.0;
 
    for (int i = 1; i < n - 1; ++i)
        f[i] = -2.0 * x[i] * x[i] + 3.0 * x[i] - x[i - 1] - 2.0 * x[i + 1] + 1.0;
 
    f[n - 1] = -2.0 * x[n - 1] * x[n - 1] + 3.0 * x[n - 1] - x[n - 2];
}
 