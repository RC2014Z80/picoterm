/*
 * Terminal software for Pi Pico
 * USB keyboard input, VGA video output, communication with RC2014 via UART on GPIO20 &21
 * Shiela Dixon, https://peacockmedia.software
 *
 * main.c handles the ins and outs
 * picoterm.c handles the behaviour of the terminal and storing the text
 *
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */


#ifndef _PICOTERM_SCREEN_H
#define _PICOTERM_SCREEN_H

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "speccyfont.h"
#include "pico/scanvideo.h"

/* #define SPC         0x20
#define ESC         0x1b
#define DEL         0x7f
#define BSP         0x08
#define LF          0x0a
#define CR          0x0d
#define FF          0x0c */

char handle_default_input();

void display_terminal();

void display_config();
char handle_config_input();


#endif
