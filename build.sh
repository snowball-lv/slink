#!/usr/bin/env sh

set -e
set -x

reset
rake

PRE=""
PRE="${PRE} /usr/lib/gcc/x86_64-linux-gnu/5/../../../x86_64-linux-gnu/crt1.o"
PRE="${PRE} /usr/lib/gcc/x86_64-linux-gnu/5/../../../x86_64-linux-gnu/crti.o"
PRE="${PRE} /usr/lib/gcc/x86_64-linux-gnu/5/crtbeginT.o"

POST=""
POST="${POST} /usr/lib/gcc/x86_64-linux-gnu/5/../../../x86_64-linux-gnu/libc.a"
POST="${POST} /usr/lib/gcc/x86_64-linux-gnu/5/libgcc_eh.a"
# POST="${POST} /usr/lib/gcc/x86_64-linux-gnu/5/libgcc.a"
# # POST="${POST} /usr/lib/gcc/x86_64-linux-gnu/5/../../../x86_64-linux-gnu/libc.a"
POST="${POST} /usr/lib/gcc/x86_64-linux-gnu/5/crtend.o"
POST="${POST} /usr/lib/gcc/x86_64-linux-gnu/5/../../../x86_64-linux-gnu/crtn.o"

# ./bin/slink ${PRE} $(find ./out/test/ -name "*.o") ${POST}

cd out/test/
gcc -nostdlib *.o -o main

./main
