#!/bin/bash

set -e

TARGET="main"

gcc src/*.c -Iinclude -lm -o "$TARGET"
echo "compilação concluída com sucesso!"

./"$TARGET"
