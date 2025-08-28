# Base sources
file(GLOB BASE_SRC CONFIGURE_DEPENDS "*.cpp" "*.hpp" "*.h")

# Scene
file(GLOB BASE_SRCS CONFIGURE_DEPENDS "scene/*.cpp")
file(GLOB BASE_HS CONFIGURE_DEPENDS "scene/*.hpp" "scene/*.h")

# Render (common, plus subfolders)
file(GLOB BASE_SRCR CONFIGURE_DEPENDS "render/*.cpp")
file(GLOB BASE_HR   CONFIGURE_DEPENDS "render/*.hpp" "render/*.h")

file(GLOB RENDER_VULKAN_SRC CONFIGURE_DEPENDS "render/vulkan/*.cpp")
file(GLOB RENDER_VULKAN_HDR CONFIGURE_DEPENDS "render/vulkan/*.hpp" "render/vulkan/*.h")

file(GLOB RENDER_DIRECTX_SRC CONFIGURE_DEPENDS "render/directx/*.cpp")
file(GLOB RENDER_DIRECTX_HDR CONFIGURE_DEPENDS "render/directx/*.hpp" "render/directx/*.h")

# ImGui
file(GLOB BASE_IMGUI CONFIGURE_DEPENDS "../external/imgui/*.cpp")

# Library
add_library(engine STATIC
    ${BASE_SRC}
    ${BASE_SRCR} ${BASE_HR}
    ${RENDER_VULKAN_SRC} ${RENDER_VULKAN_HDR}
    ${RENDER_DIRECTX_SRC} ${RENDER_DIRECTX_HDR}
    ${BASE_SRCS} ${BASE_HS}
    ${BASE_IMGUI}
)

# Includes
target_include_directories(engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/render
    ${CMAKE_CURRENT_SOURCE_DIR}/render/vulkan
    ${CMAKE_CURRENT_SOURCE_DIR}/render/directx
    ${CMAKE_CURRENT_SOURCE_DIR}/scene
    ${CMAKE_CURRENT_SOURCE_DIR}/../external/imgui
)

# Platform specific libs
if(WIN32)
    target_link_libraries(engine ${Vulkan_LIBRARY} ${ASSIMP_LIBRARIES} ${WINLIBS})
else()
    target_link_libraries(engine ${Vulkan_LIBRARY} ${ASSIMP_LIBRARIES}
        ${XCB_LIBRARIES} ${WAYLAND_CLIENT_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT}
    )
endif()

# ---- Optional: Source groups (for Visual Studio organization) ----
source_group("Scene\\Headers" FILES ${BASE_HS})
source_group("Scene\\Source"  FILES ${BASE_SRCS})

source_group("Render\\Headers" FILES ${BASE_HR})
source_group("Render\\Source"  FILES ${BASE_SRCR})

source_group("Render\\Vulkan\\Headers" FILES ${RENDER_VULKAN_HDR})
source_group("Render\\Vulkan\\Source"  FILES ${RENDER_VULKAN_SRC})

source_group("Render\\DirectX\\Headers" FILES ${RENDER_DIRECTX_HDR})
source_group("Render\\DirectX\\Source"  FILES ${RENDER_DIRECTX_SRC})

source_group("ImgGui" FILES ${BASE_IMGUI})