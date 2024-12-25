#include <ros/ros.h>
#include <geometry_msgs/Twist.h>
#include <nav_msgs/Odometry.h>
#include <std_srvs/Trigger.h>
#include <opencv2/opencv.hpp>
#define CVUI_IMPLEMENTATION
#include "robot_gui/cvui.h"

// Include RobotInfo message
#include <robotinfo_msgs/RobotInfo10Fields.h>

// Global Variables
geometry_msgs::Twist cmd_vel_msg;
nav_msgs::Odometry current_odom;
std::string data_fields[6] = {"Waiting for data...", "", "", "", "", ""};
std::string distance_message = "0";

// Callback for Robot Info (AGV Status)
void robotInfoCallback(const robotinfo_msgs::RobotInfo10Fields::ConstPtr& msg) {
    data_fields[0] = msg->data_field_01;
    data_fields[1] = msg->data_field_02;
    data_fields[2] = msg->data_field_03;
    data_fields[3] = msg->data_field_04;
    data_fields[4] = msg->data_field_05;
    data_fields[5] = msg->data_field_06;
}

void odomCallback(const nav_msgs::Odometry::ConstPtr& msg) {
    current_odom = *msg;
}

int main(int argc, char **argv) {
    ros::init(argc, argv, "robot_gui");
    ros::NodeHandle nh;

    // Publishers and Subscribers
    ros::Publisher cmd_vel_pub = nh.advertise<geometry_msgs::Twist>("/cmd_vel", 10);
    ros::Subscriber robot_info_sub = nh.subscribe("/robot_info", 10, robotInfoCallback);
    ros::Subscriber odom_sub = nh.subscribe("/odom", 10, odomCallback);
    ros::ServiceClient distance_client = nh.serviceClient<std_srvs::Trigger>("/get_distance");

    // Initialize GUI
    cv::Mat frame = cv::Mat(800, 400, CV_8UC3);
    cvui::init("Robot Teleoperation");

    while (ros::ok()) {
        frame = cv::Scalar(49, 52, 49);  // Background color

        // Info Section
        cvui::text(frame, 30, 20, "Info");
        cvui::rect(frame, 30, 40, 340, 130, 0x333333);  // Increased height for 6 lines

        // Display 6 Fields from RobotInfo10Fields
        int y_offset = 60;
        for (int i = 0; i < 6; i++) {
            cvui::printf(frame, 40, y_offset + (i * 20), 0.4, 0xffffff, "%s", data_fields[i].c_str());
        }

        // Teleoperation Section
        int button_x = 150;
        if (cvui::button(frame, button_x, 190, "Forward")) {
            cmd_vel_msg.linear.x += 0.1;
        }
        if (cvui::button(frame, button_x - 70, 250, "Left")) {
            cmd_vel_msg.angular.z += 0.1;
        }
        if (cvui::button(frame, button_x, 250, "Stop")) {
            cmd_vel_msg.linear.x = 0.0;
            cmd_vel_msg.angular.z = 0.0;
        }
        if (cvui::button(frame, button_x + 70, 250, "Right")) {
            cmd_vel_msg.angular.z -= 0.1;
        }
        if (cvui::button(frame, button_x, 310, "Backward")) {
            cmd_vel_msg.linear.x -= 0.1;
        }

        // Publish cmd_vel message continuously
        cmd_vel_pub.publish(cmd_vel_msg);

        // Velocity Section
        cvui::rect(frame, 30, 350, 340, 60, 0x333333);
        cvui::text(frame, 40, 360, "Linear velocity:");
        cvui::text(frame, 200, 360, "Angular velocity:");
        cvui::printf(frame, 40, 380, 0.5, 0xff0000, "%.2f m/sec", cmd_vel_msg.linear.x);
        cvui::printf(frame, 200, 380, 0.5, 0xff0000, "%.2f rad/sec", cmd_vel_msg.angular.z);

        // Odometry Section
        cvui::text(frame, 30, 420, "Estimated robot position based off odometry");
        cvui::rect(frame, 30, 440, 340, 60, 0x333333);
        cvui::text(frame, 50, 460, "X");
        cvui::text(frame, 150, 460, "Y");
        cvui::text(frame, 250, 460, "Z");
        cvui::printf(frame, 50, 480, 0.6, 0xffffff, "%.2f", current_odom.pose.pose.position.x);
        cvui::printf(frame, 150, 480, 0.6, 0xffffff, "%.2f", current_odom.pose.pose.position.y);
        cvui::printf(frame, 250, 480, 0.6, 0xffffff, "%.2f", current_odom.pose.pose.position.z);

        // Distance Section
        cvui::text(frame, 30, 520, "Distance Travelled");
        cvui::rect(frame, 30, 540, 340, 80, 0x333333);
        if (cvui::button(frame, 40, 570, "Call")) {
            std_srvs::Trigger srv;
            if (distance_client.call(srv)) {
                distance_message = srv.response.message;
                ROS_INFO("Distance: %s", srv.response.message.c_str());
            } else {
                distance_message = "Failed to get distance!";
            }
        }
        cvui::printf(frame, 200, 570, 0.7, 0xffffff, "%s", distance_message.c_str());

        // Render GUI
        cvui::update();
        cv::imshow("Robot Teleoperation", frame);
        cv::waitKey(20);

        ros::spinOnce();
    }

    return 0;
}
