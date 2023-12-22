add_library(
	dnsserver
	src/dnsserver/dnsserver.c
	src/dnsserver/dnsserver.h
	)



target_include_directories(dnsserver PUBLIC
"${CMAKE_CURRENT_SOURCE_DIR}/src"
"${CMAKE_CURRENT_SOURCE_DIR}/.."
)


target_link_libraries(dnsserver PUBLIC
	pico_stdlib
	pico_cyw43_arch_lwip_sys_freertos
	#pico_lwip_http
	#pico_lwip_mdns
	#lwip
	pico_lwip
	FreeRTOS-Kernel-Heap4
)

