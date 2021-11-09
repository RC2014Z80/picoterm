# picoterm
Pi Pico VGA Terminal Emulator For RC2014

PicoTerm is a terminal emulator written specifically for this module. Currently it runs 80 columns by 30 lines in black and white mode. Switching to 40 column colour version will be available shortly. It can use VT100 style escape codes, with support for the following

- \ESC[?25l | Cursor invisible
- \ESC[?25h | Cursor visible
- \ESC[H | Move to 0-0
- \ESC[s | Save the cursor position
- \ESC[u | Move cursor to previously saved position
- \ESC[-Row-;-Col-H | Move to -Row-,-Col-
- \ESC[0K | Clear from cursor to the end of the line
- \ESC[1K | Clear from the beginning of the current line to the cursor
- \ESC[2K | Clear the whole line
- \ESC[2J | Clear the screen and move the cursor to 0-0
- \ESC[-n-A | Move the cursor up -n- lines
- \ESC[-n-B | Move the cursor down -n- lines
- \ESC[-n-C | Move the cursor forward -n- characters
- \ESC[-n-D | Move the cursor backward -n- characters
- \ESC[0m | normal text (should also set foreground & background colours to normal)
- \ESC[7m | reverse text
- \ESC[0J | clear screen from cursor
- \ESC[1J | clear screen to cursor
- \ESC[3J | same as \ESC[2J
- \ESC[nS | scroll whole page up by n rows (default 1 if n missing)

40 col colour only: (sequence is ignored, no effect in 80 col b/w)

- \ESC[38;5;-n-m | Set foreground colour to -n- (0-255)
- \ESC[48;5;-n-m | Set background colour to -n- (0-255)

USB keyboards are supported via a USB OTG adapter – however, not all keyboards currently work. Most cheap generic keyboards seem to work fine, however, the testing sample is still fairly small. Hopefully with more data it will be easier to identify exactly which keyboards are likely to work and which aren’t, or, better still, a simple software fix will get more working.
