set(SOURCES "esc_controller.cpp")

idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS "."
    REQUIRES driver esc_protocol
)