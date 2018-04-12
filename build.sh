#!/usr/bin/env sh

set -e
# set -x

reset
rake

# ./bin/slink $(find ./out/test/ -name "*.o")
./bin/slink stub.o stub-2.o

# cd out/test/
# gcc -nostdlib *.o -o main
# ./main
