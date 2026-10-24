#include <chrono>
#include <cstddef>
#include <memory>
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/image.hpp"
#include "visualization_msgs/msg/marker.hpp"

using namespace std::chrono_literals;

class VisualDemo:public rclcpp::Node
{
public:
	VisualDemo():Node("visual_demo")
	{
		image_publisher_=create_publisher<sensor_msgs::msg::Image>("demo/image",10);
		marker_publisher_=create_publisher<visualization_msgs::msg::Marker>("demo/target",10);
		timer_=create_wall_timer(500ms,[this]()
		{
			publish_demo();
		});
	}

private:
	void publish_demo()
	{
		auto stamp=now();
		sensor_msgs::msg::Image image;
		image.header.stamp=stamp;
		image.header.frame_id="camera_optical_frame";
		image.width=64;
		image.height=48;
		image.encoding="rgb8";
		image.is_bigendian=false;
		image.step=image.width*3;
		image.data.resize(image.step*image.height);

		for(std::size_t y=0;y<image.height;++y)
		{
			for(std::size_t x=0;x<image.width;++x)
			{
				std::size_t index=y*image.step+x*3;
				image.data[index]=x*255/(image.width-1);
				image.data[index+1]=y*255/(image.height-1);
				image.data[index+2]=100;
			}
		}
		image_publisher_->publish(image);

		visualization_msgs::msg::Marker marker;
		marker.header.stamp=stamp;
		marker.header.frame_id="base_link";
		marker.ns="demo_targets";
		marker.id=0;
		marker.type=visualization_msgs::msg::Marker::CUBE;
		marker.action=visualization_msgs::msg::Marker::ADD;
		marker.pose.position.x=1.0;
		marker.pose.position.z=0.2;
		marker.pose.orientation.w=1.0;
		marker.scale.x=0.2;
		marker.scale.y=0.2;
		marker.scale.z=0.2;
		marker.color.g=1.0f;
		marker.color.a=1.0f;
		marker_publisher_->publish(marker);
	}

	rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr image_publisher_;
	rclcpp::Publisher<visualization_msgs::msg::Marker>::SharedPtr marker_publisher_;
	rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc,char **argv)
{
	rclcpp::init(argc,argv);
	rclcpp::spin(std::make_shared<VisualDemo>());

	rclcpp::shutdown();
	return 0;
}
