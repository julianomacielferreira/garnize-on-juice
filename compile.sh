#!/bin/bash
# Production
# g++ -std=c++17 -Wall -Wextra -O2 -o garnize_on_juice src/main.cpp

# Debug
g++ -std=c++17 -Wall -Wextra -fdiagnostics-color=always -g -o garnize_on_juice src/main.cpp

# Running
./garnize_on_juice