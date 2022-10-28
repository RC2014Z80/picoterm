cmake_minimum_required(VERSION 3.14)

#set(PICO_BOARD "vgaboard")

#set(PICO_TINYUSB_PATH "/home/domeu/Bureau/RC2014/picoterm/80col-mono/tinyusb" )
include(pico_sdk_import.cmake)
include(pico_extras_import.cmake)
pico_sdk_init()

project(picoterm VERSION 1.2.1)
message("PicoTerm 80Col version = ${CMAKE_PROJECT_VERSION}")

# Defines source files to compile
#    Not included in source: font6.c font10.c lcd.c
list( APPEND sources main.c picoterm.c font8.c )
list( APPEND sources ../common/picoterm_config.c )
list( APPEND sources ../common/picoterm_debug.c )
list( APPEND sources ../common/keybd.c )

# Define the library to includes at compile time
#
list( APPEND link_libs pico_scanvideo_dpi pico_multicore pico_stdlib )
list( APPEND link_libs hardware_gpio hardware_i2c hardware_adc hardware_uart hardware_irq hardware_flash )
list( APPEND link_libs tinyusb_device tinyusb_board tinyusb_host )

# Defines the executables to build
# ===> APPEND ENTRY HERE FOR NEW KEYBOARD LAYOUT
#
list( APPEND to_build picoterm picoterm_US picoterm_DE picoterm_FR picoterm_BE )


# Common definition for all the executables
foreach( exec ${to_build} )
  add_executable( ${exec} ${sources} )
	pico_generate_pio_header( ${exec} ../../common/uart_tx.pio)
	pico_enable_stdio_usb( ${exec} 0 ) # Redirect printf to USB-Serial. Conflict with TinyUSB!
	pico_enable_stdio_uart( ${exec} 0) # Redirect printf to uart0. But no room to place UART0 on GPIO!

	# Necessary to include tusb_config.h
	target_include_directories( ${exec} PRIVATE ${CMAKE_CURRENT_LIST_DIR} )

	pico_add_extra_outputs( ${exec} )

	# create the LOCALISE_xx string from the picoterm_xx to build
	if( ${exec} STREQUAL "picoterm" ) # picoterm  -> LOCALISE_UK (special case)
    set( localise "LOCALISE_UK" )
	else() # picoterm_FR -> LOCALISE_FR
    string(REGEX REPLACE "picoterm_" "LOCALISE_" localise ${exec} )
	endif()
	message( "Localise for ${exec} = ${localise}")

	# Common DEFINES for all executables
	target_compile_definitions( ${exec} PRIVATE
		PICO_SCANVIDEO_SCANLINE_BUFFER_COUNT=4
		PICO_SCANVIDEO_PLANE1_FIXED_FRAGMENT_DMA=true
		${localise}=true
		CMAKE_PROJECT_VERSION="${CMAKE_PROJECT_VERSION}"
		)

	# Link libraries to use
	target_link_libraries( ${exec} ${link_libs} )
endforeach()
