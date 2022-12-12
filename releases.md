# Release Notes

```
 ____  _         _____                   
|  _ \(_) ___ __|_   _|__ _ __ _ __ ___  
| |_) | |/ __/ _ \| |/ _ \ '__| '_ ` _ \
|  __/| | (_| (_) | |  __/ |  | | | | | |
|_|   |_|\___\___/|_|\___|_|  |_| |_| |_|
 ____      _                             
|  _ \ ___| | ___  __ _ ___  ___  ___    
| |_) / _ \ |/ _ \/ _` / __|/ _ \/ __|   
|  _ <  __/ |  __/ (_| \__ \  __/\__ \   
|_| \_\___|_|\___|\__,_|___/\___||___/   
```

From version 1.2 any publication will includes all U2F firmware files for 40 & 80 Columns.

A thirds number in publication (eg: 1.1.1, 1.1.x) refers to an intermediate development version until it is finally published as a major version (say 1.2).

## Version 1.4.2 - (ongoing)

### Features
* Add __ESC F__ to enter graphic mode (special graphic charset, NuPetSCII). See [example](docs/using-nupetscii.md).
* Add __ESC G__ to exit graphic mode (ASCII charset)
* Add [nupetscii-for-playscii.zip](nupetscii-font/nupetscii-for-playscii.zip) for [Playscii](https://jp.itch.io/playscii) (a ASCII art drawing tool by JP Lebreton).
* Add terminal escape sequence [resources](docs/resources.md) documentation.
* Add documentation [using-nupetscii](docs/using-nupetscii.md), how to draw art/screen/ressource with [Playscii](https://jp.itch.io/playscii) and how to draw with some assembly & code.



## Version 1.4.1 - Dec 2 2022 - NupetScii fix for CP/M

In version 1.4, the NupetScii font does displays strange behavior on CP/M WordStar (& Turbo Pascal) because the 8th bit is used for reverse video display.

This V1.4.1, allow to switch between VT100 ASCII (8th bit for reverse, default) and [NuPetScii](nupetscii-font/readme.md) making it a breeze to work with CP/M.

Tested with WordStar and works as espected.

### Features
* VT100 ASCII is the default Font used (7bits+8th bit for reverse)
* Being able to switch between [NuPetSCII 8 bits](nupetscii-font/readme.md) vs VT100 mode (7 bits, 8th bit for reverse).
* Store the NuPetAscii vs VT100 option in Flash
* Keyboard SHIFT+CTRL+L quick shortcut to switch between Nupetscii and VT100 on the fly.


## Version 1.4 - Nov 27 2022

This realease now support extended characters set (> 127). The [extra charset is based on NuPET ASCII](nupetscii-font/readme.md) (NuPetSCII for short) from Tom Wilson.<br />
Big thanks to Tom Wilson and its [Character-Editor](https://github.com/tomxp411/Character-Editor) for autorising the NuPET ASCII charset inclusion.

__Remarks:__ NupetScii ruine the WordStar display under CP/M. This version is replaced with 1.4.1

### Features
* Adding support for [NuPetSCII charset](nupetscii-font/readme.md) (for char >127).
* CTRM+SHIFT+H : display Help screen
* CTRL+SHIFT+N : display the Charset on screen.
* Revamp CONFIGURATION menu with NupetScii

### Fix & Improvement
* rename display_menu() in display_config()
* Can change screen color without limits! Thanks to Spock64 for fixing issue #14 :-).
* Using `picoterm_config_t` typedef definition & `extern` declaration to fix compilation issue. __Awesome contribution of Spock64__ (Issue #13).

## Version 1.3 - Nov 12 2022

### Features
* Adding Purple color to 80 cols
* Many improvement in configuration screen for both 40 & 80 columns version
 * Can be modified on the fly without saving (press ESC).
 * Saved to Flash by pressing S (Uppercase "S") --> will Reboot.
 * Change of Baudrate (115200, 57600, 19200, 9600, 4800, 2400, 1200, 300)
 * Change of data bits (7 or 8)
 * Change of Stop bits (1 or 2)
 * Change of parity ( None, Even, Odd )

### Fix & Improvement
* rename "80Column Mono" -> "80col-mono" (removing space & camel-case)<br />
  __Remove older compilation files:__ Renaming the project folder will requires you to drop the existing compilation files (if any previous compilation task did occured in "80Column Mono" folder). Drop the folder `Build/CMakeFiles/` and files in `Build/` .
* rename "40Column Colour" -> "40col-coulour"
* Moving USB keyboard code to `common/keybd.c` (as dquadros does for RPITerm).
* Single __tusb_config.h__ definition: move `tusb_config.h` to /common (make tusb_config.h symbolic link from /40col and /80col to ../common/tusb_config.h)
* Remove `_TUSB_CONFIG_H_` definition from `main.h` (they are not compliled, `tusb_config.h` takes priority).

## Version 1.2 - Oct 24 2022

### Features
* Debug uart on GP28 (see [debug.md](debug.md) for details).
* Display/Hide menu with CTRL+SHIFT+M (ESC will also quit).
* Adding color selection in Menu (80 cols only)
* Additionnal Keyboard Layouts
 * Added layout for keyboard FR & keyboard BE. Now supporting UK, US, DE, FR, BE.
 * Characters [,],{,},|,@,,# to F1...F8 (because of early AltGr detection)
* Welcome screen shows:
 * the tinyUSB version (pico SDK usually use an older version)
 * the Keyboard layout used and its revision number

### Fix & Improvement
* Using GP28 as rx uart @ 115200 for `debug_print()`. See common/picoterm_debug.h.
  A global `debug_msg` buffer is available for sprintf() operation before debug_print() calls.
* Using `struct PicotermConfig` to hold software configuration parameter.
  Using `load_config` & `save_config` to load/save from flash.
	See common/picoterm_config.h
* Fix AltGR support for all keyboards (including BE & FR)
* pmhid.h : KEYMAP FR Rev 2 (fix issue on AltGR + 5).
* CMakeLists.txt : PICO_TINYUSB_PATH definition (to change TinyUSB under comment)
* Solve startup issue #7
* Readme (doc+cosmetic)
 * Some cosmetic updates (using table, add graphics).
 * Adding 'Adding a keyboard layout' section
 * Completing 'Building firmware from source" and add detailled 'compiling.md' file.
 * Adding 'Remarks' section with some tips.
* BIG IMPROVEMENT IN COMPILATION (CMakeLists.txt) [See details in Pull Request #8](https://github.com/RC2014Z80/picoterm/pull/8)
 * `make picoterm` for UK layout
 * `make picoterm_FR` for FR layout
 * `make all` to build all layouts in one single operation.
* Version number sourced from CMakeLists.txt [See details in Pull Request #9](https://github.com/RC2014Z80/picoterm/pull/9)


## Version 1.1 - Sept 21, 2022

Major code publication from RFC2795.

No details.

Publication of UF2 firmware for __80 cols only__.

## Version 1.0 - Mar 24, 2022

No details.

Publication of UF2 firmware for 40 cols/80 cols.

## Version 0.1.3 - Feb 14, 2022

Minor update.

No more details.

Publication of UF2 firmware for 40 cols/80 cols.

## Version 0.1.2 - Feb 14, 2022

Initial publication. UK & US keyboard layout

Publication of UF2 firmware for __80 cols only__.
