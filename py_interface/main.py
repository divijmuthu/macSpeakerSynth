"""RACE Python controller — graphical keyboard + filter/effect controls."""

import sys
from pathlib import Path

# Allow `python py_interface/main.py` from repo root or py_interface/.
sys.path.insert(0, str(Path(__file__).resolve().parent))

from race_ui import run_ui

if __name__ == "__main__":
    run_ui()
