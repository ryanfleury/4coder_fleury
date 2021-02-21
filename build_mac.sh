#!/bin/bash

set -e

../bin/buildsuper_x64-mac.sh 4coder_fleury.cpp

cp custom_4coder.so ../../custom_4coder.so
cp -r custom_4coder.so.dSYM ../../custom_4coder.so.dSYM
