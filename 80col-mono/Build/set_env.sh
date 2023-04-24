#!/bin/bash
echo "Set PicoTerm development environment..."
echo "  call it with \"source set_env.sh\" for permanent alias."

export pico_sdk_path=/home/$USER/pico/pico-sdk
export src_path=/home/$USER/Bureau/RC2014/picoterm


alias cmake_clean='rm -rf CMakeFiles/ CMakeCache.dat'
# find a keyword in 80 column -or- 40 columns source code (case sensitive)
alias f80='grep -rn $src_path/common/*.c $src_path/common/*.h $src_path/80col-mono/*.c $src_path/80col-mono/*.h -e'
alias f40='grep -rn $src_path/common/*.c $src_path/common/*.h $src_path/40col-color/*.c $src_path/40col-color/*.h -e'
# compile 80column version
alias c80='cd $src_path/80col-mono/Build;make picoterm_BE'
# send 80column version --> to MCU
alias cp80='cp $src_path/80col-mono/Build/picoterm_BE.uf2 /media/$USER/RPI-RP2/'


echo "Alias: cmake_clean, f80 <keyword>, f40 <keyword>, c80, cp80"
echo "Clear cmake cache"
rm -rf CMakeFiles/ CMakeCache.dat
