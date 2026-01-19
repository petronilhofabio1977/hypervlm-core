#!/bin/bash

OUTDIR="dataset_text"
mkdir -p "$OUTDIR"

echo "🔎 Procurando PDFs..."
find ./Livros ./Compiladores -type f -name "*.pdf" | while read pdf; do
    name=$(basename "$pdf")
    txt="$OUTDIR/${name%.pdf}.txt"

    echo "📘 Extraindo: $pdf"
    pdftotext "$pdf" "$txt"

    # Limpar linhas vazias
    sed -i '/^[[:space:]]*$/d' "$txt"
done

echo "📂 Combinando todos os textos..."
cat $OUTDIR/*.txt > dataset_all.txt

echo "✅ Finalizado! dataset_all.txt criado."
