#!/bin/bash
echo "Set PicoTerm development environment..."
echo "  call it with \"source set_env.sh\" for permanent alias."
alias cmake_clean='rm -rf CMakeFiles/ CMakeCache.dat'
export PICOTERM_HREV=20
export pico_sdk_path=/home/$USER/pico/pico-sdk
echo "PICOTERM_HREV = $PICOTERM_HREV"
echo "Clear cmake cache"
rm -rf CMakeFiles/ CMakeCache.dat
