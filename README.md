# PicoTerm
Pi Pico VGA Terminal Emulator For RC2014 supporting several keyboard layout.

![PicoTerm to RC2014](docs/_static/picoterm-to-rc2014.jpg)

Once wired to the UART of the RC2014 (or any retro-computer) your get an autonomous system.

![PicoTerm](docs/_static/picoterm.jpg)

__PicoTerm is a terminal emulator__ written specifically for this module. Currently it runs 80 columns by 30 lines in black and white mode. Switching to 40 column colour version will be available shortly. It can use VT100 style escape codes, with support for the following

| Escape sequence             | Description                                              | [Test name](test-suite/readme.md)  |
|-----------------------------|----------------------------------------------------------|--------------------|
| \ESC[?7l	| Wraparound OFF                                                             | no_warp            |
| \ESC[?7h	| Wraparound ON                                                              | do_warp            |
| \ESC[?12l	| Text Cursor Disable Blinking (but still visible)                           | cursor_blink       |
| \ESC[?12h	| Text Cursor Enable Blinking                                                | cursor_blink       |
| \ESC[?25l | Cursor invisible                                                           | cursor_hide        |
| \ESC[?25h | Cursor visible                                                             | cursor_hide        |
| \ESC[H    | Move to 0-0                                                                | clearscr           |
| \ESC[s    | Save the cursor position                                                   | cursor_save        |
| \ESC[u    | Move cursor to previously saved position                                   | cursor_save        |
| \ESC[*{row}*;*{col}*H | Move to *{row}*,*{col}*                                        | move_at_2_3        |
| \ESC[*{row}*;*{col}*f | Move to *{row}*,*{col}*  (same as H)                           | move_at_2_3_v2     |
| \ESC[0K   | Clear from cursor to the end of the line                                   |                    |
| \ESC[1K   | Clear from the beginning of the current line to the cursor                 |                    |
| \ESC[2K   | Clear the whole line                                                       |                    |
| \ESC[0J	  | Clear the screen from cursor until end of screen                           |                    |
| \ESC[2J   | Clear the screen and move the cursor to 0-0                                | clear              |
| \ESC[*{n}*@	| Insert *{n}* Space Characters                                            | char_insert        |
| \ESC[*{n}*P	| Delete *{n}* Characters, shifting in space characters                    |                    |
| \ESC[*{n}*X	| Erase *{n}* Characters, overwriting them with a space character.         |                    |
| \ESC[*{n}*A | Move the cursor up *{n}* lines                                           |                    |
| \ESC[*{n}*B | Move the cursor down *{n}* lines                                         |                    |
| \ESC[*{n}*C | Move the cursor forward *{n}* characters                                 |                    |
| \ESC[*{n}*D | Move the cursor backward *{n}* characters                                |                    |
| \ESC[*{n}*d	| Move the cursor to an absolute *{n}* line                                |                    |
| \ESC[*{n}*E	| Move the cursor to beginning of next line, *{n}* lines down              |                    |
| \ESC[*{n}*F	| Move the cursor to beginning of previous line, *{n}* lines up            |                    |
| \ESC[*{n}*G	| Move the cursor to column *{n}*                                          |                    |
| \ESC[0m     | normal text (should also set foreground & background colours to normal)  |                    |
| \ESC[5m	  | blink ON                                                                   | blink              |
| \ESC[7m   | reverse text                                                               |                    |
| \ESC[25m	| blink OFF                                                                  | blink              |
| \ESC[27m	| reset inverse/reverse mode                                                 |                    |
| \ESC[0J   | clear screen from cursor                                                   | clearscr           |
| \ESC[1J   | clear screen to cursor                                                     |                    |
| \ESC[3J   | same as \ESC[2J                                                            |                    |
| \ESC[nS   | scroll whole page up by n rows (default 1 if n missing)                    |                    |
| \ESC[*{n}*T	| scroll up *{n}* lines                                                    |                    |
| \ESCF     | Enter graphic mode (special graphic charset, NuPetScii). [Sample](docs/using-nupetscii.md).        |  nupetscii    |
| \ESCG     | Exit graphic mode (ASCII charset)                                          |  ascii             |

Cursor Style

| Escape sequence             | Description                                              | Test name          |
|-----------------------------|----------------------------------------------------------|--------------------|
| \ESC[0 q	| Default cursor shape configured by the user                                |                    |
| \ESC[1 q	| Blinking block cursor shape                                                |                    |
| \ESC[2 q	| Steady block cursor shape                                                  |                    |
| \ESC[3 q	| Blinking underline cursor shape                                            |                    |
| \ESC[4 q	| Steady underline cursor shape                                              |                    |
| \ESC[5 q	| Blinking bar cursor shape                                                  |                    |
| \ESC[6 q	| Steady bar cursor shape                                                    |                    |

DEC Line Drawing

| Escape sequence             | Description                                              | Test name          |
|-----------------------------|----------------------------------------------------------|--------------------|
| \ESC(0   | Enables DEC Line Drawing Mode - single line                                 |                    |
| \ESC(2   | Enables DEC Line Drawing Mode - double line                                 |                    |
| \ESC(B   | Enables ASCII Mode (Default)                                                |                    |

| Hex     | ASCII    | DEC Line Drawing      |
|---------|----------|-----------------------|
| 0x6a    | j        | ┘                     |
| 0x6b    | k        | ┐                     |
| 0x6c    | l        | ┌                     |
| 0x6d    | m        | └                     |
| 0x6e    | n        | ┼                     |
| 0x71    | q        | ─                     |
| 0x74    | t        | ├                     |
| 0x75    | u        | ┤                     |
| 0x76    | v        | ┴                     |
| 0x77    | w        | ┬                     |
| 0x78    | x        | │                     |

40 col colour only: (sequence is ignored, no effect in 80 col b/w)

| Escape sequence             | Description                                              |
|-----------------------------|----------------------------------------------------------|
| \ESC[38;5;*{n}*m | Set foreground colour to *{n}* (0-255)                              |
| \ESC[48;5;*{n}*m | Set background colour to *{n}* (0-255)                              |

USB keyboards are supported via a USB OTG adapter – however, not all keyboards currently work. Most cheap generic keyboards seem to work fine, however, the testing sample is still fairly small. Hopefully with more data it will be easier to identify exactly which keyboards are likely to work and which aren’t, or, better still, a simple software fix will get more working.

PicoTerm provides:
* VT100 ASCII: default, the 8th bit is for reverse video character.
* [advanced NuPetSCII charset](nupetscii-font/readme.md): that charset defines entry from 128 to 255 to display semi-graphical characters (like Commodore C64 or CodePage 437).

![Characters added to the font8.c](nupetscii-font/nupet-ascii-reduced.png)

Big thanks to Tom Wilson and its [Character-Editor](https://github.com/tomxp411/Character-Editor) for autorising the NuPet ASCII charset inclusion.

![NuPetScii Demo example](docs/_static/NupetSciiDemo-result.jpg)

If you are interested in Drawing & Rendering NuPetScii ressource in PicoTerm you can read:
* [NupetScii-Font readme](nupetscii-font/readme.md): explains how to create ressource and extract data
* [Using NupetScii readme](docs/using-nupetscii.md): some RC2014 assembly & codes related to NuPetScii usage on RC2014.

## How PicoTerm works
* Textmode version (from v1.1) allows choice of green, amber or white on black, by holding button A, B or C on power-up. (choice is remembered).
* Configuration menu is available via CTRL+SHIFT+M (configuration can be stored in Flash)
* VGA generation starts at power-up
* Pico LED blinks --> no USB device/keyboard attached
* Pico LED off --> USB device/keyboard connected
* VGA display is suspended 1 second when plug-in an USB keyboard

## Uploading firmware

The Pi Pico uses a UF2 bootloader to appear as a mass storage device so that new firmware can be uploaded to it.  To do this, connect a Micro USB lead between the Pico and your PC/Mac/Laptop/Raspberry Pi/Android Phone.  Then push the BOOTSEL button on the Pico. Whilst holding this down, push and release the RUN button on the VGA board.  (Trust me, This is easier to to than to put in to words!). The Pico will then show up as a drive on your computer.  Simply drag and drop the UF2 firmware on to this drive.  The Pico will automatically reboot and disconnect once this is complete.

# Release notes
See the file [releases.md](releases.md) .

## Know issues
1. USB keyboard is not detected if already connected at power-up. Disconnect and reconnect it! __Hardware workaround available see the [picoterm-port](docs/picoterm-port.md)__.
2. VGA rendering sometime hangs when connecting a keyboard (rare). Press reset button (on PicoTerm) and try again.
3. Saving the configuration into Flash fails from time to time (rare). Just press reset button (on PicoTerm) and try again.

# PicoTerm documentation

The picoterm projet contains a wide variety of documentation and ressources about the software.

| Document                    | Description                                              |
|-----------------------------|----------------------------------------------------------|
| [Release notes](releases.md)       | History of changes                                |
| [NupetScii](nupetscii-font/readme.md) | Discovering the NuppetScii alternative font and coding. Discover Playscii a software to draw screen with NuPetScii. |
| [Using-NupetScii](docs/_static/using-nupetscii.md) | How to activate NuPetScii from RC2014. |
| [Compiling](compiling.md)          | Building firmware from source<br />How to setup the compilation environment to compile PicoTerm on your computer |
| [Debug](docs/debug.md)             | Poor man serial debugger for PicoTerm.<br />Need to debug and troubleshoot? This document describes the picoterm _debug uart_. |
| [Add keyboard layout](docs/add-keyboard-layout) | How to add a new keyboard layout to PicoTerm.                    |
| [test-suite](test-suite/readme.md) | How to test ESC sequence support of Picoterm and how to expand the tests.     |
| [picoterm-conn](docs/picoterm-conn.md)  | Details about the PicoTerm expansion connector and available GPIOs.      |
| [Resources](docs/resources.md)     | Useful ressource link used during development of PicoTerm                     |
