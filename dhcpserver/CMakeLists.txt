add_library(
	dhcpserver
	src/dhcpserver/dhcpserver.c
	src/dhcpserver/dhcpserver.h
	)



target_include_directories(dhcpserver PUBLIC
"${CMAKE_CURRENT_SOURCE_DIR}/src"
"${CMAKE_CURRENT_SOURCE_DIR}/.."
)


target_link_libraries(dhcpserver PUBLIC
	pico_stdlib
	pico_cyw43_arch_lwip_sys_freertos
	#pico_lwip_http
	#pico_lwip_mdns
	#lwip
	pico_lwip
	FreeRTOS-Kernel-Heap4
)

