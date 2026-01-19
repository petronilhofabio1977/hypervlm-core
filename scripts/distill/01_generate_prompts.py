#!/usr/bin/env python3
# Gera prompts simples a partir do corpus C++ (extrai trechos de código e linhas de contexto)
import re, json, sys
inp = "data/cpp_corpus.txt"
out = "data/distill_prompts.jsonl"
max_snippet_lines = 40

def is_code_line(l):
    # heurística simples: presença de ';', '{', '}', '::', '#include'
    return (';' in l) or ('{' in l) or ('}' in l) or ('::' in l) or ('#include' in l) or ('template' in l)

with open(inp, "r", errors="ignore") as f:
    text = f.read()

# split by blank-line blocks
blocks = re.split(r'\n\s*\n', text)
prompts = []
for b in blocks:
    lines = [ln for ln in b.splitlines() if ln.strip()]
    if not lines: continue
    # find code-like stretch
    code_lines = [ln for ln in lines if is_code_line(ln)]
    if not code_lines: 
        # occasionally still use small comment blocks as context
        if len(lines) >= 3 and any(len(ln)>40 for ln in lines[:3]):
            prompt_text = "\n".join(lines[:max_snippet_lines])
            prompts.append({"prompt": f"Explain the following C++ text or give an example usage:\\n\n{prompt_text}"})
        continue
    # create prompt with surrounding context: up to 20 lines around first code occurrence
    idx = next(i for i,ln in enumerate(lines) if is_code_line(ln))
    start = max(0, idx-10)
    snippet = lines[start:start+max_snippet_lines]
    prompt_text = "\n".join(snippet)
    prompts.append({"prompt": f"Explain the following C++ code and show a short example or fix if needed:\\n\n{prompt_text}"})

# deduplicate and save
seen = set()
with open(out,"w") as fo:
    for p in prompts:
        key = p["prompt"][:200]
        if key in seen: continue
        seen.add(key)
        fo.write(json.dumps(p, ensure_ascii=False) + "\n")

print(f"Generated {len(seen)} prompts -> {out}")
