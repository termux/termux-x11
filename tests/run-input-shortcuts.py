#!/usr/bin/env python3
"""Run keyboard regressions with real Android KeyEvents and the built app classes."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--sdk", type=Path, required=True)
parser.add_argument("--java-home", type=Path, required=True)
parser.add_argument("--build-tools", default="36.0.0")
parser.add_argument("--apk", type=Path, required=True)
parser.add_argument("--adb", default="adb")
args = parser.parse_args()
root = Path(__file__).resolve().parent.parent
classes = root / "lorie/build/intermediates/javac/debug/compileDebugJavaWithJavac/classes"
android = args.sdk.resolve() / "platforms/android-34/android.jar"
java = args.java_home.resolve()
env = dict(os.environ, JAVA_HOME=str(java))

def run(command, **kwargs):
    subprocess.run([str(x) for x in command], check=True, env=env, **kwargs)

remote_apk = "/data/local/tmp/termux-x11-shortcut-test.apk"
remote_dex = "/data/local/tmp/termux-x11-shortcut-test.dex"
with tempfile.TemporaryDirectory(prefix="x11-shortcut-test-") as directory:
    tmp = Path(directory)
    test_classes = tmp / "classes"
    test_classes.mkdir()
    run([java / "bin/javac", "-cp", f"{android}{os.pathsep}{classes}",
         "-d", test_classes, root / "tests/InputShortcutsTest.java"])
    app_classes = tmp / "app-classes.jar"
    run([java / "bin/jar", "cf", app_classes, "-C", classes, "."])
    dex = tmp / "dex"
    dex.mkdir()
    run([args.sdk.resolve() / "build-tools" / args.build_tools / "d8",
         "--min-api", "24", "--lib", android, "--classpath", app_classes,
         "--output", dex, *test_classes.glob("*.class")])
    try:
        run([args.adb, "push", args.apk, remote_apk])
        run([args.adb, "push", dex / "classes.dex", remote_dex])
        run([args.adb, "shell", f"CLASSPATH={remote_apk}:{remote_dex}",
             "app_process", "/system/bin", "InputShortcutsTest"])
    finally:
        subprocess.run([args.adb, "shell", "rm", "-f", remote_apk, remote_dex], check=False)
