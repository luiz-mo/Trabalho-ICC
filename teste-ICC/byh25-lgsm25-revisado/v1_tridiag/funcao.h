#ifndef FUNCAO_H
#define FUNCAO_H
 
/*
 * Função de Broyden F(X) = (f_1(X), ..., f_n(X)):
 *   f_1 = -2 x_1^2 + 3 x_1 - 2 x_2 + 1
 *   f_i = -2 x_i^2 + 3 x_i - x_{i-1} - 2 x_{i+1} + 1,  2 <= i <= n-1
 *   f_n = -2 x_n^2 + 3 x_n - x_{n-1}
 *
 * Calcula F(x) e armazena o resultado em f (vetores de tamanho n, n >= 2).
 */
void calculaF(const double *x, double *f, int n);
 
#endif