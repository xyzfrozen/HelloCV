#include <chrono>
#include <cmath>
#include <memory>
#include <stdexcept>
#include <vector>
#include "rclcpp/rclcpp.hpp"
#include "rcl_interfaces/msg/set_parameters_result.hpp"

using namespace std::chrono_literals;

class ThresholdNode:public rclcpp::Node
{
public:
	ThresholdNode():Node("threshold_node")
	{
		double initial=declare_parameter<double>("threshold",0.60);
		if(!std::isfinite(initial)||initial<0.0||initial>1.0)
		{
			throw std::invalid_argument("initial threshold must be in [0,1]");
		}

		parameter_callback_=add_on_set_parameters_callback(
			[](const std::vector<rclcpp::Parameter> &parameters)
			{
				rcl_interfaces::msg::SetParametersResult result;
				result.successful=true;

				for(const auto &parameter:parameters)
				{
					if(parameter.get_name()!="threshold")
					{
						continue;
					}

					if(parameter.get_type()!=rclcpp::ParameterType::PARAMETER_DOUBLE)
					{
						result.successful=false;
						result.reason="threshold must be a double";
						break;
					}

					double value=parameter.as_double();
					if(!std::isfinite(value)||value<0.0||value>1.0)
					{
						result.successful=false;
						result.reason="threshold must be in [0,1]";
						break;
					}
				}
				return result;
			});

		timer_=create_wall_timer(1s,[this]()
		{
			const double confidence=0.70;
			double threshold=get_parameter("threshold").as_double();
			RCLCPP_INFO(get_logger(),"confidence=%.2f threshold=%.2f accepted=%s",
				confidence,threshold,confidence>=threshold?"true":"false");
		});
	}

private:
	rclcpp::node_interfaces::OnSetParametersCallbackHandle::SharedPtr parameter_callback_;
	rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc,char **argv)
{
	rclcpp::init(argc,argv);
	int exit_code=0;

	try
	{
		rclcpp::spin(std::make_shared<ThresholdNode>());
	}
	catch(const std::exception &error)
	{
		RCLCPP_ERROR(rclcpp::get_logger("threshold_node"),"%s",error.what());
		exit_code=1;
	}

	rclcpp::shutdown();
	return exit_code;
}
