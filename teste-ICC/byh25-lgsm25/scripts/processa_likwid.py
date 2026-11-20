#!/usr/bin/env python3
"""
processa_likwid.py - extrai as métricas dos arquivos brutos do likwid-perfctr.

Entrada : resultados/likwid/<versao>_<GRUPO>_N<N>.txt
Saída   : resultados/likwid.csv  com colunas
          versao,N,grupo,regiao,metrica,valor

Para cada região (NEWTON, JACOBIANA, SISTEMA_LINEAR) são guardadas as métricas
de interesse do enunciado:
  - MEM/L3 : "Memory bandwidth [MBytes/s]" / "L3 bandwidth [MBytes/s]"
  - L2CACHE/CACHE/L1CACHE : qualquer métrica "... miss ratio" (L2 miss ratio ou
            data cache miss ratio)
  - FLOPS_DP / FLOPS_AVX : métricas "[MFLOP/s]"
Como há uma única thread, usa-se a primeira coluna numérica da tabela de métricas.
"""
import csv
import glob
import os
import re
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ENTRADA = os.path.join(RAIZ, "resultados", "likwid")
SAIDA = os.path.join(RAIZ, "resultados", "likwid.csv")

NOME = re.compile(r"^(?P<v>.+?)_(?P<g>MEM|L3|L2CACHE|CACHE|L1CACHE|FLOPS_DP|FLOPS_AVX)_N(?P<n>\d+)\.txt$")
REGIAO = re.compile(r"Region\s+(\w+),\s*Group\s+\d+:\s*(\w+)")


def interessa(metrica):
    m = metrica.lower()
    return ("bandwidth [mbytes/s]" in m and ("memory" in m or "l3" in m)) \
        or "miss ratio" in m \
        or "[mflop/s]" in m


def numero(txt):
    try:
        return float(txt.strip())
    except ValueError:
        return None


def processa_arquivo(caminho, versao, grupo, n):
    linhas = []
    regiao = None
    with open(caminho, errors="replace") as f:
        for lin in f:
            r = REGIAO.search(lin)
            if r:
                regiao = r.group(1)
                continue
            if regiao is None or not lin.startswith("|"):
                continue
            campos = [c.strip() for c in lin.strip().strip("|").split("|")]
            if len(campos) < 2 or not interessa(campos[0]):
                continue
            val = numero(campos[1])
            if val is None:        # 'nan', 'inf' ou texto: ignora
                continue
            linhas.append((versao, n, grupo, regiao, campos[0], val))
    return linhas


def main():
    arquivos = sorted(glob.glob(os.path.join(ENTRADA, "*.txt")))
    if not arquivos:
        print("Nenhum arquivo em", ENTRADA, "- rode scripts/rodar_testes.sh antes.")
        return 1
    todas = []
    for a in arquivos:
        m = NOME.match(os.path.basename(a))
        if not m:
            continue
        todas += processa_arquivo(a, m["v"], m["g"], int(m["n"]))
    with open(SAIDA, "w", newline="") as f:
        w = csv.writer(f, lineterminator="\n")
        w.writerow(["versao", "N", "grupo", "regiao", "metrica", "valor"])
        w.writerows(todas)
    print(f"{len(todas)} métricas gravadas em {SAIDA}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
