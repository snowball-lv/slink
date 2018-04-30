#!/usr/bin/env sh

set -e
set -x

reset
rake

./bin/slink $(find ./out/test/ -name "*.o")
chmod +x hello_world
./hello_world
