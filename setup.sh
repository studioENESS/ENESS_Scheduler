#!/bin/bash

sudo apt-get install make g++ cmake libglfw3-dev nlohmann-json-dev ninja-build

mkdir build && cd build
# Features are toggled with -D<FEATURE>=ON/OFF, e.g.:
#   cmake ../ -GNinja -DFEATURE_SCRIPT_LIBRARY=ON -DFEATURE_CHOOSE_DAYS=OFF
cmake ../ -GNinja
ninja
