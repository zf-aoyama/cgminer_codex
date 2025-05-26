# Repository Structure

.
└─ cgminer/
   ├─ reference/              (read-only; reference implementations only)
   │   ├─ ESP-Miner/          – software for controlling ASIC chips with an ESP32
   │   └─ bitaxeGamma/        – hardware source for a single-ASIC mining machine
   └─ ...                     (editable source code)

## Instructions

1. Treat `cgminer/reference/` and all of its subdirectories (`ESP-Miner/`, `bitaxeGamma/`) as **read-only**; use them only for guidance when refactoring or implementing features.

2. **Edit only the files inside `cgminer/`**, excluding everything under the `reference` directory tree.
