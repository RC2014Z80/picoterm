#!/bin/bash
echo "Build all versions of Picoterm for all the major Hardware Releases"
echo "check the HRev10, HRev20, ... sub-folders"
unset PICOTERM_HREV

export PICOTERM_HREV=10
# Clear CMake cache files (needed to change value of #define PICOTERM_HREV)
rm -rf CMakeFiles/ CMakeCache.dat
# Clear previous build
rm HRev$PICOTERM_HREV/*.uf2
make clean
cmake ..
make all
mkdir HRev$PICOTERM_HREV
for i in picoterm*.uf2
do
	echo "$i --> HRev$PICOTERM_HREV/${i/.uf2/_HREV_$PICOTERM_HREV.uf2}"
  mv $i HRev$PICOTERM_HREV/${i/.uf2/_HREV_$PICOTERM_HREV.uf2}
done


export PICOTERM_HREV=20
# Clear CMake cache files (needed to change value of #define PICOTERM_HREV)
rm -rf CMakeFiles/ CMakeCache.dat
# Clear previous build
rm HRev$PICOTERM_HREV/*.uf2
make clean
cmake ..
make all
mkdir HRev$PICOTERM_HREV
for i in picoterm*.uf2
do
	echo "$i --> HRev$PICOTERM_HREV/${i/.uf2/_HREV_$PICOTERM_HREV.uf2}"
  mv $i HRev$PICOTERM_HREV/${i/.uf2/_HREV_$PICOTERM_HREV.uf2}
done
