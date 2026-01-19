#!/usr/bin/env bash
set -euo pipefail
# build_and_run_all.sh
#  - backup arquivos críticos
#  - (re)configura CMake
#  - compila alvos importantes
#  - executa cada binário disponível
#  - converte PPM->PNG via magick quando disponível
#  - reúne outputs em ./outputs and logs in ./logs
#
# Uso: chmod +x build_and_run_all.sh && ./build_and_run_all.sh

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${ROOT}/build"
LOG_DIR="${ROOT}/logs"
OUT_DIR="${ROOT}/outputs"
BACKUP_DIR="${ROOT}/backups_before_full_run"

mkdir -p "${BUILD_DIR}" "${LOG_DIR}" "${OUT_DIR}" "${BACKUP_DIR}"

echo "[STEP] back up CMakeLists.txt and critical sources"
cp -n "${ROOT}/CMakeLists.txt" "${BACKUP_DIR}/CMakeLists.txt.$(date +%Y%m%d%H%M%S)" 2>/dev/null || true
# backup any render targets we created
for f in src/hvlm/render_* src/hvlm/render_engine* src/hvlm/render_prod* src/hvlm/render_pt_full*; do
  [ -e "$ROOT/$f" ] && mkdir -p "${BACKUP_DIR}/$(dirname "$f")" && cp -a "$ROOT/$f" "${BACKUP_DIR}/$(dirname "$f")/" 2>/dev/null || true
done

echo "[STEP] clean previous build (preserve cache if desired)"
# optionally remove build files: comment next line to keep CMake caches
# rm -rf "${BUILD_DIR}/*" || true

cd "${BUILD_DIR}"

echo "[STEP] cmake configure"
cmake .. -DCMAKE_BUILD_TYPE=Release 2>&1 | tee "${LOG_DIR}/cmake_configure.log"

# Build list of targets we want to try to build (safe if some don't exist)
TARGETS=(run_renderer run_pathtracer run_pathtracer_full run_engine run_prod)
# Build them one by one to limit linking conflicts and keep logs separate
for tgt in "${TARGETS[@]}"; do
  echo "[STEP] try build target: ${tgt}"
  if cmake --build . --target "${tgt}" -- -j"$(nproc)" 2>&1 | tee "${LOG_DIR}/build_${tgt}.log"; then
    echo "[OK] built ${tgt}"
  else
    echo "[WARN] build failed or target not found: ${tgt} (see ${LOG_DIR}/build_${tgt}.log)"
  fi
done

# helper to run a binary if present, capture output and move images
run_and_collect() {
  local binpath="$1"; shift
  local name
  name="$(basename "${binpath}")"
  if [ -x "${binpath}" ]; then
    echo "[STEP] running ${name} ..."
    "${binpath}" "$@" 2>&1 | tee "${LOG_DIR}/run_${name}.log"
    echo "[STEP] collect outputs for ${name}"
    # collect common file names created by your renderers
    for p in "${ROOT}"/*"${name}"*.* "${BUILD_DIR}"/*"${name}"*.* "${ROOT}/render"*"${name}"*.* "${BUILD_DIR}/render"*; do
      [ -e "$p" ] || continue
      cp -v "$p" "${OUT_DIR}/" 2>/dev/null || true
    done
    # also collect generic render_*.{ppm,png} from build root
    for p in "${BUILD_DIR}"/render_*.{ppm,png} "${ROOT}"/render_*.{ppm,png}; do
      [ -e "$p" ] || continue
      cp -v "$p" "${OUT_DIR}/" 2>/dev/null || true
    done
  else
    echo "[SKIP] ${name} not present or not executable"
  fi
}

# Run built executables (search both build and root)
for tgt in "${TARGETS[@]}"; do
  # possible paths
  BIN1="${BUILD_DIR}/${tgt}"
  BIN2="${ROOT}/build/${tgt}"    # redundancy
  BIN3="${ROOT}/${tgt}"
  if [ -x "${BIN1}" ]; then
    run_and_collect "${BIN1}"
  elif [ -x "${BIN2}" ]; then
    run_and_collect "${BIN2}"
  elif [ -x "${BIN3}" ]; then
    run_and_collect "${BIN3}"
  else
    echo "[INFO] no binary found for ${tgt}"
  fi
done

# convert any PPM in outputs to PNG (if magick present)
if command -v magick >/dev/null 2>&1; then
  echo "[STEP] converting PPM -> PNG using magick"
  shopt -s nullglob
  for ppm in "${OUT_DIR}"/*.ppm; do
    png="${ppm%.ppm}.png"
    echo " magick '${ppm}' '${png}'"
    magick "${ppm}" "${png}" 2>&1 | tee -a "${LOG_DIR}/magick_convert.log" || true
  done
else
  echo "[INFO] magick not found; skip PPM->PNG conversion. Install ImageMagick if desired."
fi

echo "[STEP] finalize: list outputs and logs"
ls -lh "${OUT_DIR}" || true
echo "Logs are in: ${LOG_DIR}"
echo "Backups are in: ${BACKUP_DIR}"

echo "[DONE] build_and_run_all finished"
