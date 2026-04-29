idf_component_register(
	SRCS
		"app_main.cpp"
		"demo_telemetry.cpp"
		"status_led.cpp"
		"telemetry_packet.cpp"
	INCLUDE_DIRS "."
	REQUIRES driver log esp_driver_gpio
)
