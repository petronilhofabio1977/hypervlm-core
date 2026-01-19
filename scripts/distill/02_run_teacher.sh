#!/usr/bin/env bash
set -e
VENV_ACT="venv/bin/activate"
[ -f "$VENV_ACT" ] && source "$VENV_ACT"

PROMPTS="data/distill_prompts_small.jsonl"
OUT="data/teacher_outputs.jsonl"
TEACHER_MODELS=("models/deepseek.gguf" "gptneo.gguf")

CANDIDATES=("./build/run_model" "./build/run.exe" "./third_party/llama.cpp/main" "./main" "./build/bin/llama" "./build/bin/main")
RUNNER=""
for c in "${CANDIDATES[@]}"; do
    if [ -x "$c" ]; then RUNNER="$c"; break; fi
done

if [ -z "$RUNNER" ]; then
    echo "Não encontrei runner GGUF automaticamente."
    exit 1
fi

echo "Usando runner: $RUNNER"

query_model() {
    local model="$1"
    local prompt_json="$2"
    local outjson="$3"

    mkdir -p "$(dirname "$outjson")"
    > "$outjson"

    while IFS= read -r line; do
        prompt=$(python3 - << 'PY' <<< "$line"
import sys, json
obj=json.loads(sys.stdin.read())
print(obj["prompt"])
PY
)
        echo "PROMPT -> $(echo "$prompt" | head -c120)..."

        resp=$("$RUNNER" -m "$model" --prompt "$prompt" --n 256 --temp 0.2 --top_k 40 2>/dev/null || true)

        python3 - << PY2 >> "$outjson"
import json
import sys
prompt = $(
    printf '%s' "$(printf "%s" "$prompt" | python3 -c 'import json,sys; print(json.dumps(sys.stdin.read()))')"
)
resp = $(
    printf '%s' "$(printf "%s" "$resp" | python3 -c 'import json,sys; print(json.dumps(sys.stdin.read()))')"
)
print(json.dumps({"prompt": prompt, "response": resp, "model": "$model"}, ensure_ascii=False))
PY2

    done < "$prompt_json"

    echo "Saved: $outjson"
}

> "$OUT"
for m in "${TEACHER_MODELS[@]}"; do
    if [ -f "$m" ]; then
        TMP="data/teacher_tmp.jsonl"
        query_model "$m" "$PROMPTS" "$TMP"
        cat "$TMP" >> "$OUT"
        rm -f "$TMP"
    else
        echo "Teacher model não encontrado: $m"
    fi
done

echo "Final teacher outputs -> $OUT"
