# Set the project name.
project(Game)

# Set source files.
set(CPP_FILES main.cpp)

# Add the executable.
add_executable(${PROJECT_NAME} WIN32 ${CPP_FILES})

# Link libraries.
set(LIBRARIES gdiplus.lib uxtheme.lib)
target_link_libraries(${PROJECT_NAME} PUBLIC ${LIBRARIES})
