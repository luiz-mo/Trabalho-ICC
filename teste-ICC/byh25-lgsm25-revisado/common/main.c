#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "newton.h"
#include "utils.h"
 
/*
 * Programa principal: lê N, x0, epsilon e MAX da entrada padrão, executa o
 * Método de Newton e imprime as iterações e os tempos no formato especificado.
 */
int main(void)
{
    int n, max, nIter = 0, status = NEWTON_OK;
    double x0, eps, tTotal = 0.0, tJac = 0.0, tSL = 0.0;
 
    if (scanf("%d %lf %lf %d", &n, &x0, &eps, &max) != 4 || n < 2 || max < 0 || !isfinite(x0) || !isfinite(eps) || eps < 0.0) {
        fprintf(stderr, "Entrada invalida. Esperado: N(>=2) x0 epsilon MAX\n");
        return 1;
    }
 
    LIKWID_MARKER_INIT;
    LIKWID_MARKER_THREADINIT;
    LIKWID_MARKER_REGISTER("NEWTON");
    LIKWID_MARKER_REGISTER("JACOBIANA");
    LIKWID_MARKER_REGISTER("SISTEMA_LINEAR");

 
    double *hist = newton(n, x0, eps, max, &nIter, &tTotal, &tJac, &tSL, &status);
 
    LIKWID_MARKER_CLOSE;
 
    if (!hist) {
        if (status == NEWTON_ERRO_NUMERICO)
            fprintf(stderr, "Falha numerica: sistema singular, pivo invalido ou resultado nao finito\n");
        else if (status == NEWTON_ERRO_MEMORIA)
            fprintf(stderr, "Erro de alocacao de memoria\n");
        else
            fprintf(stderr, "Argumentos invalidos para o Metodo de Newton\n");
        return 1;
    }
 
    for (int k = 0; k <= nIter; ++k) {
        const double *x = hist + (size_t)k * n;
        for (int j = 0; j < n; ++j)
            printf("x%d = %.15g\n", j + 1, x[j]);
        if (k < nIter) printf("#\n");
    }
 
    printf("###########\n");
    printf("%.6f\n%.6f\n%.6f\n", tTotal, tJac, tSL);
    printf("###########\n");
 
    free(hist);
    return 0;
}