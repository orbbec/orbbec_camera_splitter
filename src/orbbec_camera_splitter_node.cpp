// SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
// Copyright (c) 2022-2023 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0


#include "orbbec_camera_splitter/orbbec_camera_splitter_node.hpp"

// #include <nvblox_ros_common/qos.hpp>
#include "orbbec_camera_splitter/json.hpp"
#include <isaac_ros_common/qos.hpp>
namespace nvblox {


    OrbbecCameraSplitterNode::
    OrbbecCameraSplitterNode(const rclcpp::NodeOptions &options)
            : Node("orbbec_camera_splitter_node", options) {
        RCLCPP_INFO(get_logger(), "Creating a "
                                  "OrbbecCameraSplitterNode().");

        // Parameters
        const std::string kDefaultQoS = "SYSTEM_DEFAULT";
        constexpr size_t kInputQueueSize = 10;
        constexpr size_t kOutputQueueSize = 10;
        const rclcpp::QoS input_qos = isaac_ros::common::AddQosParameter(*this, kDefaultQoS, "input_qos")
            .keep_last(kInputQueueSize);
        const rclcpp::QoS output_qos =
            isaac_ros::common::AddQosParameter(*this, kDefaultQoS, "output_qos")
            .keep_last(kOutputQueueSize);
        const rmw_qos_profile_t input_qos_profile = input_qos.get_rmw_qos_profile();
        infra_1_sub_.subscribe(this, "input/infra_1", input_qos_profile);
        infra_1_metadata_sub_.subscribe(this, "input/infra_1_metadata", input_qos_profile);
        infra_2_sub_.subscribe(this, "input/infra_2", input_qos_profile);
        infra_2_metadata_sub_.subscribe(this, "input/infra_2_metadata", input_qos_profile);
        depth_sub_.subscribe(this, "input/depth", input_qos_profile);
        depth_metadata_sub_.subscribe(this, "input/depth_metadata", input_qos_profile);
        pointcloud_sub_.subscribe(this, "input/pointcloud", input_qos_profile);
        pointcloud_metadata_sub_.subscribe(this, "input/pointcloud_metadata", input_qos_profile);

        // constexpr int kInputQueueSize = 10;
        timesync_infra_1_.reset(
                new message_filters::Synchronizer<image_time_policy_t>(
                        image_time_policy_t(kInputQueueSize), infra_1_sub_,
                        infra_1_metadata_sub_));
        timesync_infra_1_->registerCallback(
                std::bind(
                        &OrbbecCameraSplitterNode::image1Callback, this,
                        std::placeholders::_1, std::placeholders::_2));
        timesync_infra_2_.reset(
                new message_filters::Synchronizer<image_time_policy_t>(
                        image_time_policy_t(kInputQueueSize), infra_2_sub_,
                        infra_2_metadata_sub_));
        timesync_infra_2_->registerCallback(
                std::bind(
                        &OrbbecCameraSplitterNode::image2Callback, this,
                        std::placeholders::_1, std::placeholders::_2));
        timesync_depth_.reset(
                new message_filters::Synchronizer<image_time_policy_t>(
                        image_time_policy_t(kInputQueueSize), depth_sub_, depth_metadata_sub_));
        timesync_depth_->registerCallback(
                std::bind(
                        &OrbbecCameraSplitterNode::depthCallback, this,
                        std::placeholders::_1, std::placeholders::_2));
        timesync_pointcloud_.reset(
                new message_filters::Synchronizer<pointcloud_time_policy_t>(
                        pointcloud_time_policy_t(kInputQueueSize), pointcloud_sub_,
                        pointcloud_metadata_sub_));
        timesync_pointcloud_->registerCallback(
                std::bind(
                        &OrbbecCameraSplitterNode::pointcloudCallback, this,
                        std::placeholders::_1, std::placeholders::_2));

        // Publisher
        // constexpr size_t kOutputQueueSize = 10;
        // const auto output_qos = rclcpp::QoS(
        //         rclcpp::KeepLast(kOutputQueueSize),
        //         parseQosString(output_qos_str));
        infra_1_pub_ =
                create_publisher<sensor_msgs::msg::Image>("~/output/infra_1", output_qos);
        infra_2_pub_ =
                create_publisher<sensor_msgs::msg::Image>("~/output/infra_2", output_qos);
        depth_pub_ =
                create_publisher<sensor_msgs::msg::Image>("~/output/depth", output_qos);
        pointcloud_pub_ = create_publisher<sensor_msgs::msg::PointCloud2>(
                "~/output/pointcloud", output_qos);
    }

    int
    OrbbecCameraSplitterNode::getEmitterModeFromMetadataMsg(
            const orbbec_camera_msgs::msg::Metadata::ConstSharedPtr &metadata) {
            // Field name in json metadata
        auto json_data = nlohmann::json::parse(metadata->json_data);
        return int(json_data["frame_emitter_mode"]);
    }

    template<typename MessageType>
    void
    OrbbecCameraSplitterNode::republishIfEmitterMode(
            const typename MessageType::ConstSharedPtr &image,
            const orbbec_camera_msgs::msg::Metadata::ConstSharedPtr &metadata,
            const EmitterMode emitter_mode,
            typename rclcpp::Publisher<MessageType>::SharedPtr &publisher) {
        if (getEmitterModeFromMetadataMsg(metadata) ==
            static_cast<int>(emitter_mode)) {
            publisher->publish(*image);
        }
    }

    void
    OrbbecCameraSplitterNode::image1Callback(
            sensor_msgs::msg::Image::ConstSharedPtr image,
            orbbec_camera_msgs::msg::Metadata::ConstSharedPtr metadata) {
        republishIfEmitterMode<sensor_msgs::msg::Image>(
                image, metadata, EmitterMode::kOff, infra_1_pub_);
    }

    void
    OrbbecCameraSplitterNode::image2Callback(
            sensor_msgs::msg::Image::ConstSharedPtr image,
            orbbec_camera_msgs::msg::Metadata::ConstSharedPtr metadata) {
        republishIfEmitterMode<sensor_msgs::msg::Image>(
                image, metadata, EmitterMode::kOff, infra_2_pub_);
    }

    void
    OrbbecCameraSplitterNode::depthCallback(
            sensor_msgs::msg::Image::ConstSharedPtr image,
            orbbec_camera_msgs::msg::Metadata::ConstSharedPtr metadata) {
        republishIfEmitterMode<sensor_msgs::msg::Image>(
                image, metadata,
                EmitterMode::kOn, depth_pub_);
    }

    void
    OrbbecCameraSplitterNode::pointcloudCallback(
            sensor_msgs::msg::PointCloud2::ConstSharedPtr pointcloud,
            orbbec_camera_msgs::msg::Metadata::ConstSharedPtr metadata) {
        republishIfEmitterMode<sensor_msgs::msg::PointCloud2>(
                pointcloud, metadata, EmitterMode::kOn, pointcloud_pub_);
    }

}  // namespace nvblox

// Register the node as a component
#include "rclcpp_components/register_node_macro.hpp"

RCLCPP_COMPONENTS_REGISTER_NODE(nvblox::OrbbecCameraSplitterNode)