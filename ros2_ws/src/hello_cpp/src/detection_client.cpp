#include <chrono>
#include <memory>
#include "rclcpp/rclcpp.hpp"
#include "std_srvs/srv/set_bool.hpp"

using namespace std::chrono_literals;
using SetBool=std_srvs::srv::SetBool;

int main(int argc,char **argv)
{
	rclcpp::init(argc,argv);
	auto node=std::make_shared<rclcpp::Node>("detection_client");
	rclcpp::Client<SetBool>::SharedPtr client=
		node->create_client<SetBool>("set_detection");

	if(!client->wait_for_service(5s))
	{
		RCLCPP_ERROR(node->get_logger(),"service not available within 5 seconds");
		rclcpp::shutdown();
		return 1;
	}

	auto request=std::make_shared<SetBool::Request>();
	request->data=true;
	auto pending=client->async_send_request(request);
	auto result=rclcpp::spin_until_future_complete(node,pending,3s);

	if(result!=rclcpp::FutureReturnCode::SUCCESS)
	{
		client->remove_pending_request(pending);
		RCLCPP_ERROR(node->get_logger(),"no response: timeout or interrupted");
		rclcpp::shutdown();
		return 1;
	}

	auto response=pending.get();
	RCLCPP_INFO(node->get_logger(),"success=%s, message=%s",
		response->success?"true":"false",response->message.c_str());

	rclcpp::shutdown();
	return response->success?0:1;
}
