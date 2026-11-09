#!/bin/sh
# Level (levels/*.txt) neu nach source/levels.h übersetzen (Mac/Linux, braucht g++)
cd "$(dirname "$0")" || exit 1
g++ -O2 -std=c++17 tools/leveltool.cpp -o tools/leveltool && tools/leveltool levels --nds source/levels.h
