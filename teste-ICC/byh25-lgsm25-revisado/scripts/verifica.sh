#!/bin/sh
# Confere corretude e concordância entre as versões (usa bin/broyden_<v>).
cd "$(dirname "$0")/.." || exit 1
[ -x bin/broyden_v2_otimizada ] || make versoes || exit 1
exec python3 scripts/verifica.py
