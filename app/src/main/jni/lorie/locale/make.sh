#!/bin/bash
set -e -x
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
cd ${DIR}
rm -f test generator
gcc -o generator generator.c -lxkbcommon -g -rdynamic
./generator > keymaps.h
gcc -o test test.c -lX11 -g -rdynamic
./test
