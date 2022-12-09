# NuPetSCII - Expanding the font8.c with glyphs

The `font8.c` used in the original PicoTerm 80 column only defines the ASCII charset
(from 0 to 126. 127 is use for cursor display).

`font8.c` leaks of of nice graphical interface (like Commodore 64 or Code Page 437 do have).

Extending ASCII charset would be a great idea to draw ASCII tables and rudimentary interface.

It is even possible to draw screen and maps by using the Playscii software (see further).

## NuPETSCII - ASCII charset for C256 Foenix

Here are the additionnal chars merged into the `font8.c` file.

![NuPET ASCII charset](nupet-ascii.png)

```
font8.c + nupet-ascii.data ---( compile_font.py )---> nupetscii.c
```

## Credit
I discovered the NuPET font/charset designed by Tom Wilson on this [Stefany Allaire article](https://stefanyallaire.wixsite.com/website/forum/the-specifications/character-sets).

Tom did use its own [Character-Editor](https://github.com/tomxp411/Character-Editor) to design the NuPET ASCII charset (and many other). [Character-Editor](https://github.com/tomxp411/Character-Editor) is a ¢# opensource projet written available on GitHub.

Tom Wilson gracefuly authorise the usage of its NuPet font design in PicoTerm. Thanks to him for its sharing.

# Drawing/Art with NuPetScii

If you want to draw screen or art with NuPetScii, you can use the [Playscii from JP Lebreton](https://jp.itch.io/playscii), an open-source software written in Python3.

This software can be used on Windows, Mac and Linux. Linux version runs directly from source code.

![Playscii sotfware](playscii.jpg)<small><br/>Credit: [Playscii by JP Lebreton](https://jp.itch.io/playscii)</small>

Some efforts have been made to create the NuPetSCII charset to create ressources with Playscii + NupetSCII.

After installation of Playscii:
1. open the [nupetscii-for-playscii.zip](nupetscii-for-playscii.zip) archive.
2. extract the `nupetscii.char` and `nupetscii.png` files from the archive.
3. copy the files in the `/charsets` folder of Playscii software.

Voilà!

## Ressources
* [Playscii by JP Lebreton](https://jp.itch.io/playscii)
* [NupetScii charset for Playscii](nupetscii-for-playscii.zip) (zip file)

# Activating NuPetScii with Escape Sequence

NuPetScii can be activated from PicoTerm menu and with Esc sequence. So an application can request to the terminal to switch to semi-graphical font (AKA NuPetScii).

See the [using-nupetscii.md](../docs/using-nupetscii.md) document for more details.

# About nupetscii.data

The file `nupetscii.data` contains the definition of the additional chars. This file will be compiled with the `compile_font.py` python script to generate the expanded font charset named `nupetscii.c` .

`nupetscii.c` is the font file included within the PicoTerm sources.

Characters have the size of:
* 12px height * 8px wide (normal height)
* 15px height * 8px wide (extra height)

The char here below is the biggest possible.

``` c
/* U+7F DEL=Frame (128) */
{.bitmap_index = @bitmap_index@, .adv_w = 128, .box_h = 15, .box_w = 8, .ofs_x = 0, .ofs_y = -3}
FFFFFFFF
F......F
F......F
F......F
F......F
F......F
F......F
F......F
F......F
F......F
F......F
F......F
F......F
F......F
FFFFFFFF
```

Y offet start at the bottom of char (the baseline). Descending chars like p,q use negative offset to properly align the char on the baseline.

The font can have 15 row height at the maximum with a descending offset of 3 pixels (-3). Normal characters (limited to the baseline) do makes 12 rows.

The original font is coded with 4 bit per pixel (4 bpp), so with values from 0 to F for each point.

The hexadecimal notation (0 to F) are used in the data file (one letter for each 4bpp entry)!

The dot is used inplace of "0" to improve the readability.
