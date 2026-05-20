cmake_minimum_required(VERSION 3.10)
project(libapdu VERSION 1.0.0 LANGUAGES C)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Library sources
set(APDU_SOURCES
    src/apdu.c
)

set(APDU_PUBLIC_HEADERS
    include/libapdu/apdu.h
)

# Create the library
add_library(${PROJECT_NAME} STATIC ${APDU_SOURCES})

# Specify include directories
target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Specify C99 for the target
set_target_properties(${PROJECT_NAME} PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
)

# Install rules
include(GNUInstallDirs)

install(TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}Targets
    ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES ${APDU_PUBLIC_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/libapdu
)

install(EXPORT ${PROJECT_NAME}Targets
    FILE ${PROJECT_NAME}Targets.cmake
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

# Optional: build shared library
option(BUILD_SHARED_LIBS "Build shared library" OFF)

# Testing
option(BUILD_TESTING "Build tests" ON)

if(BUILD_TESTING)
    enable_testing()

    add_executable(test_apdu
        tests/test_apdu.c
    )
    target_link_libraries(test_apdu PRIVATE ${PROJECT_NAME})
    target_include_directories(test_apdu PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

    add_test(NAME test_apdu COMMAND test_apdu)
endif()
