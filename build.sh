#!/usr/bin/env sh

set -e
set -x

reset
rake

# ./bin/slink $(find ./out/test/ -name "*.o")
./bin/slink $(find ./out/test/ -maxdepth 1 -name "*.o") \
    bin/libslink-c.a bin/libslink-sys.a

chmod +x hello_world
./hello_world
