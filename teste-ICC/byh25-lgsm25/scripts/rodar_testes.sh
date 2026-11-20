#!/bin/bash
# ============================================================================
# rodar_testes.sh - bateria completa de testes de desempenho.
#
# Para cada versão e cada N:
#   (1) TEMPO: executa REPS vezes o binário SEM instrumentação e grava os 3
#       tempos (total, Jacobianas, sistemas lineares) em resultados/tempos.csv;
#   (2) LIKWID: executa 1 vez o binário instrumentado sob likwid-perfctr para
#       cada grupo (MEM ou L3; L2CACHE ou CACHE; FLOPS_DP; FLOPS_AVX), com as
#       três regiões marcadas (NEWTON, JACOBIANA, SISTEMA_LINEAR), e guarda a
#       saída bruta em resultados/likwid/<versao>_<grupo>_N<N>.txt.
#
# Parâmetros fixos exigidos pelo enunciado: MAX=25, epsilon=0.
#
# Variáveis de ambiente (todas opcionais):
#   CORE=2          núcleo físico onde rodar (use um núcleo ocioso)
#   REPS=10         repetições da medida de tempo
#   X0=0            valor inicial das variáveis
#   LIKWID=/usr/local   prefixo do LIKWID (no DINF: /home/soft/likwid)
#   VERSOES="..."   versões a testar
#   TAMANHOS="..."  valores de N
#   N_MAX_DENSA=2000  maior N testado na v0 (O(n^3) e O(n^2) de memória)
#
# ANTES de rodar: máquina exclusiva (nada pesado em paralelo), frequência da
# CPU fixa (ver README) e 'sudo modprobe msr' se rodar em casa.
# ============================================================================
cd "$(dirname "$0")/.." || exit 1

CORE=${CORE:-2}
REPS=${REPS:-10}
X0=${X0:-0}
MAXIT=25
EPS=0
LIKWID=${LIKWID:-/usr/local}
VERSOES=${VERSOES:-"v0_densa v1_tridiag v2_otimizada v3_reciproco"}
TAMANHOS=${TAMANHOS:-"32 64 128 256 512 1000 2000 4000 8000 9000 10000 20000"}
N_MAX_DENSA=${N_MAX_DENSA:-2000}

export PATH="$LIKWID/bin:$LIKWID/sbin:$PATH"
export LD_LIBRARY_PATH="$LIKWID/lib:$LD_LIBRARY_PATH"

mkdir -p resultados/likwid
CSV=resultados/tempos.csv
echo "versao,N,rep,t_total_ms,t_jacobiana_ms,t_sistema_linear_ms" > "$CSV"

# --- escolha dos grupos (com os fallbacks previstos no enunciado) -----------
tem_grupo() { likwid-perfctr -a 2>/dev/null | awk '{print $1}' | grep -qx "$1"; }
G_MEM=""; for g in MEM L3; do tem_grupo $g && { G_MEM=$g; break; }; done
G_L2="";  for g in L2CACHE CACHE L1CACHE; do tem_grupo $g && { G_L2=$g; break; }; done
GRUPOS=""
for g in "$G_MEM" "$G_L2" FLOPS_DP FLOPS_AVX; do
    if [ -n "$g" ] && tem_grupo "$g"; then GRUPOS="$GRUPOS $g"; else echo "AVISO: grupo '$g' indisponível nesta CPU"; fi
done
echo "Grupos LIKWID usados:$GRUPOS"
echo "Núcleo: $CORE | repetições: $REPS | x0: $X0 | MAX=$MAXIT eps=$EPS"
likwid-topology -c -g > resultados/topologia.txt 2>&1 && echo "Topologia salva em resultados/topologia.txt"

for v in $VERSOES; do
  for n in $TAMANHOS; do
    if [ "$v" = "v0_densa" ] && [ "$n" -gt "$N_MAX_DENSA" ]; then
        echo "[$v] N=$n ignorado (> N_MAX_DENSA=$N_MAX_DENSA)"; continue
    fi
    ENT="$n $X0 $EPS $MAXIT"

    # (1) tempos: binário sem LIKWID, fixo no núcleo $CORE
    for r in $(seq 1 "$REPS"); do
        saida=$(echo "$ENT" | taskset -c "$CORE" "bin/broyden_$v") || { echo "ERRO em $v N=$n"; break; }
        tempos=$(echo "$saida" | awk '/^###########$/{c++; next} c==1{printf "%s%s", (k++?",":""), $1}')
        echo "$v,$n,$r,$tempos" >> "$CSV"
    done

    # (2) contadores: binário instrumentado, 1 execução por grupo
    for g in $GRUPOS; do
        echo "$ENT" | likwid-perfctr -C "$CORE" -g "$g" -m "bin/broyden_${v}_likwid" \
            > "resultados/likwid/${v}_${g}_N${n}.txt" 2> "resultados/likwid/${v}_${g}_N${n}.err" \
            || echo "AVISO: likwid-perfctr falhou em $v $g N=$n (veja o .err)"
    done
    echo "[$v] N=$n concluído"
  done
done
echo "Pronto. Próximo passo: make graficos"
