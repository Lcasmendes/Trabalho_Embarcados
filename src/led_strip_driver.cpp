#include <ros/package.h>
#include <ros/ros.h>
#include <mrs_modules_msgs/Llcp.h>

#include "led_strip_driver/LedStripDriverService.h"

#include <string>

#include <nodelet/nodelet.h>
#include <pluginlib/class_list_macros.h>

#include "../firmware/msgs.h"

namespace led_strip_driver
{

/* class LedStripDriver //{ */

class LedStripDriver : public nodelet::Nodelet {

public:
  virtual void onInit();

private:
  void callbackReceiveMessage(const mrs_modules_msgs::LlcpConstPtr &msg);
  bool callbackSetOutputs(led_strip_driver::LedStripDriverService::Request &req, led_strip_driver::LedStripDriverService::Response &res);

  ros::NodeHandle nh_;

  ros::Publisher  llcp_publisher_;
  ros::Subscriber llcp_subscriber_;

  ros::ServiceServer srv_server_set_outputs_;

  bool is_initialized_ = false;
};

//}

/* onInit() //{ */

void LedStripDriver::onInit() {

  // Get paramters
  nh_ = ros::NodeHandle("~");

  ros::Time::waitForValid();

  // Publishers
  llcp_publisher_ = nh_.advertise<mrs_modules_msgs::Llcp>("llcp_out", 1);

  llcp_subscriber_ = nh_.subscribe("llcp_in", 10, &LedStripDriver::callbackReceiveMessage, this, ros::TransportHints().tcpNoDelay());

  srv_server_set_outputs_ = nh_.advertiseService("set_outputs", &LedStripDriver::callbackSetOutputs, this);

  is_initialized_ = true;
}
//}

//}

// | ------------------------ callbacks ------------------------ |

/* callbackReceiveMessage() //{ */

void LedStripDriver::callbackReceiveMessage(const mrs_modules_msgs::LlcpConstPtr &msg) {

  if (!is_initialized_) {
    return;
  }

  // llcp is working with arrays, so we need to convert the payload from the ROS message into an array
  uint8_t payload_size = msg->payload.size();
  uint8_t payload_array[payload_size];
  std::copy(msg->payload.begin(), msg->payload.end(), payload_array);

  switch (payload_array[0]) {
    case CMD_MSG_ID: {
      ROS_INFO_STREAM("[LedStripDriver]: Received cmd message -> this should not happen");
      break;
    }
    case HEARTBEAT_MSG_ID: {
      heartbeat_msg *received_msg = (heartbeat_msg *)payload_array;
      ROS_INFO_STREAM("[LedStripDriver]: Received Heartbeat message, " << int(received_msg->cmds_received));
      break;
    }
    default: {
      ROS_ERROR_STREAM("[LedStripDriver]: Received unknown message with id " << int(payload_array[0]));
      break;
    }
  }
}

//}

/* //{ callbackSetOutputs() */

bool LedStripDriver::callbackSetOutputs(led_strip_driver::LedStripDriverService::Request &req, led_strip_driver::LedStripDriverService::Response &res) {

  if (!is_initialized_) {

    res.success = false;
    res.message = "Driver is not yet initialized!";
    ROS_WARN("[LedStripDriver]: Driver is not yet initialized!");
    return true;
  }

  cmd_msg msg_out;

  msg_out.id           = CMD_MSG_ID;
  msg_out.set_out_a    = req.output_a;
  msg_out.set_out_b    = req.output_b;
  msg_out.set_out_vbat = req.output_vbat;

  mrs_modules_msgs::Llcp llcp_msg;

  uint8_t *msg_ptr = (uint8_t *)&msg_out;

  for (int i = 0; i < sizeof(msg_out); i++) {
    llcp_msg.payload.push_back(msg_ptr[i]);
  }

  try {
    llcp_publisher_.publish(llcp_msg);
  }
  catch (...) {
    ROS_ERROR("Exception caught during publishing topic %s.", llcp_publisher_.getTopic().c_str());
    res.success = false;
    return true;
  }

  ROS_INFO_STREAM("[LedStripDriver]: Setting outputs: 12V_A: " << msg_out.set_out_a << ", 12V_B: " << msg_out.set_out_b <<  ", BAT: " << msg_out.set_out_vbat);
  res.success = true;
  res.message = "Outputs set set.";

  return true;
}

//}

// | ------------------------ routines ------------------------ |


}  // namespace led_strip_driver

PLUGINLIB_EXPORT_CLASS(led_strip_driver::LedStripDriver, nodelet::Nodelet);
