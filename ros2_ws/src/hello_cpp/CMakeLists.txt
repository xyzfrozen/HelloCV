cmake_minimum_required(VERSION 3.8)
project(hello_cpp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(std_srvs REQUIRED)
find_package(rcl_interfaces REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(visualization_msgs REQUIRED)

function(add_learning_node target)
	add_executable(${target} src/${target}.cpp)
	ament_target_dependencies(${target} rclcpp ${ARGN})
	install(TARGETS ${target} DESTINATION lib/${PROJECT_NAME})
endfunction()

add_learning_node(hello_node)
add_learning_node(heartbeat)
add_learning_node(status_publisher std_msgs)
add_learning_node(status_listener std_msgs)
add_learning_node(detection_server std_srvs)
add_learning_node(detection_client std_srvs)
add_learning_node(threshold_node rcl_interfaces)
add_learning_node(visual_demo sensor_msgs visualization_msgs)

install(DIRECTORY launch config
	DESTINATION share/${PROJECT_NAME}
)

ament_package()
