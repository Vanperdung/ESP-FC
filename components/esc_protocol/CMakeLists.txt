set(SOURCES "oneshot125_protocol.cpp"
            "dshot_protocol.cpp")

idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS "."
    REQUIRES driver
)