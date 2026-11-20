#ifndef NEWTON_H
#define NEWTON_H

/* Códigos de resultado da execução do Método de Newton. */
enum {
    NEWTON_OK = 0,
    NEWTON_ERRO_MEMORIA = 1,
    NEWTON_ERRO_ARGUMENTO = 2,
    NEWTON_ERRO_NUMERICO = 3
};

/*
 * Executa o Método de Newton para o SNLB de Broyden.
 * Retorna o histórico (nIter+1) x n em row-major; o chamador libera com free().
 * Em caso de falha retorna NULL e informa o motivo em status.
 */
double *newton(int n, double x0, double eps, int max, int *nIter,
               double *tTotal, double *tJac, double *tSL, int *status);

#endif
