#ifndef JACOBIANA_H
#define JACOBIANA_H

/*
 * VERSÃO 0 (BASE, SEM OTIMIZAÇÃO): matriz Jacobiana DENSA n x n.
 *
 * Representação "clássica" de livro-texto: vetor de ponteiros para linhas,
 * cada linha alocada separadamente com malloc (linhas NÃO contíguas em
 * memória), e nenhuma exploração da estrutura tridiagonal.
 */
typedef struct {
    int n;
    double **A;   /* A[i][j] = df_i/dx_j */
} Densa;

/* Aloca a matriz n x n (retorna 0 em sucesso, -1 em erro). */
int alocaDensa(Densa *J, int n);

/* Libera a memória da matriz. */
void liberaDensa(Densa *J);

/* Preenche a Jacobiana de Broyden em x (zera toda a matriz e depois
 * preenche as três diagonais). Custo O(n^2) por chamada. */
void calculaJacobiana(const double *x, Densa *J);

#endif
