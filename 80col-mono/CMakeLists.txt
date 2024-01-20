cmake_minimum_required(VERSION 3.14)

set( PICOTERM_VERSION 1.6.0.32)
set( PICOTERM_DESCR "Gamma" )


message("PicoTerm 80Col version = ${PICOTERM_VERSION} ${PICOTERM_DESCR}") #  ${CMAKE_PROJECT_VERSION}


# PICO_SDK_PATH defined in environment variable? --> Use it
# Otherwise the variable should be exported. Read docs/compiling.md for details!
#
if(EXISTS "$ENV{PICO_SDK_PATH}")
  message("Environment variable PICO_SDK_PATH detected" )
  set(PICO_SDK_PATH "$ENV{PICO_SDK_PATH}" )
else()
  message("Environment variable PICO_SDK_PATH not present! Will rely on EXPORT PICO_SDK_PATH=..." )
endif()

message("PICO_SDK_PATH = ${PICO_SDK_PATH}" )

if(EXISTS "${PICO_SDK_PATH}")
	message("PICO_SDK_PATH successfully detected." )
else()
	message( FATAL_ERROR "Systen Environment variable PICO_SDK_PATH undefined or not exported!" )
endif()


# PICO_SDK_PATH: Copy the CMAKE dependencies to local folder
#
file(COPY ${PICO_SDK_PATH}/external/pico_sdk_import.cmake  DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/ )
file(COPY ${PICO_SDK_PATH}/../pico-extras/external/pico_extras_import.cmake  DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/ )

# PICO_SDK_PATH: Copy the FILES dependencies to local folder
#
file(COPY ${PICO_SDK_PATH}/../pico-playground/scanvideo/textmode/font.h  DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/ )

# FONT-SUITE: Copy the files dependencies to local folder
#
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../font-suite/mono8_cp437.c  DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/ )
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../font-suite/mono8_nupetscii.c  DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/ )
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../font-suite/olivetti_thin_cp437.c  DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/ )
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../font-suite/olivetti_thin_nupetscii.c  DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/ )


# Set alternative TinyUSB source code
#
#set(PICO_TINYUSB_PATH "/home/domeu/Bureau/RC2014/picoterm/80col-mono/tinyusb" )

include(pico_sdk_import.cmake)
include(pico_extras_import.cmake)
pico_sdk_init()
project(picoterm LANGUAGES CXX C ASM VERSION ${PICOTERM_VERSION} DESCRIPTION ${PICOTERM_DESCR}) # see issue 36

# Defines source files to compile
#    Not included in source: font6.c font10.c lcd.c font8.c
list( APPEND sources main.c )
list( APPEND sources picoterm_core.c )
list( APPEND sources ../common/picoterm_conio_config.c )
list( APPEND sources picoterm_conio.c )
list( APPEND sources mono8_cp437.c mono8_nupetscii.c olivetti_thin_cp437.c olivetti_thin_nupetscii.c )
list( APPEND sources picoterm_logo.c picoterm_screen.c )
list( APPEND sources ../common/picoterm_config.c )
list( APPEND sources ../common/picoterm_debug.c )
list( APPEND sources ../common/picoterm_stddef.c )
list( APPEND sources ../common/picoterm_stdio.c )
list( APPEND sources ../common/picoterm_cursor.c )
list( APPEND sources ../common/picoterm_dec.c )
list( APPEND sources ../common/keybd.c )
list( APPEND sources ../common/picoterm_i2c.c )
list( APPEND sources ../common/pca9536.c )
list( APPEND sources ../common/pio_spi.c )
list( APPEND sources ../common/pio_sd.c )
list( APPEND sources ../cli/cli.c )
list( APPEND sources ../cli/tinyexpr.c )
list( APPEND sources ../cli/user_funcs.c )
list( APPEND sources ../pio_fatfs/ff.c )
list( APPEND sources ../pio_fatfs/ffsystem.c )
list( APPEND sources ../pio_fatfs/ffunicode.c )
list( APPEND sources ../pio_fatfs/tf_card.c )

# Define the library to includes at compile time
#
list( APPEND link_libs pico_scanvideo_dpi pico_multicore pico_stdlib )
list( APPEND link_libs hardware_gpio hardware_i2c hardware_adc hardware_uart hardware_irq hardware_flash )
list( APPEND link_libs tinyusb_device tinyusb_board tinyusb_host )
list( APPEND hardware_clocks ) # fatfs: hardware_spi not included

# Defines the executables to build
# ===> APPEND ENTRY HERE FOR NEW KEYBOARD LAYOUT
#
list( APPEND to_build_list picoterm picoterm_US picoterm_DE picoterm_FR picoterm_BE )

# Common definition for all the executables
foreach( exec ${to_build_list} )
  add_executable( ${exec} ${sources} )
	pico_generate_pio_header( ${exec} ${CMAKE_CURRENT_SOURCE_DIR}/../common/uart_tx.pio)
	pico_generate_pio_header( ${exec} ${CMAKE_CURRENT_SOURCE_DIR}/../common/spi.pio)
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
		COLUMNS=80
		ROWS=34
		VISIBLEROWS=30
		${localise}=true
		PICO_XOSC_STARTUP_DELAY_MULTIPLIER=64
		CMAKE_PROJECT_VERSION="${CMAKE_PROJECT_VERSION}"
		)

	# Link libraries to use
	target_link_libraries( ${exec} ${link_libs} )
endforeach()
