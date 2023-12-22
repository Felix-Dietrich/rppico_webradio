add_library(
	flash_utils
	src/flash_utils/flash_utils.c
	src/flash_utils/flash_utils.h
	)



target_include_directories(flash_utils PUBLIC
"${CMAKE_CURRENT_SOURCE_DIR}/src")


target_link_libraries(flash_utils PUBLIC
	pico_stdlib
	pico_flash
	hardware_flash
	hardware_exception
)

