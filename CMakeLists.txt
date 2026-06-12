cmake_minimum_required(VERSION 3.12)
project(OpenCore LANGUAGES CXX)


file(GLOB_RECURSE OPEN_CORE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_library(${PROJECT_NAME} STATIC ${OPEN_CORE_SOURCES})

target_include_directories(${PROJECT_NAME}
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/modules/rapidjson/include"
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        SDL3
        SDL3_image
        SDL3_ttf
)
