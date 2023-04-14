https://github.com/raspberrypi/pico-examples/blob/master/pio/spi/spi.pio
https://github.com/elehobica/pico_fatfs_test/tree/main/fatfs

The second lines shows how how to wire the SD card reader.
The UART0 is used for output @ 115200 bauds.

It produces:

=====================
== pico_fatfs_test ==
=====================
mount ok
Type is FAT16
Card size:    0.51 GB (GB = 1E9 bytes)

FILE_SIZE_MB = 5
BUF_SIZE = 512 bytes
Starting write test, please wait.

write speed and latency
speed,max,min,avg
KB/Sec,usec,usec,usec
175.3597, 73377, 1955, 2917
168.4301, 79083, 1935, 3037

Starting read test, please wait.

read speed and latency
speed,max,min,avg
KB/Sec,usec,usec,usec
1064.8945, 1456, 411, 480
1064.8945, 1456, 411, 480

Done

# Documentation

http://elm-chan.org/fsw/ff/00index_e.html


https://github.com/Bodmer/SdFat/blob/master/examples/ReadWriteSdFat/ReadWriteSdFat.ino
