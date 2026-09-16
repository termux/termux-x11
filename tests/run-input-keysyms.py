#!/usr/bin/env python3
"""Build the keycode-cache regression against an existing Android native build."""
import argparse
import json
from pathlib import Path
import shlex
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("compile_commands", type=Path)
parser.add_argument("--adb", help="Also run on the connected device using this adb executable")
args = parser.parse_args()

commands = json.loads(args.compile_commands.read_text())
entry = next(item for item in commands if item["file"].endswith("/InputXKB.c"))
source = Path(__file__).with_name("input-keysyms.c").resolve()
binary = args.compile_commands.resolve().parent / "input-keysyms-test"
original = entry.get("arguments") or shlex.split(entry["command"])
command = []
index = 0
while index < len(original):
    item = original[index]
    if item == "-o":
        index += 2
        continue
    if item not in ("-c", "-DNDEBUG"):
        command.append(str(source) if item == entry["file"] else item)
    index += 1

# Include the production implementation to exercise its private cache. Discard
# unrelated server functions so the test needs no running X server or stubs.
command += ["-ffunction-sections", "-Wl,--gc-sections", "-o", str(binary)]
subprocess.run(command, cwd=entry["directory"], check=True)
print(binary)
if args.adb:
    remote = "/data/local/tmp/termux-x11-input-keysyms-test"
    try:
        subprocess.run([args.adb, "push", str(binary), remote], check=True)
        subprocess.run([args.adb, "shell", "chmod", "700", remote], check=True)
        subprocess.run([args.adb, "shell", remote], check=True)
    finally:
        subprocess.run([args.adb, "shell", "rm", "-f", remote], check=False)
