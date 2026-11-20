#include "funcao.h"

/*
 * calculaMenosF: b[i] = -f_i(x), em laços simples, de passo unitário e sem
 * desvios, para que o GCC os vetorize com AVX (-O3 -march=native):
 *   -f_i = x_i (2 x_i - 3) + x_{i-1} + 2 x_{i+1} - 1
 * As bordas (i = 0 e i = n-1) ficam fora do laço principal.
 */
void calculaMenosF(const double *restrict x, double *restrict b, int n)
{
    b[0] = x[0] * (2.0 * x[0] - 3.0) + 2.0 * x[1] - 1.0;

    for (int i = 1; i < n - 1; ++i)
        b[i] = x[i] * (2.0 * x[i] - 3.0) + x[i - 1] + 2.0 * x[i + 1] - 1.0;

    b[n - 1] = x[n - 1] * (2.0 * x[n - 1] - 3.0) + x[n - 2];
}
