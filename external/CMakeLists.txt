# External dependencies

# GLFW for window and input
# We'll use FetchContent to download GLFW
include(FetchContent)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.3.8
)

FetchContent_MakeAvailable(glfw)

# Example: fmt library
# add_subdirectory(fmt)
