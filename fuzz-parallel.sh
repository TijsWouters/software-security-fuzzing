#!/usr/bin/env bash
# Usage: ./fuzz_tmux.sh <num_cores> <target_exe> [args...]
# Example: ./fuzz_tmux.sh 8 ./test

set -euo pipefail

if [[ $# -lt 2 ]]; then
  echo "Usage: $0 <num_cores> <target_exe> [args...]"
  exit 1
fi

CORES="$1"; shift
TARGET="$1"; shift || true
TARGET_ARGS=("$@")

# sanity checks
if ! [[ "$CORES" =~ ^[0-9]+$ ]] || (( CORES < 1 )); then
  echo "Error: <num_cores> must be a positive integer" >&2
  exit 1
fi
if [[ ! -x "$TARGET" ]]; then
  echo "Error: target '$TARGET' is not executable."
  exit 1
fi
command -v afl-fuzz >/dev/null || { echo "afl-fuzz not found"; exit 1; }

IN_DIR="in"
OUT_DIR="out"
mkdir -p "$IN_DIR" "$OUT_DIR"
[[ -z "$(ls -A "$IN_DIR" 2>/dev/null || true)" ]] && echo "seed" > "$IN_DIR/seed"

SESSION="afl_fuzz"
tmux has-session -t "$SESSION" 2>/dev/null && tmux kill-session -t "$SESSION"

echo "[+] Creating tmux session '$SESSION'"
tmux new-session -d -s "$SESSION"

AFL_BIN="$(command -v afl-fuzz)"
WD="$(pwd)"
TARGET_PATH="$(realpath "$TARGET")"

# master pane
tmux send-keys -t "$SESSION" \
  "cd '$WD' && AFL_NO_UI=1 '$AFL_BIN' -i in -o out -M fuzzer0 -- '$TARGET_PATH' ${TARGET_ARGS[@]} @@" C-m

# slave panes
for ((i=1; i<CORES; i++)); do
  tmux split-window -t "$SESSION":0 -v \
    "cd '$WD' && AFL_NO_UI=1 '$AFL_BIN' -i in -o out -S fuzzer$i -- '$TARGET_PATH' ${TARGET_ARGS[@]} @@"
  tmux select-layout -t "$SESSION":0 tiled >/dev/null
done

echo "[*] Started $CORES fuzzers in tmux session '$SESSION'"
echo "[*] Attach with:  tmux attach -t $SESSION"
echo "[*] Detach with:  Ctrl+B then D"
