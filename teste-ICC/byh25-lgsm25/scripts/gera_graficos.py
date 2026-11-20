#!/usr/bin/env python3
"""
gera_graficos.py - gera os 12 gráficos de linhas exigidos pelo enunciado.

Entradas : resultados/tempos.csv   (gerado por rodar_testes.sh)
           resultados/likwid.csv   (gerado por processa_likwid.py)
Saídas   : resultados/graficos/<teste>_<trecho>.png  (12 arquivos)
           resultados/tabela_tempos.csv              (média e desvio por versão/N)

Testes  : tempo | banda | cache | flops
Trechos : newton (execução total) | jacobiana | sistema_linear
Eixo X (N) sempre em escala logarítmica; eixo Y do tempo também.
Uma linha por versão do programa, com cores distintas.
"""
import csv
import os
import sys
from collections import defaultdict
from statistics import mean, pstdev

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RES = os.path.join(RAIZ, "resultados")
OUT = os.path.join(RES, "graficos")

VERSOES = [  # (pasta, rótulo, cor, marcador)
    ("v0_densa",     "v0 - base (matriz densa)",          "tab:red",    "o"),
    ("v1_tridiag",   "v1 - tridiagonal simples",          "tab:orange", "s"),
    ("v2_otimizada", "v2 - otimizada",                    "tab:green",  "^"),
    ("v3_reciproco", "v3 - otimizada + inverso do pivô",  "tab:blue",   "D"),
]
TRECHOS = [  # (id do arquivo, região LIKWID, coluna do CSV de tempos, título)
    ("newton",         "NEWTON",         "t_total_ms",           "Método de Newton (execução total)"),
    ("jacobiana",      "JACOBIANA",      "t_jacobiana_ms",       "Cálculo das matrizes Jacobianas"),
    ("sistema_linear", "SISTEMA_LINEAR", "t_sistema_linear_ms",  "Resolução dos sistemas lineares"),
]


def le_csv(nome):
    caminho = os.path.join(RES, nome)
    if not os.path.exists(caminho):
        return []
    with open(caminho, newline="") as f:
        return list(csv.DictReader(f))


def finaliza(ax, titulo, ylabel, ylog=False):
    ax.set_xscale("log")
    if ylog:
        ax.set_yscale("log")
    ax.set_xlabel("Tamanho do SNLB (N)")
    ax.set_ylabel(ylabel)
    ax.set_title(titulo)
    ax.grid(True, which="both", alpha=0.3)
    ax.legend(fontsize=8)
    plt.tight_layout()


def salva(fig, nome):
    os.makedirs(OUT, exist_ok=True)
    fig.savefig(os.path.join(OUT, nome + ".png"), dpi=150)
    plt.close(fig)
    print("gerado:", nome + ".png")


def graficos_tempo(tempos):
    dados = defaultdict(lambda: defaultdict(list))   # (versao, coluna) -> N -> [t]
    for r in tempos:
        for _, _, col, _ in TRECHOS:
            dados[(r["versao"], col)][int(r["N"])].append(float(r[col]))
    # tabela resumo
    with open(os.path.join(RES, "tabela_tempos.csv"), "w", newline="") as f:
        w = csv.writer(f, lineterminator="\n")
        w.writerow(["versao", "N", "trecho", "media_ms", "desvio_ms", "reps"])
        for (v, col), por_n in sorted(dados.items()):
            for n, ts in sorted(por_n.items()):
                w.writerow([v, n, col, f"{mean(ts):.6f}", f"{pstdev(ts):.6f}", len(ts)])
    for tid, _, col, titulo in TRECHOS:
        fig, ax = plt.subplots(figsize=(6.5, 4.2))
        for v, rot, cor, mk in VERSOES:
            por_n = dados.get((v, col))
            if not por_n:
                continue
            ns = sorted(por_n)
            med = [mean(por_n[n]) for n in ns]
            dp = [pstdev(por_n[n]) for n in ns]
            ax.errorbar(ns, med, yerr=dp, label=rot, color=cor, marker=mk,
                        markersize=4, capsize=2, linewidth=1.3)
        finaliza(ax, "Tempo de execução - " + titulo, "Tempo médio (ms)", ylog=True)
        salva(fig, f"tempo_{tid}")


def seleciona(metrica_tipo, grupo, metrica):
    m = metrica.lower()
    if metrica_tipo == "banda":
        return grupo in ("MEM", "L3") and "bandwidth [mbytes/s]" in m
    if metrica_tipo == "cache":
        return grupo in ("L2CACHE", "CACHE", "L1CACHE") and "miss ratio" in m
    if metrica_tipo == "flops":
        if grupo == "FLOPS_DP":
            return m.startswith("dp [mflop/s]")
        if grupo == "FLOPS_AVX":
            return "packed dp [mflop/s]" in m or m.startswith("avx dp [mflop/s]")
    return False


def graficos_likwid(lik):
    cfg = {
        "banda": ("Banda de memória", "Memory bandwidth [MBytes/s]"),
        "cache": ("Taxa de cache miss (L2, ou L1 se não houver L2)", "Cache miss ratio"),
        "flops": ("Operações aritméticas", "MFLOP/s"),
    }
    for teste, (nome, ylabel) in cfg.items():
        for tid, regiao, _, titulo in TRECHOS:
            pts = defaultdict(dict)   # (versao, grupo) -> N -> valor
            for r in lik:
                if r["regiao"] == regiao and seleciona(teste, r["grupo"], r["metrica"]):
                    pts[(r["versao"], r["grupo"])][int(r["N"])] = float(r["valor"])
            if not pts:
                print(f"sem dados para {teste}_{tid}")
                continue
            fig, ax = plt.subplots(figsize=(6.5, 4.2))
            for v, rot, cor, mk in VERSOES:
                for (vv, g), por_n in sorted(pts.items()):
                    if vv != v:
                        continue
                    ns = sorted(por_n)
                    estilo = "--" if g == "FLOPS_AVX" else "-"
                    r2 = rot + (f" [{g}]" if teste == "flops" else "")
                    ax.plot(ns, [por_n[n] for n in ns], estilo, color=cor, marker=mk,
                            markersize=4, linewidth=1.3, label=r2)
            finaliza(ax, f"{nome} - {titulo}", ylabel)
            salva(fig, f"{teste}_{tid}")


def main():
    tempos = le_csv("tempos.csv")
    lik = le_csv("likwid.csv")
    if not tempos and not lik:
        print("Sem dados em", RES, "- rode scripts/rodar_testes.sh antes.")
        return 1
    if tempos:
        graficos_tempo(tempos)
    if lik:
        graficos_likwid(lik)
    return 0


if __name__ == "__main__":
    sys.exit(main())
