#!/usr/bin/env sh

set -e
# set -x

reset
rake

./bin/slink $(find ./out/test/ -name "*.o")

# cd out/test/
# gcc -nostdlib *.o -o main
# ./main
