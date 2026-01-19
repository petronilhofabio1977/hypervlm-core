#!/usr/bin/env python3
# Combina prompts + teacher outputs em pares, tokeniza com bpe_cpp.model e gera tiny.txt com N inteiros por linha.
import json, sys, subprocess, os
sp_model = "bpe_cpp.model"
prompts_file = "data/distill_prompts.jsonl"
teacher_file = "data/teacher_outputs.jsonl"
out_tokens = "tiny.txt"
max_tokens_per_example = 16  # seu run_train espera 4 ints; aqui aproximamos; ajuste se quiser
if not os.path.exists(sp_model):
    print("ERRO: tokenizador bpe_cpp.model não encontrado.", file=sys.stderr); sys.exit(1)

# carregar teacher outputs into dict by short prompt prefix
teacher = []
with open(teacher_file,"r",errors="ignore") as f:
    for L in f:
        try:
            teacher.append(json.loads(L))
        except: pass

# simple pairing: iterate prompts and pick corresponding first teacher response occurrence
pairs = []
with open(prompts_file,"r",errors="ignore") as f:
    for L in f:
        try:
            pj = json.loads(L)
        except: continue
        prompt = pj["prompt"]
        # find teacher resp that contains this prompt (heuristic)
        resp = None
        for t in teacher:
            if t.get("prompt","").strip()[:120] == prompt.strip()[:120]:
                resp = t.get("response","")
                break
        if not resp and teacher:
            resp = teacher[0].get("response","")
        if resp:
            pairs.append((prompt, resp))

# tokenizar usando sentencepiece (chama API via subprocess for consistent int outputs)
import sentencepiece as spm
sp = spm.SentencePieceProcessor(model_file=sp_model)

with open(out_tokens,"w") as fo:
    for p,r in pairs:
        # create an input: use prompt + separator + response, or encode only response depending on student objective
        seq = (p + "\n\n" + r).replace("\n", " ")
        ids = sp.encode(seq, out_type=int)
        # break into chunks of max_tokens_per_example
        for i in range(0, max(1,len(ids)), max_tokens_per_example):
            chunk = ids[i:i+max_tokens_per_example]
            if len(chunk) < 4:
                # pad or skip; we will pad to length 4 as run_train originally expects 4 ints per line
                while len(chunk) < 4:
                    chunk.append(0)
            fo.write(" ".join(map(str, chunk[:4])) + "\n")
print(f"Wrote tokenized dataset -> {out_tokens} (examples: {len(pairs)})")
