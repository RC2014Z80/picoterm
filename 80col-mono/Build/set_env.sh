#!/bin/bash
echo "Set PicoTerm development environment..."
echo "  call it with \"source set_env.sh\" for permanent alias."
alias cmake_clean='rm -rf CMakeFiles/ CMakeCache.dat'

export pico_sdk_path=/home/$USER/pico/pico-sdk
echo "Clear cmake cache"
rm -rf CMakeFiles/ CMakeCache.dat
