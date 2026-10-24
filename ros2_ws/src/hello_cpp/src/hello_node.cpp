#include <memory>
#include "rclcpp/rclcpp.hpp"

int main(int argc,char **argv)
{
	rclcpp::init(argc,argv);
	auto node=std::make_shared<rclcpp::Node>("hello_cv");

	RCLCPP_INFO(node->get_logger(),"HelloCV: node is ready");
	rclcpp::spin(node);

	rclcpp::shutdown();
	return 0;
}
