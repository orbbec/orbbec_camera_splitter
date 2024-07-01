cmake_minimum_required(VERSION 3.8)
project(orbbec_camera_splitter)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
set(CMAKE_CXX_STANDARD 17)
add_compile_options(-Wall -Wextra -Wpedantic)

set(CMAKE_CXX_STANDARD 17)
add_compile_options(-Wall -Wextra -Wpedantic)

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(ament_cmake_ros REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_components REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(message_filters REQUIRED)
find_package(orbbec_camera_msgs REQUIRED)
find_package(nvblox_ros_common REQUIRED)
find_package(isaac_ros_common REQUIRED)
find_package(CUDAToolkit REQUIRED)


#############
# LIBRARIES #
#############
add_library(${PROJECT_NAME}_component
  src/${PROJECT_NAME}_node.cpp
)
target_compile_definitions(${PROJECT_NAME}_component
  PRIVATE "COMPOSITION_BUILDING_DLL")
ament_target_dependencies(${PROJECT_NAME}_component
  rclcpp
  rclcpp_components
  sensor_msgs
  message_filters
  orbbec_camera_msgs
  nvblox_ros_common
  isaac_ros_common
)
target_include_directories(${PROJECT_NAME}_component PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)

rclcpp_components_register_nodes(${PROJECT_NAME}_component "nvblox::OrbbecCameraSplitterNode")
set(node_plugins "${node_plugins}nvblox::OrbbecCameraSplitterNode;$<TARGET_FILE:${PROJECT_NAME}_component>\n")


############
# BINARIES #
############
add_executable(${PROJECT_NAME}_node
  src/${PROJECT_NAME}_node_main.cpp
)
target_link_libraries(${PROJECT_NAME}_node ${PROJECT_NAME}_component)

###########
# INSTALL #
###########

# includes
install(
  DIRECTORY include/
  DESTINATION include
)

# libs
install(
  TARGETS ${PROJECT_NAME}_component
  EXPORT ${PROJECT_NAME}Targets
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

# nodes
install(TARGETS ${PROJECT_NAME}_node
  DESTINATION lib/${PROJECT_NAME})

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

##########
# EXPORT #
##########
ament_export_include_directories(
  include
)
ament_export_libraries(
  ${PROJECT_NAME}_component
)
ament_export_targets(
  ${PROJECT_NAME}Targets
)

ament_export_dependencies(
  rclcpp
  sensor_msgs
)

ament_package()
