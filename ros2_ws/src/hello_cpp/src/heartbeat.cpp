#include <chrono>
#include <cstdint>
#include <memory>
#include "rclcpp/rclcpp.hpp"

using namespace std::chrono_literals; //可以使用时间字面量 1s 500ms

class Heartbeat:public rclcpp::Node //继承 Node
{

public:
	Heartbeat():Node("heartbeat") //构造函数
	{
		timer_=create_wall_timer(1s,[this]() //lambda 表达式，懒得写函数 this捕获对象指针
		{
			RCLCPP_INFO(get_logger(),"heartbeat=%llu",
				static_cast<unsigned long long>(++count_));
		});
	}

private:
	std::uint64_t count_=0;
	rclcpp::TimerBase::SharedPtr timer_; //指向定时器对象的共享智能指针

};

int main(int argc,char **argv)
{
	rclcpp::init(argc,argv);

    rclcpp::spin(std::make_shared<Heartbeat>());
    //auto node=std::make_shared<Heartbeat>();
	//rclcpp::spin(node);

    rclcpp::shutdown();
	return 0;
}
