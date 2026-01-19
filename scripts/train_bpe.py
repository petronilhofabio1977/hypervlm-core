import sentencepiece as spm
import sys

if len(sys.argv) < 3:
    print("Uso: python3 train_bpe.py <input_text> <vocab_size>")
    sys.exit(1)

input_file = sys.argv[1]
vocab_size = int(sys.argv[2])

spm.SentencePieceTrainer.train(
    input=input_file,
    model_prefix="bpe",
    vocab_size=vocab_size,
    character_coverage=1.0,
    model_type="bpe",
    input_sentence_size=5000000,
    shuffle_input_sentence=True
)

print("BPE treinado! Arquivos gerados: bpe.model + bpe.vocab")
