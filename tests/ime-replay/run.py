#!/usr/bin/env python3
"""Replay real Android input-connection calls and verify a disposable editor draft."""
import argparse, json, pathlib, shlex, subprocess, sys, time
base = pathlib.Path(__file__).resolve().parent
parser = argparse.ArgumentParser(description='Replay IME fixtures into an empty Codex test draft on a shared-UID Termux:X11 build')
parser.add_argument('--adb', default='adb')
parser.add_argument('--cases', type=pathlib.Path, default=base.parent / 'ime-cases.json')
parser.add_argument('--repeat', type=int, default=2)
parser.add_argument('--container', default='codex-desktop')
parser.add_argument('--user', default='paddev')
parser.add_argument('--display', default=':1')
parser.add_argument('--window', default='ChatGPT')
parser.add_argument('--x', type=int, default=1600)
parser.add_argument('--y', type=int, default=2090)
args = parser.parse_args()
if args.repeat < 1:
    parser.error('--repeat must be positive')
adb = [args.adb]
native = ['run-as', 'com.termux', 'files/usr/bin/env', 'PATH=/data/data/com.termux/files/usr/bin:/system/bin', 'PREFIX=/data/data/com.termux/files/usr']
linux = native + ['files/usr/bin/proot-distro', 'login', '--isolated', '--shared-x11', '--user', args.user, '--env', 'DISPLAY=' + args.display, args.container, '--']

def shell(args):
    return subprocess.run(adb + ['shell', shlex.join(args)], capture_output=True, text=True, check=True, timeout=15).stdout

def desktop(cmd):
    cmd = cmd.replace('ChatGPT', shlex.quote(args.window)).replace('1600 2090', f'{args.x} {args.y}')
    return shell(linux + ['bash', '-c', cmd])

def gate(value):
    shell(['run-as', 'com.termux.x11', 'sh', '-c', 'printf %s ' + shlex.quote(value) + ' > cache/ime-probe-gate'])
fixtures = json.loads(args.cases.read_text())
if not fixtures or len({c['id'] for c in fixtures}) != len(fixtures):
    parser.error('fixtures must have unique IDs and must not be empty')
cases = {f"{c['id']}-{n}": dict(c, id=f"{c['id']}-{n}") for n in range(args.repeat) for c in fixtures}
shell(['run-as', 'com.termux.x11', 'mkdir', '-p', 'cache'])
subprocess.run(adb + ['shell', 'run-as com.termux.x11 sh -c ' + shlex.quote('cat > cache/ime-probe-cases.json')], input=json.dumps(list(cases.values()), ensure_ascii=False), text=True, check=True)
for attempt in range(10):
    try:
        warm = desktop('wmctrl -r ChatGPT -b add,fullscreen; wmctrl -a ChatGPT; xdotool keyup Control_L Control_R Shift_L Shift_R Alt_L Alt_R; xdotool mousemove 1600 2090 click 1; sleep 0.5; xdotool key --clearmodifiers --delay 150 ctrl+a BackSpace; xdotool type --clearmodifiers --delay 100 x11probe; xdotool key --clearmodifiers --delay 150 ctrl+a ctrl+c; sleep 0.5; timeout 3 xclip -selection clipboard -out')
        if warm == 'x11probe':
            break
    except (subprocess.CalledProcessError, subprocess.TimeoutExpired):
        pass
    time.sleep(2)
else:
    raise RuntimeError('Input field not ready')
shell(['run-as', 'com.termux.x11', 'rm', '-f', 'cache/ime-probe-status', 'cache/ime-probe-gate'])
shell(['am', 'broadcast', '-a', 'com.termux.x11.IME_PROBE', '-p', 'com.termux.x11'])
rows = []
previous = ''
deadline = time.monotonic() + 120
while time.monotonic() < deadline:
    try:
        status = shell(['run-as', 'com.termux.x11', 'cat', 'cache/ime-probe-status']).strip()
    except subprocess.CalledProcessError:
        time.sleep(0.2)
        continue
    if status == previous:
        time.sleep(0.2)
        continue
    previous = status
    deadline = time.monotonic() + 120
    if status == 'complete':
        break
    if status.startswith('error|'):
        raise RuntimeError(status)
    (phase, case) = status.split('|', 1)
    if phase == 'ready':
        desktop('wmctrl -a ChatGPT; xdotool mousemove 1600 2090 click 1; xdotool key --clearmodifiers --delay 120 ctrl+a BackSpace; sleep 0.3')
        gate(case + ':go')
    elif phase == 'done':
        marker = '__NO_SELECTION_' + case + '__'
        command = 'printf %s ' + shlex.quote(marker) + ' | xclip -selection clipboard -in; xdotool key --clearmodifiers --delay 120 ctrl+a ctrl+c; sleep 0.5; timeout 3 xclip -selection clipboard -out'
        actual = desktop(command)
        row = dict(id=case, expected=cases[case]['expected'], actual=actual, passed=actual == cases[case]['expected'])
        rows.append(row)
        print(json.dumps(row, ensure_ascii=False), flush=True)
        gate(case + ':next')
print(json.dumps(dict(summary=True, passed=sum((r['passed'] for r in rows)), total=len(rows), expected_cases=len(cases), status=previous)), flush=True)
if previous != 'complete' or len(rows) != len(cases) or (not all((r['passed'] for r in rows))):
    sys.exit(1)
