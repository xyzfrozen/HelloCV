#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

using namespace std::chrono_literals;

class StatusPublisher:public rclcpp::Node
{
public:
	StatusPublisher():Node("status_publisher")
	{
		publisher_=create_publisher<std_msgs::msg::String>(
			"vision/status",rclcpp::QoS(10).reliable());

		timer_=create_wall_timer(500ms,[this]()
		{
			std_msgs::msg::String message;
			message.data="frame="+std::to_string(++count_)+", target=found";
			publisher_->publish(message);
			RCLCPP_INFO(get_logger(),"sent: %s",message.data.c_str());
		});
	}

private:
	std::uint64_t count_=0;
	rclcpp::Publisher<std_msgs::msg::String>::SharedPtr publisher_;
	rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc,char **argv)
{
	rclcpp::init(argc,argv);
	rclcpp::spin(std::make_shared<StatusPublisher>());

	rclcpp::shutdown();
	return 0;
}
