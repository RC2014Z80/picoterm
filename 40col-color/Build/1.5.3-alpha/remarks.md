This alpha version 1.5.3.80 is an upgrade "at the best" to integrates 80col
features at much as possible.

Notes about the 40 cols version:
* No secondary screen buffer (not enough memory), so cannot restore original
  screen after configuration screen is displayed.
* Support as much as VT100 sequences of 80col as possible.
* Word Wrap is activated by default (I don't know about previous version).
* As internal data structure for rendering is very much different from 80col,
  some bugs may still exists (it has been an hard work).
* Charset look only supporting 7 bits chars (see display charset screen)

Don't hesitate to open an issue about 40col implementation.
I will do the best to support it... but 40col it's not the priority.

Cheers,
Dominique
