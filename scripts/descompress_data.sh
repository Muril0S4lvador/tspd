#!/bin/bash

TEMPO_INICIO=$SECONDS

DIR_SCRIPT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$DIR_SCRIPT/../data" || exit 1

diretorio_atual="$(pwd)"
DATA_FILE="ALL_tsp.tar.gz"
PASTA_DESTINO="$diretorio_atual/descompressed"

if [ ! -f "$DATA_FILE" ]; then
    echo "Erro: O arquivo '$diretorio_atual/$DATA_FILE' não existe."
    exit 1
fi

mkdir -p "$PASTA_DESTINO"

echo "=================================================="
echo "1/3 Extraindo arquivo principal ($DATA_FILE)..."
tar -xf "$DATA_FILE" -C "$PASTA_DESTINO"

cd "$PASTA_DESTINO" || exit 1

echo "2/3 Organizando pastas..."
# Mapeia e organiza todos os arquivos em subpastas em um único loop rápido
for arquivo in *; do
    if [ -f "$arquivo" ]; then
        NOME_PASTA="${arquivo%%.*}"
        if [ -n "$NOME_PASTA" ]; then
            mkdir -p "$NOME_PASTA"
            mv "$arquivo" "$NOME_PASTA/"
        fi
    fi
done

echo "3/3 Descompactando todos os arquivos .gz em LOTE..."

# Executa o gzip em LOTE aproveitando o paralelismo/busca rápida do sistema de arquivos
# O comando 'find' passa TODOS os .gz de uma só vez para o gzip
find . -type f -name "*.gz" -exec gzip -df {} + 2>/dev/null

TEMPO_TOTAL=$(( SECONDS - TEMPO_INICIO ))
MINUTOS=$(( TEMPO_TOTAL / 60 ))
SEGUNDOS=$(( TEMPO_TOTAL % 60 ))

echo -e "\n=================================================="
echo "Processo concluído com sucesso!"
printf "Tempo total de execução: %02dm %02ds\n" $MINUTOS $SEGUNDOS
echo "=================================================="