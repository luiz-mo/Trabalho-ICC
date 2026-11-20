#ifndef FUNCAO_H
#define FUNCAO_H

/*
 * VERSÃO 2 (OTIMIZADA): função de Broyden F(X) = (f_1(X), ..., f_n(X)):
 *   f_1 = -2 x_1^2 + 3 x_1 - 2 x_2 + 1
 *   f_i = -2 x_i^2 + 3 x_i - x_{i-1} - 2 x_{i+1} + 1,  2 <= i <= n-1
 *   f_n = -2 x_n^2 + 3 x_n - x_{n-1}
 *
 * calculaMenosF grava diretamente b = -F(x) (lado direito do sistema
 * linear J*delta = -F), eliminando um vetor auxiliar e uma passagem de
 * cópia/negação. x e b não podem se sobrepor (restrict). n >= 2.
 */
void calculaMenosF(const double *restrict x, double *restrict b, int n);

#endif
