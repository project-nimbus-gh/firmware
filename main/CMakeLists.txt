idf_component_register(
	SRCS
		"src/app_main.cpp"
		"src/demo_telemetry.cpp"
	INCLUDE_DIRS "include"
	REQUIRES protocol driver connect log
)
