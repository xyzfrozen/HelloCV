#include <memory>
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

class StatusListener:public rclcpp::Node
{
public:
	StatusListener():Node("status_listener")
	{
		subscription_=create_subscription<std_msgs::msg::String>(
			"vision/status",rclcpp::QoS(10).reliable(),
			[this](std_msgs::msg::String::ConstSharedPtr message)
			{
				RCLCPP_INFO(get_logger(),"received: %s",message->data.c_str());
			});
	}

private:
	rclcpp::Subscription<std_msgs::msg::String>::SharedPtr subscription_;
};

int main(int argc,char **argv)
{
	rclcpp::init(argc,argv);
	rclcpp::spin(std::make_shared<StatusListener>());

	rclcpp::shutdown();
	return 0;
}
