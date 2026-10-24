#include <memory>
#include "rclcpp/rclcpp.hpp"
#include "std_srvs/srv/set_bool.hpp"

using SetBool=std_srvs::srv::SetBool;

class DetectionServer:public rclcpp::Node
{
public:
	DetectionServer():Node("detection_server")
	{
		service_=create_service<SetBool>("set_detection",
			[this](const std::shared_ptr<SetBool::Request> request,
				std::shared_ptr<SetBool::Response> response)
			{
				enabled_=request->data;
				response->success=true;
				response->message=enabled_?"detection enabled":"detection disabled";
				RCLCPP_INFO(get_logger(),"%s",response->message.c_str());
			});
	}

private:
	bool enabled_=false;
	rclcpp::Service<SetBool>::SharedPtr service_;
};

int main(int argc,char **argv)
{
	rclcpp::init(argc,argv);
	rclcpp::spin(std::make_shared<DetectionServer>());

	rclcpp::shutdown();
	return 0;
}
