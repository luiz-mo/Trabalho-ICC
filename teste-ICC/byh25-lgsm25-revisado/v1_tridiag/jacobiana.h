
#ifndef JACOBIANA_H
#define JACOBIANA_H
 
/*
 * Matriz tridiagonal n x n armazenada em três vetores:
 *   a[i] = J[i+1][i]   (subdiagonal,   tamanho n-1)
 *   d[i] = J[i][i]     (diagonal,      tamanho n)
 *   c[i] = J[i][i+1]   (superdiagonal, tamanho n-1)
 */
typedef struct {
    int n;
    double *a;
    double *d;
    double *c;
} TriDiag;
 
/* Aloca os vetores da matriz tridiagonal (retorna 0 em sucesso, -1 em erro). */
int alocaTriDiag(TriDiag *J, int n);
 
/* Libera a memória da matriz tridiagonal. */
void liberaTriDiag(TriDiag *J);
 
/* Calcula a matriz Jacobiana da função de Broyden no ponto x. */
void calculaJacobiana(const double *x, TriDiag *J);
 
#endif
 