# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
set(PICO_SDK_PATH "/home/felix/Dokumente/raspberryPiPico/pico/pico-sdk")
set(FREERTOS_KERNEL_PATH "/home/felix/Dokumente/raspberryPiPico/pico/FreeRTOS-Kernel")

set(PICO_BOARD pico_w CACHE STRING "Board type")


# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)
include(/home/felix/Dokumente/raspberryPiPico/pico/FreeRTOS-Kernel/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)
include(pico_extras_import.cmake)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.4.0")
  message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.4.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

project(wifi_MP3_Stream C CXX ASM)

add_subdirectory(audioI2SAPI)
add_subdirectory(picomp3lib)

 #generate c from html for web server
 message("generate c from html for web server")
 execute_process(COMMAND
    python3 makefsdata.py 
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    )

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(wifi_MP3_Stream wifi_MP3_Stream.c)

pico_set_program_name(wifi_MP3_Stream "wifi_MP3_Stream")
pico_set_program_version(wifi_MP3_Stream "0.1")

pico_enable_stdio_uart(wifi_MP3_Stream 0)
pico_enable_stdio_usb(wifi_MP3_Stream 1)

# Add the standard library to the build
target_link_libraries(wifi_MP3_Stream
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(wifi_MP3_Stream PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts or any other standard includes, if required
  
)

target_compile_definitions(wifi_MP3_Stream PRIVATE
  CYW43_HOST_NAME="deDietrich"
)


include_directories(${CMAKE_CURRENT_LIST_DIR})

# Add any user requested libraries
target_link_libraries(wifi_MP3_Stream
        #pico_cyw43_arch_lwip_threadsafe_background
        pico_cyw43_arch_lwip_sys_freertos
        pico_lwip_http
        pico_lwip_mdns
        FreeRTOS-Kernel-Heap4 # FreeRTOS kernel and dynamic heap
        audioI2SAPI
        picomp3lib
        hardware_adc
        )
 
pico_set_linker_script(wifi_MP3_Stream ${CMAKE_SOURCE_DIR}/memmap_reserved_flash.ld)

pico_add_extra_outputs(wifi_MP3_Stream)

