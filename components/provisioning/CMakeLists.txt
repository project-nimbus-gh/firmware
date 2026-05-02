idf_component_register(
	SRCS
		"src/provisioning_manager.cpp"
	INCLUDE_DIRS "include"
	REQUIRES connect bt nvs_flash esp_system log
)
