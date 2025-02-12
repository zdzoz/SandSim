if(APPLE)
    add_library(sokol sokol.m)
    target_link_libraries(sokol PUBLIC
        "-framework Cocoa"
        "-framework QuartzCore"
        "-framework Metal"
        "-framework MetalKit"
    )
else(sokol)
    add_library(sokol sokol.c)
endif()

target_include_directories(sokol PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

