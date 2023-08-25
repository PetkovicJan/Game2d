# Set the project name.
project(TilingTool)

find_package(Qt5 COMPONENTS REQUIRED Core Widgets)

# Set source files.
set(CPP_FILES 
	main.cpp
	ConfiguringWidgetQt.cpp
	TilingWidgetQt.cpp)

# Set header files.
set(HPP_FILES
	ConfiguringWidgetQt.hpp
	TilingWidgetQt.hpp)
	
# Add the executable.
add_executable(${PROJECT_NAME} ${CPP_FILES} ${HPP_FILES})

# Link libraries.
set(LIBRARIES Qt5::Widgets Qt5::Core)
target_link_libraries(${PROJECT_NAME} PUBLIC ${LIBRARIES})

# Configure Qt moc.
set_target_properties(${PROJECT_NAME} PROPERTIES AUTOMOC TRUE AUTORCC TRUE)
