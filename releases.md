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

## Version 1.6.0.32 - (ongoing)

__New GPIO attribution for expansion connector__ for applying to "Pico Vga Terminal" and "RP2040 VGA Terminal".

### Features
* Documentation review (see [GPIO usage on expansion connector](docs/picoterm-conn.md)).
* add `picoterm_harddef.h` to store hardware definition
	* SD Card initial support with pio_fatfs (version of FatFS over PIO SPI). Using GPIO 26,27,28,5. Voir main.c, spi_sd_init() et spi_sd_init().
	* I2C bus moved to GPIO 18 & 19
	* PoorMan Debugger moved to GPIO 22 @ 115200 bds
* __initial support of [fatfs](http://elm-chan.org/fsw/ff/00index_e.html) over PIO SPI__.<br />Read more about this from [pio_fatfs/readme.txt](pio_fatfs/readme.txt) file and [pure SPI pico_fatfs_test](https://github.com/elehobica/pico_fatfs_test) repository.
* __initial support of CLI__: Command Line Interpreter activated with SHIFT+CTRL+C with [several commands](docs/cli.md).<br />The CLI is used to test SDCard interaction and future configurable features.<br />The CLI is extended via the `cli/user_funcs.c` file.
	* sd_info
	* dir
	* type
	* send_file
* Adding [keyboard hotkey](docs/hotkey.md) (1.6.0.31)

### Fix & Improvement
* Sending escape sequences for keyboard strokes: SCANCODE_CURSOR_LEFT, SCANCODE_CURSOR_RIGHT, SCANCODE_CURSOR_UP, SCANCODE_CURSOR_DOWN, SCANCODE_PAGE_DOWN, SCANCODE_PAGE_UP, SCANCODE_HOME, SCANCODE_END, SCANCODE_DEL, SCANCODE_INS (see pmhid.h)
* keydb should pump message only for keyboard (not the mouse). See Issue #43 (Thanks Abaffa for suggestion)
* German keyboard adding apostrophe on scancode 0x32 . See Issue #44 (Thanks Skaringa, >1.6.0.31, Keymap Rev 2)
* Set background color (instead of foreground color) on [40m -> [47m and [100m -> [107m . See Issue #45 (Thanks Juzzas, >1.6.0.31)
* Use CMAKE_CURRENT_SOURCE_DIR ro generate pio header file (see CMakeList.txt, Thanks Juzzas, >1.6.0.31)
* set_env.sh to quicly setup the environment. Call it with `source set_env.sh` .
* rename conio slip_character() to put_char() - more conform with putc()
* set PICO_XOSC_STARTUP_DELAY_MULTIPLIER=64 when compiling as suggested by Spencer Owner.<br />On some boards/samples, the xosc can take longer to stabilize than is usual ([see this adafruit_itsybitsy_rp2040.h](https://github.com/raspberrypi/pico-sdk/blob/master/src/boards/include/boards/adafruit_itsybitsy_rp2040.h))



## Version 1.5.3 - Apr 23 2023
Notice: 40col-color upgraded _at the best_ to 80col features. Read specific remarks.md in the target build folder.

### Features
* Adding License file (BSD 3-Clause).
* Key Repeat implemented into keybd.c (see keydown_start_repeat_delay, keydown_resent_delay for parametrisation).
* save screen+cursor when activating menu screen. Restore them when existing menu screen.
* Buzzer & USB-Power pins: auto-detect PCA9536 I2C GPIO expander at startup (otherwise use GPIOs). See [picoterm-conn](docs/picoterm-conn.md) for details.
* pictoterm_screen now diplays at best under NupetScii and cp437 fonts.
* Multi-FontFace support : adding OlivettiThin font added
 * Support up to 16 Font-Face
 * Support up to 16 Charset
 * Know issue: CP437 & NupetScii data should also be designed in 14 pixels height for compiling OlivettiThin (also 14 pixels Height)

### Fix & Improvement
* font cp437 fix char 0xC4 (horizontal line)
* DEC Lines in ASCII: using = instead of - offers a better visibility.
* CMakeList copy the required files from PICO_SDK and font-suite (issue #25).
* Decoupling picoterm.c
 * extract console features to picoterm_conio.c
 * extract displayed screeb to picoterm_screen.c
 * passed full test-suite
 * renamed picoterm_core.c (main processing file)
* picoterm_conio.c
 * conio_config -> group all the console parameters that before was individual parameters
 * conio_config.cursor -> contains pos.x, pos.y, state (for visibility,blink state,...), symbol (for cursor symbol)
 * cursor save & restore
* Decoupling picoterm_screen.c
 * Store the PICOTERM_LOGO into picoterm_logo.c
 * allow replacement for custom project (Please keeps the PicoTerm credit)
 * passed full test-suite
* CMakeList.txt: Using $ENV{PICO_SDK_PATH} to detect environment variable and initialise ${PICO_SDK_PATH}
* Document how to add new charset.
* Document how to add new font-face.
* Avoids multiples allocation in `build_font()` causing memory fragmentation.
* font-suite/nupetscii.c & cp437.c are renamed mono8_cp437.c & mono8_nupetscii.c for better consistancy. Picoterm CMakeList and code are updated accordingly!

## Version 1.5.2 - Dec 30 2022

This is a temporary release for testing the new features.

### Features
* Display character rendering mode ASCII/ANSI on welcom screen.
* Display the selected ANSI graphical font 'to be used' on welcom screen.
* Include DEC line drawing (redirected to ANSI graphical font. Best with NupetScii, also supported in ASCII).
 * ESC(0 : single line Drawing
 * ESC(2 : double line drawing
 * ESC(B : return to Ascii print (exit line drawning, stays under ANSI font).
* Add new ANSI escape by [abaffa](https://github.com/abaffa) for 80col for VT100
 * ESCc : reset settings
 * ESC[?47l	: restore screen
 * ESC[?47h	: save screen
 * ESC[c : ask VT100ID
 * ESC[0c :	ask VT100ID
 * ESC[5n :	ask VT100 Status
 * ESC[?2l : enter VT52 mode
* VT52 escapes not available for VT100  by [abaffa](https://github.com/abaffa) for 80col
 * ESCA : Cursor up
 * ESCB : Cursor down
 * ESCC : Cursor right
 * ESCD : Cursor left
 * ESCH : Cursor to home
 * ESCI : Reverse line feed
 * ESCJ : Erase to end of screen
 * ESCK : Erase to end of line
 * ESCZ : Identify
 * ESC[Z : Identify
 * ESC< : enter VT100 mode
* move ESCF & ESCG back to global scope (VT100 & VT52) to activate ANSI foont (NupetScii,CP437,..)
 * ESCF : Special graphics character set (can be overseen by DEC Drawing lines)
 * ESCG : Select ASCII character set (can be overseen by DEC Drawing lines)
* Config : select the graphical font (NupetSCII / CP437) & store it into flash

### Fix & Improvement
* Lighten documentation in readme file (moved it into /docs).
* Rewrite the firmware upgrading.
* support ESC( with one parameter togheter with ESC[
* Additional tests writing.
* Multiple graphical ANSI font support.
* Cursor style supported under ASCII & ANSI graphical font.
* picoterm_cursor.h : move cursor definition
* picoterm_stddef.h : create standard definition (like point, font_id, ...)

## Version 1.5.1

### Features
* reorganize documentation
* Append /test-suite for testing escape sequences on PicoTerm
* activate GPIO 26 five seconds after reset/power-up. This is used to activate external USB power (for keyboard).
* Blinking interval at 525ms (like vt100 terminal)
* Add new ANSI escape by [abaffa](https://github.com/abaffa) for 80col
 * ESC[?7h : Wraparound ON
 * ESC[?7l : Wraparound OFF
 * ESC[?12h : Text Cursor Enable Blinking
 * ESC[?12l : Text Cursor Disable Blinking
 * ESC[5m : Blink ON
 * ESC[25m : Blink OFF
 * ESC[{n}@ : Insert Character Insert {n} spaces at the current cursor position, shifting all existing text to the right. Text exiting the screen to the right is removed.
 * ESC[P : Delete Character Delete characters at the current cursor position, shifting in space characters from the right edge of the screen.
 * ESC[X : Erase Character Erase characters from the current cursor position by overwriting them with a space character.
* Cursor Types by [abaffa](https://github.com/abaffa) for 80col< br/>SP is space.
 * ESC[0SPq User Shape Default cursor shape configured by the user
 * ESC[1SPq Blinking Block Blinking block cursor shape
 * ESC[2SPq Steady Block Steady block cursor shape
 * ESC[3SPq Blinking Underline Blinking underline cursor shape
 * ESC[4SPq Steady Underline Steady underline cursor shape
 * ESC[5SPq Blinking Bar Blinking bar cursor shape
 * ESC[6SPq Steady Bar Steady bar cursor shape
* DEC Line Drawing by [abaffa](https://github.com/abaffa) for 80col
 * ESC(0 Designate Character Set – DEC Line Drawing Enables DEC Line Drawing Mode - single line
 * ESC(2 Designate Character Set – DEC Line Drawing Enables DEC Line Drawing Mode - double line
 * see also the "Hex	ASCII	DEC Line Drawing" (in the ESC Seq documentation)
* ESC(B Designate Character Set – US ASCII Enables ASCII Mode (Default)

### Fix & Improvement

## Version 1.5 - Dec 18 2022

### Features
* Added 38400 bauds (used by RomWBW) by [abaffa](https://github.com/abaffa)
* Blinking cursor by [abaffa](https://github.com/abaffa)
* Add new ANSI escape by [abaffa](https://github.com/abaffa) for 80col
 * ESC[{line};{column}f : move cursor to given position
 * ESC[#d : moves cursor to an absolute # line
 * ESC[#G : moves cursor to column #
 * ESC[#E  : moves cursor to beginning of next line
 * ESC[#F : moves cursor to beginning of previous line
 * ESC[#T : scrolls up # lines
 * ESC[27m : reset inverse/reverse mode
* New escape to switch between ASCII and Semi graphical font for 80col
 * Add __ESC F__ to enter graphic mode (special graphic charset, NuPetSCII). See [example](docs/using-nupetscii.md).
 * Add __ESC G__ to exit graphic mode (ASCII charset)
* Add [nupetscii-for-playscii.zip](nupetscii-font/nupetscii-for-playscii.zip) for [Playscii](https://jp.itch.io/playscii) (a ASCII art drawing tool by JP Lebreton).
* Add documentation: terminal escape sequence [resources](docs/resources.md)
* Add documentation: [using-nupetscii](docs/using-nupetscii.md), how to draw art/screen/ressource with [Playscii](https://jp.itch.io/playscii) and how to draw with some assembly & code.


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
