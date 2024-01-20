/*
 * Terminal software for Pi Pico
 * USB keyboard input, VGA video output, communication with RC2014 via UART on GPIO20 & 21
 *
 * Based on work by
 * - Shiela Dixon     (picoterm) https://peacockmedia.software
 * - TinyUSB hid_app demo
 * - Daniel Quadros   (RPTerm)   https://github.com/dquadros/RPTerm/blob/main/keybd.cpp
 *
 * This file handles the USB keyboard
 * Based in the SDK example for tinyusb v0.13.0
 *
 * The The HID host code in the tinyUSB stack will select the boot protocol and
 * a zero idle rate (device only send reports if there is a change) when a HID
 * device is mounted.
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

 /* Oct 27, 2022 : Domeu : Extract code from main.c by following work made by Daniel Quadros */

#include "bsp/board.h"
#include "tusb.h"

#include "keybd.h"
#include "pmhid.h"
#include "picoterm_debug.h"

// New TinyUSB stuff
enum {
    SDL_SCANCODE_SPACE = 44,
    SDL_SCANCODE_LCTRL = 224,
    SDL_SCANCODE_LSHIFT = 225,
    SDL_SCANCODE_LALT = 226, /**< alt, option */
    SDL_SCANCODE_LGUI = 227, /**< windows, command (apple), meta */
    SDL_SCANCODE_RCTRL = 228,
    SDL_SCANCODE_RSHIFT = 229,
    SDL_SCANCODE_RALT = 230, /**< alt gr, option */
    SDL_SCANCODE_RGUI = 231, /**< windows, command (apple), meta */
};


 // A buffer for characters coming in from UART. It's of limited value as things stand
 // but will come into its own if and when we switch to interrupts for UART
 struct KeyboardBuffer {
   int length;
   int take;
   int insert;
   char buff[2000];
 };

 struct KeyboardBuffer keybuffer1 = {0};

 void keybd_init( key_change_cb_t key_down_callback, key_change_cb_t key_up_callback ){
     keybd_dev_addr = UNDEFINED_ADDR;

     key_down_cb = key_down_callback; // callbacks NULL accepted
     key_up_cb = key_up_callback;

     keybuffer1.length=2000;
     keybuffer1.take=0;
     keybuffer1.insert=0;
}

static void default_key_down(int scancode, int keysym, int modifiers);

static bool capslock_key_down_in_last_report = false;
static bool capslock_key_down_in_this_report = false;
static bool capslock_on = false;

// --- Keyboard repeat feature ----------------------------
static int __last_key_down_scancode = 0; // NULL
static int __last_key_down_modifier = 0;
static uint32_t __last_key_down = 0;        // time of the last key down
static uint32_t __last_key_down_resent = 0; // time of the last keydown resent

static int keydown_start_repeat_delay = 500; // Start repeat a character after x ms
static int keydown_resent_delay = 50; // interval between multiple repeat in ms

void set_last_key_down( int scancode, int keysym, int modifiers );
void clear_last_key_down( int scancode, int keysym, int modifiers );

bool keyboard_attached(){
  return keybd_dev_addr != UNDEFINED_ADDR;
}

 //--------------------------------------------------------------------+
 // USB HID
 //--------------------------------------------------------------------+

 #define MAX_REPORT  4

 // Each HID instance can has multiple reports
 static struct
 {
     uint8_t report_count;
     tuh_hid_report_info_t report_info[MAX_REPORT];
 }hid_info[CFG_TUH_HID];

 static void process_kbd_report(hid_keyboard_report_t const *report);
 //static void process_mouse_report(hid_mouse_report_t const * report);
 static void process_generic_report(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len);


//--------------------------------------------------------------------+
// Keyboard Buffer Routine
//--------------------------------------------------------------------+

void insert_key_into_buffer(unsigned char ch){
   keybuffer1.buff[keybuffer1.insert++]=ch;
   if(keybuffer1.insert==keybuffer1.length)keybuffer1.insert=0;
}

bool key_ready(){
     return (keybuffer1.take!=keybuffer1.insert);
}

unsigned char read_key_from_buffer(){
     unsigned char ch=keybuffer1.buff[keybuffer1.take++];
     if(keybuffer1.take==keybuffer1.length)keybuffer1.take=0;
     return ch;
}

void clear_key_buffer(){
    while( key_ready() )
      read_key_from_buffer();
}

void key_repeat_task(){
    // Check if key is maintained down too enough to start repeating
    // Must be called from the main loop()
    if( (__last_key_down_scancode != 0) && ((board_millis()-__last_key_down)>=keydown_start_repeat_delay) )
      if( (board_millis() - __last_key_down_resent)>keydown_resent_delay ) {
        //sprintf( debug_msg, "keydown repeat for %i", __last_key_down_scancode );
        //debug_print( debug_msg );
        __last_key_down_resent = board_millis();
        if( key_down_cb != NULL)
          key_down_cb( __last_key_down_scancode, 0, __last_key_down_modifier );
      }

}

//--------------------------------------------------------------------+
// TinyUSB Callbacks - Generic report
//--------------------------------------------------------------------+
// called by tiny USB when receiving an USB Report
static void process_generic_report(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len) {
     (void) dev_addr;

     uint8_t const rpt_count = hid_info[instance].report_count;
     tuh_hid_report_info_t* rpt_info_arr = hid_info[instance].report_info;
     tuh_hid_report_info_t* rpt_info = NULL;

     if ( rpt_count == 1 && rpt_info_arr[0].report_id == 0)
     {
         // Simple report without report ID as 1st byte
         rpt_info = &rpt_info_arr[0];
     }
     else {
         // Composite report, 1st byte is report ID, data starts from 2nd byte
         uint8_t const rpt_id = report[0];

         // Find report id in the arrray
         for(uint8_t i=0; i<rpt_count; i++)
         {
             if (rpt_id == rpt_info_arr[i].report_id )
             {
                 rpt_info = &rpt_info_arr[i];
                 break;
             }
         }

         report++;
         len--;
     }

     if (!rpt_info) {
         sprintf( debug_msg, "Couldn't find the report info for this report !");
         debug_print( debug_msg );
         return;
     }

     // For complete list of Usage Page & Usage checkout src/class/hid/hid.h. For examples:
     // - Keyboard                     : Desktop, Keyboard
     // - Mouse                        : Desktop, Mouse
     // - Gamepad                      : Desktop, Gamepad
     // - Consumer Control (Media Key) : Consumer, Consumer Control
     // - System Control (Power key)   : Desktop, System Control
     // - Generic (vendor)             : 0xFFxx, xx
     if ( rpt_info->usage_page == HID_USAGE_PAGE_DESKTOP )
     {
         switch (rpt_info->usage)
         {
             case HID_USAGE_DESKTOP_KEYBOARD:
                 TU_LOG1("HID receive keyboard report\r\n");
                 // Assume keyboard follow boot report layout
                 process_kbd_report( (hid_keyboard_report_t const*) report );
                 break;

             default: break;
         }
     }
}

//--------------------------------------------------------------------+
// TinyUSB callback
//--------------------------------------------------------------------+


// Invoked when device with hid interface is mounted
// Report descriptor is also available for use. tuh_hid_parse_report_descriptor()
// can be used to parse common/simple enough descriptor.
// Note: if report descriptor length > CFG_TUH_ENUMERATION_BUFSIZE, it will be skipped
// therefore report_desc = NULL, desc_len = 0
void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len) {
    sprintf( debug_msg, "HID device address = %d, instance = %d is mounted", dev_addr, instance );
    debug_print( debug_msg );

    // Interface protocol (hid_interface_protocol_enum_t)
    const char* protocol_str[] = { "None", "Keyboard", "Mouse" };
    uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);
    sprintf( debug_msg, "HID Interface Protocol = %s", protocol_str[itf_protocol] );
    debug_print( debug_msg );

    if( itf_protocol == 1 ) { // Did we attached a keyboard? Only pump report for keyboards
      keybd_dev_addr = dev_addr;

	   // printf("%d USB: device %d connected, protocol %s\n", time_us_32() - t0 , dev_addr, protocol_str[itf_protocol]);

	    // By default host stack will use activate boot protocol on supported interface.
	    // Therefore for this simple example, we only need to parse generic report descriptor (with built-in parser)
	    if ( itf_protocol == HID_ITF_PROTOCOL_NONE ) {
	        hid_info[instance].report_count = tuh_hid_parse_report_descriptor(hid_info[instance].report_info, MAX_REPORT, desc_report, desc_len);
	        sprintf( debug_msg, "HID has %u reports \r\n", hid_info[instance].report_count );
	       debug_print( debug_msg );
	    }

	    // request to receive report tuh_hid_report_received_cb() will be invoked when report is available
	    if ( !tuh_hid_receive_report(dev_addr, instance) ) {
	        debug_print("HID Error: cannot request to receive report");
	    }
		}
}

// Invoked when device with hid interface is un-mounted
void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {
    sprintf( debug_msg, "HID device address = %d, instance = %d is unmounted", dev_addr, instance);
    debug_print( debug_msg );

    if( dev_addr == keybd_dev_addr ) // did we disconnect the detected Keyboard?
      keybd_dev_addr = UNDEFINED_ADDR;
}

// Invoked when received report from device via interrupt endpoint
void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
    uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);

    switch (itf_protocol)
    {
        case HID_ITF_PROTOCOL_KEYBOARD:
            TU_LOG2("HID receive boot keyboard report");
            process_kbd_report( (hid_keyboard_report_t const*) report );
            break;


        default:
            // Generic report requires matching ReportID and contents with previous parsed report info
            process_generic_report(dev_addr, instance, report, len);
            break;
    }

    // continue to request to receive report
    if ( !tuh_hid_receive_report(dev_addr, instance) )
    {
        debug_print("Error: cannot request to receive report");
    }
}


//--------------------------------------------------------------------+
// Keyboard
//--------------------------------------------------------------------+
static void default_key_down(int scancode, int keysym, int modifiers);

void set_last_key_down( int scancode, int keysym, int modifiers ){
   __last_key_down_scancode = scancode;
   __last_key_down_modifier = modifiers;
   __last_key_down = board_millis();
   __last_key_down_resent = 0;
}

void clear_last_key_down( int scancode, int keysym, int modifiers ){
  __last_key_down_scancode = 0; // NULL
  __last_key_down_modifier = 0;
}

// look up new key in previous keys
static inline bool find_key_in_report(hid_keyboard_report_t const *report, uint8_t keycode) {
    for(uint8_t i=0; i<6; i++) {
        if (report->keycode[i] == keycode)
          return true;
    }
    return false;
}


static void process_kbd_report(hid_keyboard_report_t const *report) {
    static hid_keyboard_report_t prev_report = { 0, 0, {0} }; // previous report to check key released

    //------------- example code ignore control (non-printable) key affects -------------//
    for(uint8_t i=0; i<6; i++)
    {
        if ( report->keycode[i] )
        {


            capslock_key_down_in_this_report = false;
            for(uint8_t i=0; i<6; i++){
                if ( find_key_in_report(report, HID_KEY_CAPS_LOCK)){
                capslock_key_down_in_this_report = true;
                }
            }
            if(capslock_key_down_in_this_report==true && capslock_key_down_in_last_report==false){
                // toggle the value
                capslock_on = !capslock_on;
            }


            if ( find_key_in_report(&prev_report, report->keycode[i]) )
            {
                // exist in previous report means the current key is holding
                // Domeu note: Append when keeping "a" pressed & pressing releasing another key.
                // Domeu note: When holding only the "a" pressed... then nothing happens.
                //debug_print( "Key Holding");
            }else
            {
                // not existed in previous report means the current key is pressed
                bool const is_shift = report->modifier & (KEYBOARD_MODIFIER_LEFTSHIFT | KEYBOARD_MODIFIER_RIGHTSHIFT);
                bool const is_altgr = report->modifier & (KEYBOARD_MODIFIER_RIGHTALT);
                bool const is_ctrl = report->modifier & (KEYBOARD_MODIFIER_LEFTCTRL | KEYBOARD_MODIFIER_RIGHTCTRL);
                // capslock_on

                int modifier = 0;
                modifier = modifier | (is_shift ? WITH_SHIFT : 0);
                modifier = modifier | (is_altgr ? WITH_ALTGR : 0);
                modifier = modifier | (is_ctrl ? WITH_CTRL : 0);
                modifier = modifier | (capslock_on ? WITH_CAPSLOCK : 0);

                //pico_key_down(report->keycode[i], 0, modifier);
                set_last_key_down( report->keycode[i], 0, modifier );
                if( key_down_cb != NULL )
                  key_down_cb( report->keycode[i], 0, modifier );
                else
                  default_key_down( report->keycode[i], 0, modifier ); // just add it to caracter buffer
            }
        }
        // Check for key depresses (i.e. was present in prev report but not here)
        if (prev_report.keycode[i]) {
            // If not present in the current report then depressed
            if (!find_key_in_report(report, prev_report.keycode[i]))
            {
                bool const is_shift = report->modifier & (KEYBOARD_MODIFIER_LEFTSHIFT | KEYBOARD_MODIFIER_RIGHTSHIFT);
                bool const is_altgr = report->modifier & (KEYBOARD_MODIFIER_RIGHTALT);
                bool const is_ctrl = report->modifier & (KEYBOARD_MODIFIER_LEFTCTRL | KEYBOARD_MODIFIER_RIGHTCTRL);
                // capslock_on

                int modifier = 0;
                modifier = modifier | (is_shift ? WITH_SHIFT : 0);
                modifier = modifier | (is_altgr ? WITH_ALTGR : 0);
                modifier = modifier | (is_ctrl ? WITH_CTRL : 0);
                modifier = modifier | (capslock_on ? WITH_CAPSLOCK : 0);

                //pico_key_up(prev_report.keycode[i], 0, modifier);
                clear_last_key_down( prev_report.keycode[i], 0, modifier );
                if( key_up_cb != NULL )
                  key_up_cb( prev_report.keycode[i], 0, modifier );
            }
        }
    }

    /*
    // synthesize events for modifier keys
    static const uint8_t mods[] = {
            KEYBOARD_MODIFIER_LEFTCTRL, SDL_SCANCODE_LCTRL,
            KEYBOARD_MODIFIER_RIGHTCTRL, SDL_SCANCODE_RCTRL,
            KEYBOARD_MODIFIER_LEFTALT, SDL_SCANCODE_LALT,
            KEYBOARD_MODIFIER_RIGHTALT, SDL_SCANCODE_RALT,
            KEYBOARD_MODIFIER_LEFTSHIFT, SDL_SCANCODE_LSHIFT,
            KEYBOARD_MODIFIER_RIGHTSHIFT, SDL_SCANCODE_RSHIFT,
    };
    for(int i=0;i<count_of(mods); i+= 2) {
        check_mod(report->modifier, prev_report.modifier, mods[i], mods[i+1]);
    }
    */

    prev_report = *report;
}

bool scancode_is_mod(int scancode) {
    static const uint8_t mods[] = {
            SDL_SCANCODE_LCTRL,
            SDL_SCANCODE_RCTRL,
            SDL_SCANCODE_LALT,
            SDL_SCANCODE_RALT,
            SDL_SCANCODE_LSHIFT,
            SDL_SCANCODE_RSHIFT,
    };
    for(int i=0;i<count_of(mods); i+= 2) {
        if(scancode== mods[i]){
          return true;
        }
    }
    // default
    return false;
}

signed char scancode_has_esc_seq(int scancode){
    // Some ScanCode are converted to specific esccape sequence
    // (like Key right, Key Left, etc).
    // Return the index in the structure (or -1)
    for( int i=0; i<PM_ESC_SEQ_COUNT; i++ )
      if( keycode2escseq[i][0]==scancode )
        return i;
    return -1;
}

int scancode_esc_seq_len(uint8_t index){
    // len of chars in the Escape Sequence
    return keycode2escseq[index][1];
}

char scancode_esc_seq_item(uint8_t index, uint8_t pos ) {
    // One of the char of the escape sequence
    return keycode2escseq[index][2+pos];
}


static void default_key_down(int scancode, int keysym, int modifiers) {
    // Create your own key_down event handler & assign it to `key_down_cb`
    if( scancode_is_mod(scancode)==false ){
      // which char at that key?
      uint8_t ch = keycode2ascii[scancode][0];
      if( modifiers & WITH_SHIFT ){
          ch = keycode2ascii[scancode][1];
      }
      else if((modifiers & WITH_CAPSLOCK) && ch>='a' && ch<='z'){
          ch = keycode2ascii[scancode][1];
      }
      else if(modifiers & WITH_CTRL && ch>95){
          ch=ch-96;
      }
      else if(modifiers & WITH_ALTGR){
          ch = keycode2ascii[scancode][2];
      }
      // Send it directly to the keyboard buffer
			insert_key_into_buffer( ch );
    }
}

//--------------------------------------------------------------------+
// Mouse
//--------------------------------------------------------------------+

// see https://github.com/dquadros/RPTerm/blob/main/keybd.cpp for exemple
