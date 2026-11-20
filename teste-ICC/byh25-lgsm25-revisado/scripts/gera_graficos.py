#!/usr/bin/env python3
"""Gera os 12 gráficos de desempenho a partir dos CSVs experimentais."""

import csv
import math
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

VERSOES = [
    ("v0_densa", "v0 - base (matriz densa)", "tab:red", "o"),
    ("v1_tridiag", "v1 - tridiagonal simples", "tab:orange", "s"),
    ("v2_otimizada", "v2 - otimizada", "tab:green", "^"),
    ("v3_reciproco", "v3 - otimizada + inverso do pivô", "tab:blue", "D"),
]
VERSOES_VALIDAS = {v[0] for v in VERSOES}
TAMANHOS_OFICIAIS = [32, 64, 128, 256, 512, 1000, 2000, 4000, 8000, 9000, 10000, 20000]
TRECHOS = [
    ("newton", "NEWTON", "t_total_ms", "Método de Newton (execução total)"),
    ("jacobiana", "JACOBIANA", "t_jacobiana_ms", "Cálculo das matrizes Jacobianas"),
    ("sistema_linear", "SISTEMA_LINEAR", "t_sistema_linear_ms", "Resolução dos sistemas lineares"),
]
REGIOES_VALIDAS = {t[1] for t in TRECHOS}


def le_csv(nome):
    caminho = os.path.join(RES, nome)
    if not os.path.isfile(caminho):
        raise ValueError(f"Arquivo obrigatório ausente: {caminho}")
    with open(caminho, newline="", encoding="utf-8") as f:
        return list(csv.DictReader(f))


def numero_finito(valor, contexto):
    try:
        x = float(valor)
    except (TypeError, ValueError):
        raise ValueError(f"valor numérico inválido em {contexto}: {valor!r}")
    if not math.isfinite(x):
        raise ValueError(f"valor não finito em {contexto}: {valor!r}")
    return x



def le_configuracao():
    caminho = os.path.join(RES, "config_experimento.txt")
    if not os.path.isfile(caminho):
        raise ValueError("config_experimento.txt ausente; execute a bateria de desempenho completa.")

    config = {}
    with open(caminho, encoding="utf-8") as f:
        for linha in f:
            linha = linha.strip()
            if linha and "=" in linha:
                chave, valor = linha.split("=", 1)
                config[chave.strip()] = valor.strip()

    try:
        config["versoes_lista"] = config["versoes"].split()
        config["tamanhos_lista"] = [int(x) for x in config["tamanhos"].split()]
        config["n_max_densa_int"] = int(config["n_max_densa"])
        config["reps_int"] = int(config["reps"])
        config["max_int"] = int(config["max"])
        config["epsilon_float"] = float(config["epsilon"])
        config["x0_float"] = float(config["x0"])
    except (KeyError, ValueError):
        raise ValueError("config_experimento.txt ausente ou com parâmetros inválidos.")

    if set(config["versoes_lista"]) != VERSOES_VALIDAS:
        raise ValueError("A geração dos gráficos exige as quatro versões do projeto.")
    if config["tamanhos_lista"] != TAMANHOS_OFICIAIS:
        raise ValueError("Para os gráficos finais, use todos os N exigidos pelo trabalho.")
    if config["n_max_densa_int"] != 2000 or config["reps_int"] != 10:
        raise ValueError("A configuração final exige N_MAX_DENSA=2000 e REPS=10.")
    if config["max_int"] != 25 or config["epsilon_float"] != 0.0 or config["x0_float"] != 0.0:
        raise ValueError("A configuração final exige x0=0, epsilon=0 e MAX=25.")
    return config


def tamanhos_esperados(config, versao):
    return [
        n for n in config["tamanhos_lista"]
        if versao != "v0_densa" or n <= config["n_max_densa_int"]
    ]


def valida_tempos(tempos, config):
    if not tempos:
        raise ValueError("tempos.csv está vazio.")

    dados = defaultdict(lambda: defaultdict(list))
    repeticoes = defaultdict(set)
    tamanhos_permitidos = set(config["tamanhos_lista"])
    reps_esperadas = set(range(1, config["reps_int"] + 1))
    for i, r in enumerate(tempos, start=2):
        v = r.get("versao", "")
        if v not in VERSOES_VALIDAS:
            raise ValueError(f"versão desconhecida em tempos.csv, linha {i}: {v!r}")
        try:
            n = int(r["N"])
        except (KeyError, TypeError, ValueError):
            raise ValueError(f"N inválido em tempos.csv, linha {i}")
        if n <= 0 or n not in tamanhos_permitidos:
            raise ValueError(f"N inesperado em tempos.csv, linha {i}: {n}")
        try:
            rep = int(r["rep"])
        except (KeyError, TypeError, ValueError):
            raise ValueError(f"repetição inválida em tempos.csv, linha {i}")
        if rep not in reps_esperadas or rep in repeticoes[(v, n)]:
            raise ValueError(f"repetição ausente, duplicada ou fora do intervalo na linha {i}")
        repeticoes[(v, n)].add(rep)

        for _, _, coluna, _ in TRECHOS:
            x = numero_finito(r.get(coluna), f"tempos.csv linha {i}, coluna {coluna}")
            if x <= 0:
                raise ValueError(
                    f"tempo deve ser positivo para escala logarítmica: linha {i}, {coluna}={x}"
                )
            dados[(v, coluna)][n].append(x)

    for v, _, _, _ in VERSOES:
        for n in tamanhos_esperados(config, v):
            if repeticoes[(v, n)] != reps_esperadas:
                raise ValueError(
                    f"Tempos incompletos para {v}, N={n}: "
                    f"{len(repeticoes[(v, n)])}/{config['reps_int']} repetições."
                )
        for _, _, coluna, _ in TRECHOS:
            if not dados[(v, coluna)]:
                raise ValueError(f"Não há tempos válidos para {v}, trecho {coluna}.")
    return dados


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


def salva(fig, nome, gerados):
    os.makedirs(OUT, exist_ok=True)
    caminho = os.path.join(OUT, nome + ".png")
    fig.savefig(caminho, dpi=150)
    plt.close(fig)
    gerados.add(nome + ".png")
    print("gerado:", nome + ".png")


def graficos_tempo(dados, gerados):
    # Tabela de médias e desvios padrão por versão, N e trecho.
    caminho_tabela = os.path.join(RES, "tabela_tempos.csv")
    with open(caminho_tabela, "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f, lineterminator="\n")
        w.writerow(["versao", "N", "trecho", "media_ms", "desvio_ms", "reps"])
        for (v, coluna), por_n in sorted(dados.items()):
            for n, valores in sorted(por_n.items()):
                w.writerow([
                    v, n, coluna, f"{mean(valores):.6f}",
                    f"{pstdev(valores):.6f}", len(valores)
                ])

    for tid, _, coluna, titulo in TRECHOS:
        fig, ax = plt.subplots(figsize=(6.5, 4.2))
        for v, rotulo, cor, marcador in VERSOES:
            por_n = dados[(v, coluna)]
            ns = sorted(por_n)
            medias = [mean(por_n[n]) for n in ns]
            desvios = [pstdev(por_n[n]) for n in ns]
            ax.errorbar(
                ns, medias, yerr=desvios, label=rotulo, color=cor,
                marker=marcador, markersize=4, capsize=2, linewidth=1.3
            )
        finaliza(ax, "Tempo de execução - " + titulo, "Tempo médio (ms)", ylog=True)
        salva(fig, f"tempo_{tid}", gerados)


def seleciona(tipo, grupo, metrica):
    m = " ".join(metrica.lower().split())

    if tipo == "banda":
        return (
            (grupo == "MEM" and m.startswith("memory bandwidth [mbytes/s]"))
            or (grupo == "L3" and m.startswith("l3 bandwidth [mbytes/s]"))
        )
    if tipo == "cache":
        if grupo == "L2CACHE":
            return m.startswith("l2") and "miss ratio" in m
        if grupo == "CACHE":
            return (m.startswith("cache") or m.startswith("data cache")) and "miss ratio" in m
        if grupo == "L1CACHE":
            return m.startswith("l1") and "miss ratio" in m
    if tipo == "flops":
        if grupo == "FLOPS_DP":
            return m.startswith("dp") and ("mflop/s" in m or "mflops/s" in m)
        if grupo == "FLOPS_AVX":
            return "dp" in m and ("mflop/s" in m or "mflops/s" in m) and ("packed" in m or m.startswith("avx"))
    return False


def valida_likwid(lik):
    if not lik:
        raise ValueError("likwid.csv está vazio.")

    tipos = ("banda", "cache", "flops")
    dados = {tipo: defaultdict(dict) for tipo in tipos}

    for i, r in enumerate(lik, start=2):
        v = r.get("versao", "")
        grupo = r.get("grupo", "")
        regiao = r.get("regiao", "")
        if v not in VERSOES_VALIDAS:
            raise ValueError(f"versão desconhecida em likwid.csv, linha {i}: {v!r}")
        if regiao not in REGIOES_VALIDAS:
            raise ValueError(f"região desconhecida em likwid.csv, linha {i}: {regiao!r}")
        try:
            n = int(r["N"])
        except (KeyError, TypeError, ValueError):
            raise ValueError(f"N inválido em likwid.csv, linha {i}")
        if n <= 0:
            raise ValueError(f"N deve ser positivo em likwid.csv, linha {i}")
        valor = numero_finito(r.get("valor"), f"likwid.csv linha {i}")

        for tipo in tipos:
            if seleciona(tipo, grupo, r.get("metrica", "")):
                dados[tipo][(v, grupo, regiao)][n] = valor

    for tipo in tipos:
        for _, regiao, _, _ in TRECHOS:
            for v, _, _, _ in VERSOES:
                encontrados = [
                    por_n for (vv, _g, rr), por_n in dados[tipo].items()
                    if vv == v and rr == regiao and por_n
                ]
                if not encontrados:
                    raise ValueError(
                        f"Dados LIKWID ausentes: métrica={tipo}, versão={v}, região={regiao}."
                    )
                if tipo == "flops":
                    grupos = {
                        g for (vv, g, rr), por_n in dados[tipo].items()
                        if vv == v and rr == regiao and por_n
                    }
                    if not {"FLOPS_DP", "FLOPS_AVX"}.issubset(grupos):
                        raise ValueError(
                            f"Faltam FLOPS_DP ou FLOPS_AVX: versão={v}, região={regiao}."
                        )
    return dados


def graficos_likwid(dados, gerados):
    for tipo, titulo_metrica, ylabel in (
        ("banda", "Banda de memória", "Banda [MBytes/s]"),
        ("cache", "Taxa de cache miss", "Cache miss ratio"),
        ("flops", "Operações aritméticas", "Desempenho [MFLOP/s]"),
    ):
        for tid, regiao, _, titulo_regiao in TRECHOS:
            fig, ax = plt.subplots(figsize=(6.5, 4.2))
            for v, rotulo, cor, marcador in VERSOES:
                chaves = sorted(
                    (chave for chave in dados[tipo] if chave[0] == v and chave[2] == regiao),
                    key=lambda chave: chave[1]
                )
                for chave in chaves:
                    _, grupo, _ = chave
                    por_n = dados[tipo][chave]
                    ns = sorted(por_n)
                    estilo = "--" if grupo == "FLOPS_AVX" else "-"
                    legenda = rotulo + (f" [{grupo}]" if tipo == "flops" else "")
                    ax.plot(
                        ns, [por_n[n] for n in ns], estilo, color=cor,
                        marker=marcador, markersize=4, linewidth=1.3, label=legenda
                    )

            if tipo == "banda":
                grupos = {g for (_v, g, r) in dados[tipo] if r == regiao}
                ylabel_atual = "Banda L3 [MBytes/s]" if grupos == {"L3"} else ylabel
            elif tipo == "cache":
                grupos = {g for (_v, g, r) in dados[tipo] if r == regiao}
                ylabel_atual = f"Taxa de cache miss ({', '.join(sorted(grupos))})"
            else:
                ylabel_atual = ylabel

            finaliza(ax, f"{titulo_metrica} - {titulo_regiao}", ylabel_atual)
            salva(fig, f"{tipo}_{tid}", gerados)


def main():
    os.makedirs(OUT, exist_ok=True)
    # Remove saídas antigas antes da validação para que uma execução incompleta
    # nunca deixe gráficos de uma coleta anterior parecendo atuais.
    for nome in os.listdir(OUT):
        if nome.endswith(".png"):
            os.remove(os.path.join(OUT, nome))
    tabela_antiga = os.path.join(RES, "tabela_tempos.csv")
    if os.path.exists(tabela_antiga):
        os.remove(tabela_antiga)

    try:
        config = le_configuracao()
        tempos = valida_tempos(le_csv("tempos.csv"), config)
        lik = valida_likwid(le_csv("likwid.csv"))
    except (OSError, ValueError, KeyError) as exc:
        print(f"ERRO: {exc}", file=sys.stderr)
        return 1

    gerados = set()
    graficos_tempo(tempos, gerados)
    graficos_likwid(lik, gerados)

    if len(gerados) != 12:
        print(f"ERRO: foram gerados {len(gerados)} gráficos, mas são esperados 12.", file=sys.stderr)
        return 1

    print("SUCESSO: os 12 gráficos foram gerados com dados válidos.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
