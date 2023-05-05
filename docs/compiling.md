# How to compile the Firmware

It is highly recommended that you follow the Getting Started With Raspberry Pi Pico documentation from the Raspberry Pi Foundation.  In addition to this, you will need to copy pico_sdk_import.cmake, pico_extras_import.cmake and font.h from pico-sdk, pico-extras and pico-playground\scanvideo\textmode.

Please follow this documentfor details about setting-up compile chain and build the picoterm for all the keyboard layouts. Issuing a `make all` will build all the layout at once.

**This document have been written while installing and compiling PicoTerm on a Linux machine (Linux Mint/Ubuntu)**.

## Install Pico-SDK

First step is to read the [Getting Started With Raspberry Pi Pico documentation](https://www.raspberrypi.com/documentation/microcontrollers/c_sdk.html) from the Raspberry-Pi foundation.

This document explains how to install the Pico C SDK, example and the tool-chain on the computer.

It is heavily recommanded to proceed the "blink" example compilation and to upload it on the board.

## Install pico-extra & pico-playground

```
cd ~/
cd pico
git clone https://github.com/raspberrypi/pico-extras
git clone https://github.com/raspberrypi/pico-playground
```

## Copy the PicoTerm sources

```
cd ~/
cd Bureau
mkdir RC2014
cd RC2014
REM make your fork of the https://github.com/RC2014Z80/picoterm then clone the copy
REM git clone https://github.com/mchobby/picoterm.git
REM or clone the original repos
git clone https://github.com/RC2014Z80/picoterm.git

REM We will build the "80col-mono" version
cd picoterm/80col-mono/Build

REM Indicates where to find the Pico SDK
REM   It is better to have absolute path instead of relative path
REM   export PICO_SDK_PATH=../../../../../pico/pico-sdk
export PICO_SDK_PATH=/home/__YOUR_USER__/pico/pico-sdk

REM Since jan 4, 2023 this operation is made by Cmake
REM
REM adding the required cmake file for this build
REM cp $PICO_SDK_PATH/external/pico_sdk_import.cmake ..
REM cp $PICO_SDK_PATH/../pico-extras/external/pico_extras_import.cmake ..
```

Your 80col-mono folder must contains the file `tusb_config.h` which is a link to `../common/tusb_config.h`.

It appears that link is not always restored on some computer. In such case, restablish it -OR- make a copy of `../common/tusb_config.h` in the `80col-mono`.

Now, we can call the cmake.

```
cmake ..
```

This will show the following results

```
$ cmake ..
PICO_SDK_PATH is /home/domeu/pico/pico-sdk
PICO platform is rp2040.
Defaulting PICO_EXTRAS_PATH as sibling of PICO_SDK_PATH: /home/domeu/pico/pico-sdk/../pico-extras
Build type is Release
PICO target board is pico.
Using board configuration from /home/domeu/pico/pico-sdk/src/boards/include/boards/pico.h
TinyUSB available at /home/domeu/pico/pico-sdk/lib/tinyusb/src/portable/raspberrypi/rp2040; enabling build support for USB.
cyw43-driver available at /home/domeu/pico/pico-sdk/lib/cyw43-driver
lwIP available at /home/domeu/pico/pico-sdk/lib/lwip
CMake Warning at /home/domeu/pico/pico-extras/src/rp2_common/lwip/CMakeLists.txt:5 (message):
  lwIP submodule has not been initialized; TCP/IP support will be unavailable

           hint: try 'git submodule update --init'.


-- Configuring done
-- Generating done
-- Build files have been written to: /home/domeu/Bureau/RC2014/picoterm/80col-mono/Build
```

# Need to upgrade TinyUSB library ?

The `tinyusb` library is included within PICO_SDK. If you need a newer version then it must be downloaded from tinyusb.org and installed within the source code.

```
cd ~/
cd Bureau
cd RC2014
cd picoterm/80col-mono/
mkdir tinyusb
```

Follow installation instruction from
https://github.com/hathach/tinyusb/blob/master/docs/reference/getting_started.rst

I have created the folder ~/Bureau/RC2014/picoterm/80col-mono/tinyusb
In the tinyusb folder copy the following subfolder from the archive : src, hw

In the "80col-mono" folder edit the file CMakeLists.txt and add the following "set" command

```
set(PICO_TINYUSB_PATH "/home/domeu/Bureau/RC2014/picoterm/80col-mono/tinyusb" )
include(pico_sdk_import.cmake)       
include(pico_extras_import.cmake)
```

# Building the firmware

Now, it is time to make the build

```
cd ~/
cd Bureau
cd RC2014
cd picoterm/80col-mono/Build

REM Since jan 4, 2023 this operation is made by cmake
REM
REM cp $PICO_SDK_PATH/../pico-playground/scanvideo/textmode/font.h ../

REM create with UK Mapping (default)
REM
make picoterm

REM create with FR Mapping
REM
make picoterm_FR

REM create all the mapping
REM
make all
```
Notice that make messages will includes the following statement when using another TinyUSB version:

```
TinyUSB available at /home/domeu/Bureau/RC2014/picoterm/80col-mono/tinyusb/src/portable/raspberrypi/rp2040; enabling build support for USB.
```

Voila! the `picoterm.uf2` is available in the current directory.
