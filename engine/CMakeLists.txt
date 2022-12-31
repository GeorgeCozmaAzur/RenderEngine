file(GLOB BASE_SRC "*.cpp" "*.hpp" "*.h")
file(GLOB BASE_HEADERS  "*.hpp" "*.h")

set(DIR_RENDER "render")
set(DIR_SCENE "scene")
file(GLOB BASE_SRCR "${DIR_RENDER}/*.cpp")
file(GLOB BASE_HR "${DIR_RENDER}/*.h" "${DIR_RENDER}/*.hpp")
file(GLOB BASE_SRCS "${DIR_SCENE}/*.cpp")
file(GLOB BASE_HS "${DIR_SCENE}/*.h" "${DIR_SCENE}/*.hpp")
file(GLOB BASE_IMGUI "../external/imgui/*.cpp")

source_group("Scene\\Headers" FILES ${BASE_HS})
source_group("Scene\\Source" FILES ${BASE_SRCS})

source_group("Render\\Headers" FILES ${BASE_HR})
source_group("Render\\Source" FILES ${BASE_SRCR})

source_group("ImgGui" FILES ${BASE_IMGUI})

if(WIN32)
    add_library(engine STATIC ${BASE_SRC} ${BASE_SRCR} ${BASE_HR} ${BASE_SRCS} ${BASE_HS} ${BASE_IMGUI})
    target_link_libraries(engine ${Vulkan_LIBRARY} ${ASSIMP_LIBRARIES} ${WINLIBS})
 else(WIN32)
    add_library(engine STATIC ${BASE_SRC} ${BASE_SRCR} ${BASE_HR} ${BASE_SRCS} ${BASE_HS} ${BASE_IMGUI})
    target_link_libraries(engine ${Vulkan_LIBRARY} ${ASSIMP_LIBRARIES} ${XCB_LIBRARIES} ${WAYLAND_CLIENT_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
endif(WIN32)