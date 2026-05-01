idf_component_register(
	SRCS
		"src/gpio_output.cpp"
		"src/gpio_input.cpp"
		"src/status_led.cpp"
	INCLUDE_DIRS "include"
	REQUIRES driver esp_driver_gpio
)
