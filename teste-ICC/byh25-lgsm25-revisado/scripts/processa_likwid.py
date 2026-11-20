#!/usr/bin/env python3
"""Extrai métricas LIKWID das saídas brutas para resultados/likwid.csv."""

import csv
import glob
import math
import os
import re
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ENTRADA = os.path.join(RAIZ, "resultados", "likwid")
SAIDA = os.path.join(RAIZ, "resultados", "likwid.csv")

NOME = re.compile(
    r"^(?P<v>.+?)_(?P<g>MEM|L3|L2CACHE|CACHE|L1CACHE|FLOPS_DP|FLOPS_AVX)_N(?P<n>\d+)\.txt$"
)
REGIAO = re.compile(r"Region\s+([A-Za-z0-9_]+),\s*Group\s+\d+:", re.IGNORECASE)
REGIOES_ESPERADAS = {"NEWTON", "JACOBIANA", "SISTEMA_LINEAR"}


def interessa(grupo, metrica):
    """Aceita somente a métrica correspondente ao grupo coletado."""
    m = " ".join(metrica.lower().split())

    if grupo == "MEM":
        return m.startswith("memory bandwidth [mbytes/s]")
    if grupo == "L3":
        return m.startswith("l3 bandwidth [mbytes/s]")
    if grupo == "L2CACHE":
        return m.startswith("l2") and "miss ratio" in m
    if grupo == "CACHE":
        return (m.startswith("cache") or m.startswith("data cache")) and "miss ratio" in m
    if grupo == "L1CACHE":
        return m.startswith("l1") and "miss ratio" in m
    if grupo == "FLOPS_DP":
        return m.startswith("dp") and ("mflop/s" in m or "mflops/s" in m)
    if grupo == "FLOPS_AVX":
        return "dp" in m and ("mflop/s" in m or "mflops/s" in m) and ("packed" in m or m.startswith("avx"))
    return False


def numero(txt):
    try:
        valor = float(txt.strip())
    except (TypeError, ValueError):
        return None
    return valor if math.isfinite(valor) else None


def processa_arquivo(caminho, versao, grupo, n):
    linhas = []
    regioes_encontradas = set()
    regiao = None

    with open(caminho, encoding="utf-8", errors="replace") as f:
        for lin in f:
            r = REGIAO.search(lin)
            if r:
                regiao = r.group(1).upper()
                if regiao in REGIOES_ESPERADAS:
                    regioes_encontradas.add(regiao)
                continue

            if regiao not in REGIOES_ESPERADAS or not lin.strip().startswith("|"):
                continue

            campos = [c.strip() for c in lin.strip().strip("|").split("|")]
            if len(campos) < 2 or not interessa(grupo, campos[0]):
                continue

            valor = numero(campos[1])
            if valor is None:
                continue

            linhas.append((versao, n, grupo, regiao, campos[0], valor))

    faltantes = REGIOES_ESPERADAS - regioes_encontradas
    if faltantes:
        raise ValueError(
            f"{os.path.basename(caminho)}: regiões ausentes: {', '.join(sorted(faltantes))}"
        )
    if not linhas:
        raise ValueError(f"{os.path.basename(caminho)}: nenhuma métrica válida encontrada")

    regioes_com_metrica = {linha[3] for linha in linhas}
    sem_metrica = REGIOES_ESPERADAS - regioes_com_metrica
    if sem_metrica:
        raise ValueError(
            f"{os.path.basename(caminho)}: sem métrica válida em {', '.join(sorted(sem_metrica))}"
        )

    return linhas



def le_configuracao():
    caminho = os.path.join(RAIZ, "resultados", "config_experimento.txt")
    if not os.path.isfile(caminho):
        raise ValueError("config_experimento.txt ausente; execute a bateria de desempenho completa.")

    config = {}
    with open(caminho, encoding="utf-8") as f:
        for linha in f:
            linha = linha.strip()
            if linha and "=" in linha:
                chave, valor = linha.split("=", 1)
                config[chave.strip()] = valor.strip()

    obrigatorias = {
        "versoes", "tamanhos", "n_max_densa", "grupo_mem",
        "grupo_cache", "grupo_flops_dp", "grupo_flops_avx"
    }
    faltantes = obrigatorias - config.keys()
    if faltantes:
        raise ValueError(f"config_experimento.txt incompleto: {', '.join(sorted(faltantes))}")

    try:
        config["versoes_lista"] = config["versoes"].split()
        config["tamanhos_lista"] = [int(x) for x in config["tamanhos"].split()]
        config["n_max_densa_int"] = int(config["n_max_densa"])
    except ValueError:
        raise ValueError("versoes, tamanhos ou n_max_densa inválidos em config_experimento.txt")

    if not config["versoes_lista"] or not config["tamanhos_lista"]:
        raise ValueError("versoes e tamanhos não podem estar vazios na configuração.")
    return config


def valida_cobertura_arquivos(arquivos, config):
    grupos = [
        config["grupo_mem"], config["grupo_cache"],
        config["grupo_flops_dp"], config["grupo_flops_avx"]
    ]
    esperados = set()
    for versao in config["versoes_lista"]:
        for n in config["tamanhos_lista"]:
            if versao == "v0_densa" and n > config["n_max_densa_int"]:
                continue
            for grupo in grupos:
                esperados.add(f"{versao}_{grupo}_N{n}.txt")

    encontrados = {os.path.basename(a) for a in arquivos}
    ausentes = esperados - encontrados
    inesperados = encontrados - esperados
    if ausentes:
        amostra = ", ".join(sorted(ausentes)[:8])
        raise ValueError(f"coletas LIKWID ausentes ({len(ausentes)}): {amostra}")
    if inesperados:
        amostra = ", ".join(sorted(inesperados)[:8])
        raise ValueError(f"arquivos LIKWID inesperados: {amostra}")


def main():
    arquivos = sorted(glob.glob(os.path.join(ENTRADA, "*.txt")))
    if not arquivos:
        if os.path.exists(SAIDA):
            os.remove(SAIDA)
        print(f"Nenhum arquivo .txt em {ENTRADA}. Execute a coleta LIKWID primeiro.", file=sys.stderr)
        return 1

    try:
        config = le_configuracao()
        valida_cobertura_arquivos(arquivos, config)
    except ValueError as exc:
        if os.path.exists(SAIDA):
            os.remove(SAIDA)
        print(f"ERRO: {exc}", file=sys.stderr)
        return 1

    todas = []
    erros = []
    processados = 0

    for caminho in arquivos:
        m = NOME.match(os.path.basename(caminho))
        if not m:
            erros.append(f"nome de arquivo inesperado: {os.path.basename(caminho)}")
            continue
        try:
            todas.extend(processa_arquivo(caminho, m["v"], m["g"], int(m["n"])))
            processados += 1
        except (OSError, ValueError) as exc:
            erros.append(str(exc))

    if erros or not todas:
        # Remove CSV antigo para impedir que dados de uma coleta anterior sejam
        # confundidos com os resultados da coleta atual incompleta.
        if os.path.exists(SAIDA):
            os.remove(SAIDA)
        for erro in erros:
            print(f"ERRO: {erro}", file=sys.stderr)
        print("Processamento interrompido: existem coletas ausentes ou inválidas.", file=sys.stderr)
        return 1

    with open(SAIDA, "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f, lineterminator="\n")
        w.writerow(["versao", "N", "grupo", "regiao", "metrica", "valor"])
        w.writerows(todas)

    print(f"{len(todas)} métricas válidas extraídas de {processados} arquivos: {SAIDA}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
