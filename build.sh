#!/usr/bin/env sh

set -e
set -x

reset
rake
./bin/slink $(find ./out/ -name "*.o")
