# Trabalho: Sistemas Não-Lineares de Broyden (Método de Newton) — CI1164

> Renomeie esta pasta para `login1-login2` (seus logins do DINF) antes de entregar
> e gere o pacote com `zip -r login1-login2.zip login1-login2` (ou `tar`).
> O relatório final deve ser `relatorio-login1-login2.pdf` nesta pasta (ver `relatorio/`).

## Dependências

Para compilar e testar o código:
- GCC, GNU Make, Bash e `taskset` (pacote `util-linux`);
- Python 3.

Para processar e visualizar os resultados:
- Python 3 com `matplotlib`;
- LIKWID instalado e configurado para a arquitetura do computador, com acesso autorizado aos contadores de hardware.

Para compilar o artigo:
- `pdflatex` e os pacotes LaTeX usados pelo relatório.

Em Debian/Ubuntu, as dependências gerais podem ser instaladas com:

```sh
sudo apt update
sudo apt install build-essential python3 python3-matplotlib util-linux \
    texlive-latex-base texlive-latex-recommended texlive-latex-extra \
    texlive-lang-portuguese
```

O LIKWID deve ser instalado separadamente conforme a distribuição e a arquitetura da máquina. A coleta não altera permissões do sistema: se o usuário não tiver acesso autorizado aos contadores, a execução deve ser feita com o suporte do administrador.

## Início rápido

```sh
make                    # gera ./broyden (versão MELHOR = v3_reciproco)
echo "10 0 1e-8 25" | ./broyden      # N=10, x0=0, eps=1e-8, MAX=25
make teste              # compila as 4 versões e confere corretude/concordância
```

Entrada (stdin): `N x0 epsilon MAX`. Saída: todas as iterações (`x1 = ...`, separadas por `#`),
seguidas de `###########`, dos 3 tempos em ms (Newton total, Jacobianas, sistemas lineares) e de
outro `###########`.

> **Sobre x0:** com `x0 <= 0` (por exemplo 0 ou -1) o Newton converge para todos os N testados.
> Com `x0` positivo e N grande o método pode divergir / ficar singular (isso ocorre igualmente em todas as
> versões: é propriedade do método e do chute inicial, não do código) e o programa encerra com
> "Falha numerica". Use `x0 = 0` (padrão do script) nos testes de desempenho.

## Mapa dos arquivos

```
login1-login2/
├── Makefile                  regras: all, clean (obrigatórias) + versoes, likwid, teste, testes-desempenho, graficos
├── README.md                 este arquivo
├── common/                   módulos IGUAIS em todas as versões
│   ├── main.c                programa principal (só main(): lê entrada, chama newton(), imprime)
│   ├── newton.h              interface do Método de Newton (assinatura única para todas as versões)
│   ├── utils.c / utils.h     timestamp() em ms (gettimeofday) e macros LIKWID (viram no-op sem -DLIKWID_PERFMON)
├── v0_densa/                 VERSÃO BASE (SEM OTIMIZAÇÃO)
├── v1_tridiag/               versão intermediária = seu código "atual" (tridiagonal + pivoteamento)
├── v2_otimizada/             versão OTIMIZADA
├── v3_reciproco/             versão OTIMIZADA + inverso do pivô  (é a copiada para ./broyden)
├── scripts/                  verificação, coleta LIKWID, processamento e gráficos
├── relatorio/                esqueleto do artigo em LaTeX (relatorio.tex)
└── resultados/               gerado pelos scripts (tempos.csv, likwid.csv, topologia.txt, graficos/)
```

Cada pasta `vN_*` contém os mesmos 6 módulos, com a mesma interface de `newton.h`:
`funcao.{c,h}` (F de Broyden), `jacobiana.{c,h}` (estrutura da Jacobiana e seu cálculo),
`solver.{c,h}` (resolução do sistema linear interno) e `newton.c` (laço do método).

### As versões

| Pasta | Papel | Matriz | Solver | Principais características |
|---|---|---|---|---|
| `v0_densa` | **BASE, sem otimização** | `double **` densa n×n, linhas separadas | Gauss com pivoteamento parcial, O(n³) | Jacobiana O(n²) (zera tudo a cada iteração). Só é executada até `N_MAX_DENSA=2000` (n³ e n² de memória). |
| `v1_tridiag` | Intermediária (código que você já tinha) | 3 vetores `a,d,c` | Gauss tridiagonal com pivoteamento entre linhas adjacentes, O(n) | `calloc/free` de `du2` a cada iteração; 3 passadas `isfinite`; vetores `F`, `b` separados; sem `restrict`. |
| `v2_otimizada` | **Otimizada** | 4 vetores alinhados (64 B) `a,d,c,du2` | idem v1, enxuto | ver lista abaixo |
| `v3_reciproco` | **Otimizada + extra** | idem v2 + `inv` | idem v2 com 1 divisão por linha | troca divisões por multiplicações pelo inverso do pivô |

Otimizações aplicadas em `v2_otimizada` (e herdadas por `v3`), com o arquivo onde estão:

| Técnica (aula) | Onde | Efeito |
|---|---|---|
| Estrutura de dados esparsa (matriz de banda, Parte 5) | `jacobiana.*`, `solver.*` | memória O(n) em vez de O(n²); solver O(n) em vez de O(n³) (já em v1) |
| `restrict` / sem aliasing (Partes 7a, 9) | `funcao.*`, `jacobiana.*`, `solver.c`, `newton.c` | o GCC vetoriza sem versionar laços por possível alias (veja `-fopt-info-vec`) |
| Laços simples, passo unitário, sem desvios → SIMD/AVX (Parte 7a) | `calculaMenosF`, `calculaJacobiana` | vetorizados com vetores de 32 bytes |
| Subdiagonal constante inicializada 1 vez | `alocaTriDiag` | a Jacobiana só reescreve `d` e `c`; o solver não toca em `a` |
| Eliminação de subexpressões / menos trabalho (Parte 9) | `calculaMenosF` | `x(2x-3)` em vez de `-2x²+3x`; calcula direto `b = -F` (sem vetor F e sem cópia) |
| Fusão de laços (Parte 10) | `newton.c` (`normaEFinito`, `atualizaX`) | norma + checagem de finitude em 1 passada; `x+Δ` + norma de Δ + finitude em 1 passada |
| Evitar alocação no laço (Parte 9) | `solver.c`, `jacobiana.c` | `du2` pré-alocado (sem `calloc/free` por iteração) |
| Evitar desvios / testes redundantes (Parte 9) | `solver.c` | sem `isfinite` na fatoração, 1 passada para a escala, sem testes de pivô repetidos na retrossubstituição |
| Alinhamento de memória (Parte 7a/9) | `alocaAlinhado` (`aligned_alloc(64)`) | cargas/armazenamentos AVX alinhados nos vetores de trabalho |
| Evitar divisão (pipelining, Parte 7a) | `v3_reciproco/solver.c` | DIV tem latência alta e estava no caminho crítico da retrossubstituição |

Por que **não** há padding de potência de 2 / blocking: todos os acessos têm passo unitário e a memória
é O(n), então os problemas de cache thrashing e de reuso de blocos (Parte 10) não se aplicam.

## Como medir e gerar os gráficos

1. Compile e confira: `make versoes && make teste`.
2. Compile as versões instrumentadas: `make likwid LIKWID=/usr/local` (no DINF: `LIKWID=/home/soft/likwid`).
3. **Condições do teste** (enunciado): máquina própria/exclusiva (não as servidoras compartilhadas do DINF),
   mesma máquina para compilar e testar, mesmas flags (`-O3 -march=native -mavx -fopt-info-vec`).
   Fixe a frequência da CPU (ex.: `sudo cpupower frequency-set -g performance`, ou `likwid-setFrequencies`),
   feche programas pesados e carregue o módulo MSR (`sudo modprobe msr`).
4. Rode a bateria: `REPS=10 bash scripts/rodar_testes.sh` (ou `make testes-desempenho LIKWID=/usr/local REPS=10`). O script escolhe automaticamente o primeiro núcleo permitido; para fixar outro, informe `CORE=N`. Cada nova bateria limpa os resultados gerados anteriormente para evitar misturar coletas. Se `likwid-perfctr` estiver em `/usr/bin`, use o prefixo `LIKWID=/usr`.
   Parâmetros fixos: `MAX=25`, `epsilon=0`, `N = {32, 64, 128, 256, 512, 1000, 2000, 4000, 8000, 9000, 10000, 20000}`.
5. Gere os gráficos: `make graficos`. A geração final exige a bateria completa: 10 repetições e todos os N oficiais; execuções parciais servem apenas para depuração.

### Onde ficam os resultados

| Arquivo | Conteúdo |
|---|---|
| `resultados/config_experimento.txt` | configuração usada na bateria (versões, N, repetições, núcleo e grupos LIKWID) |
| `resultados/topologia.txt` | saída de `likwid-topology -c -g` (arquitetura a descrever no relatório) |
| `resultados/tempos.csv` | tempos brutos (cada repetição): total, Jacobianas, sistemas lineares (ms) |
| `resultados/tabela_tempos.csv` | média e desvio padrão por versão/N/trecho |
| `resultados/likwid/*.txt` | saída bruta do `likwid-perfctr` (uma por versão × grupo × N) |
| `resultados/likwid.csv` | métricas extraídas (versão, N, grupo, região, métrica, valor) |
| `resultados/graficos/*.png` | os **12 gráficos**, abaixo |

Gráficos (`<teste>_<trecho>.png`): testes `tempo`, `banda` (grupo MEM, ou L3), `cache` (L2CACHE, ou CACHE/L1CACHE)
e `flops` (FLOPS_DP linha contínua e FLOPS_AVX tracejada); trechos `newton` (execução total), `jacobiana`
e `sistema_linear`. Eixo N em escala logarítmica (e eixo do tempo também). Uma linha de cor distinta por versão.

## Scripts (`scripts/`)

| Arquivo | Função |
|---|---|
| `verifica.sh` / `verifica.py` | testes de corretude: formato da saída, nº de iterações, resíduo ‖F(x)‖ recalculado em Python e concordância entre as versões |
| `rodar_testes.sh` | bateria completa: tempos (sem LIKWID) + `likwid-perfctr` por grupo; escolhe automaticamente os grupos de fallback (MEM→L3, L2CACHE→CACHE→L1CACHE) |
| `processa_likwid.py` | converte as saídas brutas do LIKWID em `resultados/likwid.csv` |
| `gera_graficos.py` | gera os 12 gráficos e a tabela de tempos (requer `matplotlib`) |

## Relatório

`relatorio/relatorio.tex` é um esqueleto de artigo científico com texto das otimizações já redigido e os
12 gráficos incluídos automaticamente de `resultados/graficos/`. Preencha os trechos marcados com `[PREENCHER]`
(arquitetura, análise de cada gráfico) e compile com `cd relatorio && pdflatex relatorio.tex` (duas vezes);
copie o PDF final para `../relatorio-login1-login2.pdf`.

## Observações

- As macros `LIKWID_MARKER_*` ficam em `common/utils.h` e viram no-op sem `-DLIKWID_PERFMON`, então
  `make` funciona sem o LIKWID instalado. Em `common/main.c`, a inicialização inclui `LIKWID_MARKER_INIT`
  e `LIKWID_MARKER_THREADINIT` antes do registro das regiões. Os marcadores são `NEWTON` (laço todo),
  `JACOBIANA` e `SISTEMA_LINEAR` (regiões internas). A API do LIKWID aceita regiões aninhadas; o script
  confere se as três aparecem na saída e interrompe a bateria se alguma estiver ausente.
- O tempo `NEWTON` cobre o laço iterativo após alocações/inicialização dos vetores; a impressão das iterações fica fora da região medida. `timestamp()` retorna milissegundos.
- Os arquivos foram normalizados para finais de linha Unix (LF).
