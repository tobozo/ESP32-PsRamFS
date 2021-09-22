idf_component_register(SRCS "src/pfs.c" INCLUDE_DIRS "src")
#target_compile_definitions(${COMPONENT_LIB} PUBLIC "-DLOG_LOCAL_LEVEL=ESP_LOG_VERBOSE")
