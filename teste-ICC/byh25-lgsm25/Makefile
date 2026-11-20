# ============================================================================
# Makefile - Trabalho: Sistemas Não-Lineares de Broyden (Método de Newton)
#
# Regras obrigatórias:
#   make all    -> gera o executável ./broyden (versão MELHOR, abaixo)
#   make clean  -> remove tudo que foi gerado
#
# Regras extras:
#   make versoes -> gera bin/broyden_<versao> para todas as versões
#   make likwid  -> gera bin/broyden_<versao>_likwid (instrumentadas p/ LIKWID)
#   make teste   -> compila tudo e confere corretude/concordância entre versões
#   make testes-desempenho -> roda a bateria completa (scripts/rodar_testes.sh)
#   make graficos          -> processa resultados e gera os 12 gráficos
#
# Variáveis úteis (podem ser passadas na linha de comando):
#   MELHOR=v2_otimizada   versão copiada para ./broyden
#   LIKWID=/usr/local     prefixo da instalação do LIKWID
#                         (no DINF: LIKWID=/home/soft/likwid)
# ============================================================================

CC      = gcc
CFLAGS  = -O3 -march=native -mavx -fopt-info-vec -Wall -Wextra
LDLIBS  = -lm

LIKWID        ?= /home/soft/likwid
LIKWID_CFLAGS  = -DLIKWID_PERFMON -I$(LIKWID)/include
LIKWID_LIBS    = -L$(LIKWID)/lib -llikwid

# Versões do programa (cada uma é um diretório com seus próprios módulos).
VERSOES = v0_densa v1_tridiag v2_otimizada v3_reciproco
MELHOR ?= v3_reciproco

# Módulos comuns a todas as versões (ficam em common/).
COMUNS = main.c utils.c

.PHONY: all versoes likwid clean teste testes-desempenho graficos

all: broyden

broyden: bin/broyden_$(MELHOR)
	cp $< $@

versoes: $(addprefix bin/broyden_,$(VERSOES))

likwid: $(addprefix bin/broyden_,$(addsuffix _likwid,$(VERSOES)))

bin:
	mkdir -p bin

# --- Gabarito de compilação -------------------------------------------------
# $(1) = nome do binário   $(2) = diretório da versão
# $(3) = flags extras      $(4) = bibliotecas extras
define PROGRAMA
OBJ_$(1) := $(addprefix build/$(1)/,$(COMUNS:.c=.o) $(patsubst %.c,%.o,$(notdir $(wildcard $(2)/*.c))))

build/$(1):
	mkdir -p $$@

build/$(1)/%.o: common/%.c | build/$(1)
	$$(CC) $$(CFLAGS) $(3) -Icommon -I$(2) -MMD -MP -c $$< -o $$@

build/$(1)/%.o: $(2)/%.c | build/$(1)
	$$(CC) $$(CFLAGS) $(3) -Icommon -I$(2) -MMD -MP -c $$< -o $$@

bin/$(1): $$(OBJ_$(1)) | bin
	$$(CC) $$^ -o $$@ $(4) $$(LDLIBS)

-include $$(OBJ_$(1):.o=.d)
endef

$(foreach v,$(VERSOES),$(eval $(call PROGRAMA,broyden_$(v),$(v),,)))
$(foreach v,$(VERSOES),$(eval $(call PROGRAMA,broyden_$(v)_likwid,$(v),$$(LIKWID_CFLAGS),$$(LIKWID_LIBS))))

# --- Testes e experimentos ---------------------------------------------------
teste: versoes
	scripts/verifica.sh

testes-desempenho: versoes likwid
	scripts/rodar_testes.sh

graficos:
	python3 scripts/processa_likwid.py
	python3 scripts/gera_graficos.py

clean:
	rm -rf build bin broyden *.o *.d
