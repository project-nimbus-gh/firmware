idf_component_register(
	SRCS
		"src/connectivity.cpp"
		"src/wifi_connection.cpp"
		"src/bluetooth_connection.cpp"
	INCLUDE_DIRS "include"
	REQUIRES 
		esp_event 
		esp_netif 
		esp_wifi 
		log 
		nvs_flash 
		bt 
		esp_common
)
