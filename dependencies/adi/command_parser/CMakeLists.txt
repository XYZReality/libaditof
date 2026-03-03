cmake_minimum_required(VERSION 3.0)
project(command_parser)

add_library(${PROJECT_NAME} STATIC
    command_parser.cpp
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
    PRIVATE
        .
)

# Install the library
install(TARGETS ${PROJECT_NAME}
    EXPORT command_parser-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# Install the header file
install(FILES command_parser.h
    DESTINATION include
)