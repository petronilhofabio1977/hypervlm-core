#!/usr/bin/env bash
# Orquestrador: gera prompts -> roda teacher -> monta dataset -> treina
set -e
ROOT=$(pwd)
source venv/bin/activate
python3 scripts/distill/01_generate_prompts.py
bash scripts/distill/02_run_teacher.sh
python3 scripts/distill/03_build_student_dataset.py
bash scripts/distill/04_train_student.sh
echo "PIPELINE COMPLETO: Teacher->Student concluído"
