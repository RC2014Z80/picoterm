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


#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#ifndef _PICOTERM_H
#define _PICOTERM_H

#include "speccyfont.h"
#include "pico/scanvideo.h"

uint32_t * wordsForRow(int y);
void prepare_text_buffer();
void slip_character(unsigned char ch,int x,int y);
void clear_cursor();
void print_cursor();
void handle_new_character(unsigned char ch);
void print_string(char str[]);
// for debugging purposes
void print_ascii_value(unsigned char asc);


#endif
