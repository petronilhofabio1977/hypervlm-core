import sentencepiece as spm
import sys

if len(sys.argv) < 3:
    print("Uso: python3 encode_bpe.py <modelo_bpe> <arquivo_txt>")
    sys.exit(1)

model = sys.argv[1]
file = sys.argv[2]

sp = spm.SentencePieceProcessor(model_file=model)

with open(file, "r", encoding="utf-8") as f:
    for line in f:
        line = line.strip()
        if not line:
            print("")
            continue
        tokens = sp.encode(line, out_type=int)
        print(" ".join(map(str, tokens)))
