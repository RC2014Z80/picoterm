#!/bin/bash
echo "Set PicoTerm development environment..."
echo "  call it with \"source set_env.sh\" for permanent alias."

export pico_sdk_path=/home/$USER/pico/pico-sdk
export src_path=/home/$USER/Bureau/RC2014/picoterm
export keyb=_BE

alias cmake_clean='rm -rf CMakeFiles/ CMakeCache.dat'
alias set_fr='export keyb=_FR'
alias set_be='export keyb=_BE'
# find a keyword in 80 column -or- 40 columns source code (case sensitive)
alias f80='grep -rn $src_path/common/*.c $src_path/common/*.h $src_path/80col-mono/*.c $src_path/80col-mono/*.h -e'
alias f40='grep -rn $src_path/common/*.c $src_path/common/*.h $src_path/40col-color/*.c $src_path/40col-color/*.h -e'
# compile 80column/40column version
alias c80='cd $src_path/80col-mono/Build;make picoterm$keyb'
alias c40='cd $src_path/40col-color/Build;make picoterm$keyb'
# send 80column/40cpm version --> to MCU
alias cp80='cp $src_path/80col-mono/Build/picoterm$keyb.uf2 /media/$USER/RPI-RP2/'
alias cp40='cp $src_path/40col-color/Build/picoterm$keyb.uf2 /media/$USER/RPI-RP2/'


echo "Alias: cmake_clean, f80 <keyword>, f40 <keyword>, c80, cp80, c40, cp40, set_fr, set_be"
echo "Clear cmake cache"
rm -rf CMakeFiles/ CMakeCache.dat
