#!/bin/bash

cd bitcoinspoon
./compile.sh && result=true || result=false
cd ..

if ! [ $result ]; then
 exit
fi

echo "Compiling"
gcc -g -Ibitcoinspoon/code main.c bitcoinspoon/code/objects/all.a bitcoinspoon/libraries/objects/all.a -lsqlite3 -lgmp -lstdc++ -o gatekeeper
