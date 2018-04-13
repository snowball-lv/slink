#!/usr/bin/env sh

set -e
# set -x

reset
rake

./bin/slink $(find ./out/test/ -name "*.o")
# ./bin/slink stub.o stub-2.o

# cd out/test/
# gcc -nostdlib *.o -o main
# ./main

# nasm -f elf64 stub.asm -o stub.o
# nasm -f elf64 stub-2.asm -o stub-2.o
# gcc -nostdlib stub.o stub-2.o -o main_test
# ./main_test
