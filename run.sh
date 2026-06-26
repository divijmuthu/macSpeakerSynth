#!/usr/bin/env bash
#
# run.sh — one-command launcher for the RACE synth.
#
# Starts BOTH halves of the project in their own macOS Terminal windows:
#   1. The C++ audio engine  (./build/race_synth)
#   2. The Python control UI (py_interface/main.py)
#
# Usage:
#   ./run.sh                 # build if needed, then launch both in new terminals
#   ./run.sh --backend coreaudio   # force Core Audio backend (macOS)
#   ./run.sh --backend miniaudio   # force miniaudio backend
#   ./run.sh --rebuild       # force a clean reconfigure + build first
#   ./run.sh --inline        # run engine in background + UI in foreground (no new windows)
#
set -euo pipefail

# --- locate ourselves -------------------------------------------------------
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
ENGINE_BIN="${BUILD_DIR}/race_synth"
VENV_DIR="${ROOT_DIR}/.venv"

# --- parse args -------------------------------------------------------------
BACKEND=""          # empty => engine default
REBUILD=0
INLINE=0
while [[ $# -gt 0 ]]; do
  case "$1" in
    --backend) BACKEND="${2:-}"; shift 2 ;;
    --rebuild) REBUILD=1; shift ;;
    --inline)  INLINE=1; shift ;;
    -h|--help)
      grep '^#' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
    *) echo "Unknown option: $1" >&2; exit 1 ;;
  esac
done

# --- 1. build the engine if needed -----------------------------------------
if [[ "$REBUILD" -eq 1 || ! -x "$ENGINE_BIN" ]]; then
  echo "[run.sh] Configuring + building C++ engine..."
  cmake -S "$ROOT_DIR" -B "$BUILD_DIR" >/dev/null
  cmake --build "$BUILD_DIR" --target race_synth
fi

# --- 2. make sure Python deps are present ----------------------------------
if [[ ! -d "$VENV_DIR" ]]; then
  echo "[run.sh] Creating Python venv + installing deps..."
  python3 -m venv "$VENV_DIR"
  "$VENV_DIR/bin/pip" install --quiet --upgrade pip
  "$VENV_DIR/bin/pip" install --quiet -r "$ROOT_DIR/py_interface/requirements.txt"
fi

# --- 3. assemble the two commands ------------------------------------------
ENGINE_ENV=""
if [[ -n "$BACKEND" ]]; then
  ENGINE_ENV="RACE_AUDIO_BACKEND=${BACKEND} "
fi
ENGINE_CMD="cd '${ROOT_DIR}' && ${ENGINE_ENV}'${ENGINE_BIN}'"
UI_CMD="cd '${ROOT_DIR}' && '${VENV_DIR}/bin/python' py_interface/main.py"

# --- 4a. inline mode (no new windows) --------------------------------------
if [[ "$INLINE" -eq 1 ]]; then
  echo "[run.sh] Launching engine in background, UI in foreground..."
  ( eval "$ENGINE_CMD" ) &
  ENGINE_PID=$!
  trap 'kill "$ENGINE_PID" 2>/dev/null || true' EXIT
  sleep 0.5
  eval "$UI_CMD"
  exit 0
fi

# --- 4b. open two Terminal.app windows (default on macOS) ------------------
open_terminal() {
  local title="$1" cmd="$2"
  osascript >/dev/null <<EOF
tell application "Terminal"
  activate
  do script "echo -n -e '\\033]0;${title}\\007'; ${cmd}"
end tell
EOF
}

if [[ "$(uname)" == "Darwin" ]]; then
  echo "[run.sh] Opening engine + UI in separate Terminal windows..."
  open_terminal "RACE engine" "$ENGINE_CMD"
  sleep 0.5   # give the engine a moment to bind its ZMQ sockets
  open_terminal "RACE UI" "$UI_CMD"
  echo "[run.sh] Done. Close either window (Ctrl-C) to stop that half."
else
  echo "[run.sh] Non-macOS detected; falling back to --inline mode."
  exec "$0" --inline ${BACKEND:+--backend "$BACKEND"}
fi
