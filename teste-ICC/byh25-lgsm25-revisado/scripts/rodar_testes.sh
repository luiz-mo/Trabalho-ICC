#!/usr/bin/env bash
# Bateria de testes de desempenho para as quatro versões do SNLB de Broyden.
# Parâmetros fixos do trabalho: MAX=25 e epsilon=0.

set -uo pipefail
cd "$(dirname "$0")/.." || exit 1

erro() {
    printf 'ERRO: %s\n' "$*" >&2
    exit 1
}

aviso() {
    printf 'AVISO: %s\n' "$*" >&2
}

# Configuração geral.
REPS=${REPS:-10}
X0=${X0:-0}
MAXIT=25
EPS=0
LIKWID=${LIKWID:-/usr/local}
VERSOES=${VERSOES:-"v0_densa v1_tridiag v2_otimizada v3_reciproco"}
TAMANHOS=${TAMANHOS:-"32 64 128 256 512 1000 2000 4000 8000 9000 10000 20000"}
N_MAX_DENSA=${N_MAX_DENSA:-2000}

[[ "$REPS" =~ ^[1-9][0-9]*$ ]] || erro "REPS deve ser um inteiro positivo."
[[ "$N_MAX_DENSA" =~ ^[1-9][0-9]*$ ]] || erro "N_MAX_DENSA deve ser um inteiro positivo."
command -v taskset >/dev/null 2>&1 || erro "taskset não encontrado."

# Aceita LIKWID como prefixo (/usr/local ou /usr), diretório bin (/usr/bin)
# ou usa os executáveis já disponíveis no PATH.
if [[ -x "$LIKWID/bin/likwid-perfctr" ]]; then
    LIKWID_BIN="$LIKWID/bin"
elif [[ -x "$LIKWID/likwid-perfctr" ]]; then
    LIKWID_BIN="$LIKWID"
else
    LIKWID_BIN=""
fi

if [[ -n "$LIKWID_BIN" ]]; then
    PATH="$LIKWID_BIN:$PATH"
    export PATH
fi
if [[ -d "$LIKWID/lib" ]]; then
    LD_LIBRARY_PATH="$LIKWID/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
    export LD_LIBRARY_PATH
fi

PERFCTR=$(command -v likwid-perfctr) || erro "likwid-perfctr não encontrado. Ajuste LIKWID."
TOPOLOGY=$(command -v likwid-topology) || erro "likwid-topology não encontrado. Ajuste LIKWID."

# Usa automaticamente o primeiro CPU permitido, a menos que CORE seja informado.
if [[ -z "${CORE:-}" ]]; then
    lista_cpus=$(taskset -pc $$ 2>/dev/null | sed 's/.*: //')
    CORE=$(printf '%s\n' "$lista_cpus" | awk -F'[, -]' '{print $1}')
fi
[[ "${CORE:-}" =~ ^[0-9]+$ ]] || erro "Não foi possível determinar um núcleo permitido. Informe CORE."
taskset -c "$CORE" true >/dev/null 2>&1 || erro "CORE=$CORE não está permitido para este processo."

# Verifica os binários antes de iniciar qualquer experimento.
for v in $VERSOES; do
    [[ -x "bin/broyden_${v}" ]] || erro "Binário ausente: bin/broyden_${v}. Execute make versoes."
    [[ -x "bin/broyden_${v}_likwid" ]] || erro "Binário instrumentado ausente: bin/broyden_${v}_likwid. Execute make likwid."
done

# Consulta os grupos disponíveis e escolhe somente fallbacks documentados.
grupos_disponiveis=$("$PERFCTR" -a 2>/dev/null) || erro "Não foi possível consultar os grupos do LIKWID."
tem_grupo() {
    printf '%s\n' "$grupos_disponiveis" | awk '{print $1}' | grep -Fxq "$1"
}
escolhe_grupo() {
    local candidato
    for candidato in "$@"; do
        if tem_grupo "$candidato"; then
            printf '%s' "$candidato"
            return 0
        fi
    done
    return 1
}

G_MEM=$(escolhe_grupo MEM L3) || erro "Nenhum dos grupos MEM/L3 está disponível."
G_L2=$(escolhe_grupo L2CACHE CACHE L1CACHE) || erro "Nenhum grupo de cache disponível (L2CACHE/CACHE/L1CACHE)."
G_DP=$(escolhe_grupo FLOPS_DP) || erro "Grupo FLOPS_DP indisponível."
G_AVX=$(escolhe_grupo FLOPS_AVX) || erro "Grupo FLOPS_AVX indisponível."
GRUPOS="$G_MEM $G_L2 $G_DP $G_AVX"

mkdir -p resultados/likwid resultados/graficos || erro "Não foi possível preparar a pasta de resultados."

# Cada bateria começa limpa para impedir mistura com dados de execuções antigas.
rm -f resultados/likwid/*.txt resultados/likwid/*.err resultados/likwid.csv \
      resultados/tabela_tempos.csv resultados/graficos/*.png \
    || erro "Não foi possível limpar os resultados da bateria anterior."

"$TOPOLOGY" -c -g > resultados/topologia.txt 2>&1 \
    || erro "Falha ao obter a topologia. Consulte resultados/topologia.txt."

CSV=resultados/tempos.csv
printf 'versao,N,rep,t_total_ms,t_jacobiana_ms,t_sistema_linear_ms\n' > "$CSV" \
    || erro "Não foi possível criar $CSV."

CONFIG=resultados/config_experimento.txt
{
    printf 'versoes=%s\n' "$VERSOES"
    printf 'tamanhos=%s\n' "$TAMANHOS"
    printf 'n_max_densa=%s\n' "$N_MAX_DENSA"
    printf 'reps=%s\n' "$REPS"
    printf 'core=%s\n' "$CORE"
    printf 'x0=%s\n' "$X0"
    printf 'max=%s\n' "$MAXIT"
    printf 'epsilon=%s\n' "$EPS"
    printf 'grupo_mem=%s\n' "$G_MEM"
    printf 'grupo_cache=%s\n' "$G_L2"
    printf 'grupo_flops_dp=%s\n' "$G_DP"
    printf 'grupo_flops_avx=%s\n' "$G_AVX"
} > "$CONFIG" || erro "Não foi possível gravar $CONFIG."

printf 'LIKWID: %s\n' "$PERFCTR"
printf 'Grupos usados: MEM/L3=%s | cache=%s | FLOPS_DP=%s | FLOPS_AVX=%s\n' "$G_MEM" "$G_L2" "$G_DP" "$G_AVX"
printf 'Núcleo permitido: %s | repetições: %s | x0: %s | MAX=%s | epsilon=%s\n' "$CORE" "$REPS" "$X0" "$MAXIT" "$EPS"

falhas=0
for v in $VERSOES; do
    for n in $TAMANHOS; do
        if [[ "$v" == "v0_densa" && "$n" -gt "$N_MAX_DENSA" ]]; then
            printf '[%s] N=%s ignorado (> N_MAX_DENSA=%s)\n' "$v" "$n" "$N_MAX_DENSA"
            continue
        fi

        ENT="$n $X0 $EPS $MAXIT"
        binario="bin/broyden_${v}"

        # (1) Medidas de tempo sem instrumentação LIKWID.
        for ((r=1; r<=REPS; r++)); do
            if ! saida=$(printf '%s\n' "$ENT" | taskset -c "$CORE" "$binario"); then
                aviso "execução de tempo falhou: versão=$v N=$n repetição=$r"
                exit 1
            fi

            mapfile -t tempos < <(printf '%s\n' "$saida" | awk '/^###########$/ {c++; next} c==1 {print $1}')
            if [[ ${#tempos[@]} -ne 3 ]]; then
                aviso "saída de tempo inválida: versão=$v N=$n repetição=$r (esperados 3 tempos)"
                exit 1
            fi
            for valor in "${tempos[@]}"; do
                [[ "$valor" =~ ^[-+]?[0-9]*\.?[0-9]+([eE][-+]?[0-9]+)?$ ]] \
                    || erro "tempo inválido na saída: '$valor' (versão=$v N=$n)"
                awk -v x="$valor" 'BEGIN { exit !(x > 0) }' \
                    || erro "tempo não positivo na saída: '$valor' (versão=$v N=$n)"
            done
            printf '%s,%s,%s,%s,%s,%s\n' "$v" "$n" "$r" "${tempos[0]}" "${tempos[1]}" "${tempos[2]}" >> "$CSV"
        done

        # (2) Uma coleta LIKWID por grupo, com as três regiões marcadas.
        for g in $GRUPOS; do
            arquivo="resultados/likwid/${v}_${g}_N${n}.txt"
            erro_arquivo="resultados/likwid/${v}_${g}_N${n}.err"
            if printf '%s\n' "$ENT" | "$PERFCTR" -C "$CORE" -g "$g" -m "bin/broyden_${v}_likwid" \
                > "$arquivo" 2> "$erro_arquivo"; then
                if [[ ! -s "$arquivo" ]]; then
                    aviso "LIKWID não produziu saída: versão=$v grupo=$g N=$n"
                    falhas=1
                else
                    for regiao in NEWTON JACOBIANA SISTEMA_LINEAR; do
                        if ! grep -Eq "Region[[:space:]]+${regiao}," "$arquivo"; then
                            aviso "região $regiao ausente na saída LIKWID: versão=$v grupo=$g N=$n"
                            falhas=1
                        fi
                    done
                fi
            else
                aviso "LIKWID falhou: versão=$v grupo=$g N=$n (consulte $erro_arquivo)"
                falhas=1
            fi
        done
        printf '[%s] N=%s processado\n' "$v" "$n"
    done
done

if [[ "$falhas" -ne 0 ]]; then
    erro "Uma ou mais coletas LIKWID falharam. Não considere a bateria completa."
fi

printf 'Coleta concluída sem erros. Próximo passo: make graficos\n'
