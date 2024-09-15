# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize flags
set(PICO_EXTRAS_FETCH_FROM_GIT OFF)
set(FELIX OFF)
set(MANFRED OFF)

if(APPLE)
  set(USER_ENV $ENV{USER})
  message(STATUS "Hallo ${USER_ENV}")
  if(${USER_ENV} STREQUAL "felix")
    set(FELIX ON)
  elseif(${USER_ENV} STREQUAL "manfred")
    set(MANFRED ON)
  endif()
endif()

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
if(MANFRED)
  message("😀 Mac OS Konfiguration von Manfred")
  set(PICO_SDK_PATH "/Users/manfred/Documents/Manfred/Entwicklung/Projekte/pi/pico/pico-sdk")
  set(FREERTOS_KERNEL_PATH "/Users/manfred/Documents/Manfred/Entwicklung/Projekte/pi/pico/FreeRTOS-Kernel")
elseif(FELIX)
  message("😀 UNIX Konfiguration von Felix")
  set(PICO_SDK_PATH "/Users/felix/Documents/Hobby/Projekte/pico/pico-sdk")
  set(FREERTOS_KERNEL_PATH "/Users/felix/Documents/Hobby/Projekte/pico/FreeRTOS-Kernel")
else()
  message("😬 Für dieses Betriebsystem gibt es noch keine Konfiguration.")
endif()


set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

if(MANFRED)
  message("😀 Mac OS Konfiguration von Manfred")
  include(/Users/manfred/Documents/Manfred/Entwicklung/Projekte/pi/pico/FreeRTOS-Kernel/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)
elseif(FELIX)
  message("😀 UNIX Konfiguration von Felix")
  include(/Users/felix/Documents/Hobby/Projekte/pico/FreeRTOS-Kernel/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)
else()
  message("😬 Für dieses Betriebsystem gibt es noch keine Konfiguration.")
endif()


include(pico_extras_import.cmake)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.4.0")
  message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.4.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

project(wifi_MP3_Stream C CXX ASM)

add_subdirectory(audioI2SAPI)
add_subdirectory(flash_utils)
add_subdirectory(picomp3lib)
add_subdirectory(dnsserver)
add_subdirectory(dhcpserver)

 #generate c from html for web server
 message("generate c from html for web server")
 execute_process(COMMAND
    python3 makefsdata.py 
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    )

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Get the list of source files in in the 'RTOS_tasks' folder
file(GLOB TASK_SOURCES RTOS_tasks/*.c)

# Add executable. Default name is the project name, version 0.1
add_executable(wifi_MP3_Stream wifi_MP3_Stream.c ${TASK_SOURCES})

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
  ${CMAKE_CURRENT_LIST_DIR}/RTOS_tasks
  
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
        flash_utils
        dnsserver
        dhcpserver
        )
 
pico_set_linker_script(wifi_MP3_Stream ${CMAKE_SOURCE_DIR}/memmap_reserved_flash.ld)

pico_add_extra_outputs(wifi_MP3_Stream)

