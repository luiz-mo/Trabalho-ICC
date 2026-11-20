#!/usr/bin/env python3
"""
verifica.py - testes de corretude e de concordância entre as versões.

Para cada caso (N, x0, epsilon, MAX) executa todos os binários bin/broyden_<v>
e confere:
  1. formato da saída (x1..xn por iteração, '#' entre iterações, bloco final
     '###########' com 3 tempos);
  2. número de iterações impressas;
  3. residual ||F(x_final)||_inf calculado AQUI em Python (independente do C);
  4. concordância entre as versões (maior diferença no vetor final).
Código de saída 0 se tudo passou, 1 caso contrário.
"""
import math
import os
import subprocess
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
VERSOES = ["v0_densa", "v1_tridiag", "v2_otimizada", "v3_reciproco"]
# (N, x0, eps, MAX)
CASOS = [(2, 0.0, 1e-12, 25), (3, 0.0, 1e-12, 25), (8, 0.0, 0.0, 25),
         (32, 0.0, 1e-12, 25), (100, -1.0, 1e-12, 50), (257, -0.5, 1e-12, 50),
         (500, 0.0, 0.0, 25), (1000, 0.0, 1e-10, 25)]
TOL_RESIDUO = 1e-8
TOL_CONCORDANCIA = 1e-9


def broyden_F(x):
    n = len(x)
    f = [0.0] * n
    f[0] = -2 * x[0] ** 2 + 3 * x[0] - 2 * x[1] + 1
    for i in range(1, n - 1):
        f[i] = -2 * x[i] ** 2 + 3 * x[i] - x[i - 1] - 2 * x[i + 1] + 1
    f[n - 1] = -2 * x[n - 1] ** 2 + 3 * x[n - 1] - x[n - 2]
    return f


def roda(versao, n, x0, eps, mx):
    exe = os.path.join(RAIZ, "bin", "broyden_" + versao)
    ent = f"{n} {x0!r} {eps!r} {mx}\n"
    p = subprocess.run([exe], input=ent, capture_output=True, text=True)
    return p.returncode, p.stdout


def parse(saida, n):
    """Retorna (lista de iterações, [t_total, t_jac, t_sl]) ou levanta ValueError."""
    corpo, sep, resto = saida.partition("###########\n")
    if not sep:
        raise ValueError("faltou o separador final '###########'")
    tempos_txt, sep2, _ = resto.partition("###########")
    tempos = [float(t) for t in tempos_txt.split()]
    if len(tempos) != 3:
        raise ValueError("esperados 3 tempos no bloco final")
    iteracoes = []
    for bloco in corpo.strip("\n").split("#\n"):
        linhas = [l for l in bloco.strip("\n").split("\n") if l]
        if len(linhas) != n:
            raise ValueError(f"iteração com {len(linhas)} linhas, esperado {n}")
        vet = []
        for j, l in enumerate(linhas, 1):
            nome, _, val = l.partition(" = ")
            if nome != f"x{j}":
                raise ValueError(f"linha inesperada: {l!r}")
            vet.append(float(val))
        iteracoes.append(vet)
    return iteracoes, tempos


def main():
    ok_geral = True
    for (n, x0, eps, mx) in CASOS:
        print(f"--- N={n} x0={x0} eps={eps} MAX={mx}")
        finais = {}
        for v in VERSOES:
            if not os.path.exists(os.path.join(RAIZ, "bin", "broyden_" + v)):
                print(f"  [{v}] binário ausente (rode 'make versoes')")
                ok_geral = False
                continue
            rc, saida = roda(v, n, x0, eps, mx)
            if rc != 0:
                print(f"  [{v}] FALHA: código de saída {rc}")
                ok_geral = False
                continue
            try:
                its, tempos = parse(saida, n)
            except ValueError as e:
                print(f"  [{v}] FALHA de formato: {e}")
                ok_geral = False
                continue
            if eps == 0.0 and len(its) != mx + 1:
                print(f"  [{v}] FALHA: com eps=0 eram esperadas {mx+1} iterações impressas, vieram {len(its)}")
                ok_geral = False
            res = max(abs(t) for t in broyden_F(its[-1]))
            estado = "ok" if res < TOL_RESIDUO else "RESIDUO ALTO"
            if res >= TOL_RESIDUO:
                ok_geral = False
            print(f"  [{v:13s}] iterações={len(its)-1:3d}  ||F||={res:.2e}  {estado}")
            finais[v] = its[-1]
        ref = finais.get("v1_tridiag")
        if ref:
            for v, x in finais.items():
                dif = max(abs(a - b) for a, b in zip(x, ref))
                if dif > TOL_CONCORDANCIA:
                    print(f"  [{v}] DIFERE da v1: {dif:.2e}")
                    ok_geral = False
    # Entrada inválida deve falhar com código != 0
    p = subprocess.run([os.path.join(RAIZ, "bin", "broyden_v2_otimizada")],
                       input="1 0 0 25\n", capture_output=True, text=True)
    if p.returncode == 0:
        print("FALHA: N=1 deveria ser rejeitado")
        ok_geral = False
    else:
        print("--- entrada inválida (N=1) rejeitada: ok")
    print("\nRESULTADO:", "TUDO OK" if ok_geral else "HÁ FALHAS")
    return 0 if ok_geral else 1


if __name__ == "__main__":
    sys.exit(main())
