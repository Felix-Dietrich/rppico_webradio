add_library(
	audioI2SAPI
	src/audioI2SAPI/audio_i2s_api.c
	src/audioI2SAPI/audio_i2s_api.h
	)



target_include_directories(audioI2SAPI PUBLIC
"${CMAKE_CURRENT_SOURCE_DIR}/src")


target_link_libraries(audioI2SAPI PUBLIC
	pico_stdlib
	pico_audio_i2s
)
target_compile_definitions( audioI2SAPI PUBLIC
# compile time configuration of I2S
PICO_AUDIO_I2S_MONO_INPUT=1
#define for our example code
USE_AUDIO_I2S=1
#            PICO_AUDIO_I2S_DATA_PIN=22
#            PICO_AUDIO_I2S_CLOCK_PIN_BASE=23
#            PICO_DEFAULT_UART=0
#            PICO_DEFAULT_UART_TX_PIN=28
#            PICO_DEFAULT_UART_RX_PIN=29
        )

