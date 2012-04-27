/*********************************************************************
 *
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2008, Willow Garage, Inc.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Willow Garage, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 * Author: TKruse
 *********************************************************************/

#include <base_local_planner/obstacle_cost_function.h>
#include <cmath>
#include <Eigen/Core>

namespace base_local_planner {

ObstacleCostFunction::ObstacleCostFunction(const costmap_2d::Costmap2DROS* costmap_ros) : costmap_ros_(costmap_ros) {
  if (costmap_ros != NULL) {
    footprint_spec_ = costmap_ros_->getRobotFootprint();
    costmap_ros_->getCostmapCopy(costmap_);
    world_model_ = new base_local_planner::CostmapModel(costmap_);
  }
}

ObstacleCostFunction::~ObstacleCostFunction() {
  if (world_model_ != NULL) {
    delete world_model_;
  }
}


void ObstacleCostFunction::setParams(double max_trans_vel, double max_scaling_factor, double scaling_speed) {
  // TODO: move this to prepare if possible
   max_trans_vel_ = max_trans_vel;
   max_scaling_factor_ = max_scaling_factor;
   scaling_speed_ = scaling_speed;
 }

bool ObstacleCostFunction::prepare() {
  //make sure to get an updated copy of the costmap before computing trajectories
  costmap_ros_->getCostmapCopy(costmap_);
  // updated footprint?
  footprint_spec_ = costmap_ros_->getRobotFootprint();
  return true;

}

double ObstacleCostFunction::scoreTrajectory(Trajectory &traj) {
  double cost = 0;
  double scale = getScalingFactor(traj, scaling_speed_, max_trans_vel_, max_scaling_factor_);
  double px, py, pth;

  for (unsigned int i = 0; i < traj.getPointsSize(); ++i) {
    traj.getPoint(i, px, py, pth);
    cost = footprintCost(px, py, pth,
        scale,
        footprint_spec_,
        costmap_,
        world_model_);
  }
  return cost;
}

double ObstacleCostFunction::getScalingFactor(Trajectory &traj, double scaling_speed, double max_trans_vel, double max_scaling_factor) {
  double vmag = sqrt(traj.xv_ * traj.xv_ + traj.yv_ * traj.yv_);

  //if we're over a certain speed threshold, we'll scale the robot's
  //footprint to make it either slow down or stay further from walls
  double scale = 1.0;
  if (vmag > scaling_speed) {
    //scale up to the max scaling factor linearly... this could be changed later
    double ratio = (vmag - scaling_speed) / (max_trans_vel - scaling_speed);
    scale = max_scaling_factor * ratio + 1.0;
  }
  return scale;
}

double ObstacleCostFunction::footprintCost (
    const double& x,
    const double& y,
    const double& th,
    double scale,
    std::vector<geometry_msgs::Point>& footprint_spec,
    costmap_2d::Costmap2D& costmap,
    base_local_planner::WorldModel* world_model) {
  double cos_th = cos(th);
  double sin_th = sin(th);

  double occ_cost = 0.0;

  std::vector<geometry_msgs::Point> scaled_oriented_footprint;
  for(unsigned int i  = 0; i < footprint_spec.size(); ++i) {
    geometry_msgs::Point new_pt;
    new_pt.x = x + (scale * footprint_spec[i].x * cos_th - scale * footprint_spec[i].y * sin_th);
    new_pt.y = y + (scale * footprint_spec[i].x * sin_th + scale * footprint_spec[i].y * cos_th);
    scaled_oriented_footprint.push_back(new_pt);
    geometry_msgs::Point robot_position;
    robot_position.x = x;
    robot_position.y = y;

    //check if the footprint is legal
    double footprint_cost = world_model->footprintCost(robot_position, scaled_oriented_footprint, costmap.getInscribedRadius(), costmap.getCircumscribedRadius());

    if (footprint_cost < 0) {
      return -6.0;
    }
    unsigned int cell_x, cell_y;

    //we won't allow trajectories that go off the map... shouldn't happen that often anyways
    if ( ! costmap.worldToMap(x, y, cell_x, cell_y)) {
      return -7.0;
    }

    occ_cost = std::max(std::max(occ_cost, footprint_cost), double(costmap.getCost(cell_x, cell_y)));

  }

  return occ_cost;
}

} /* namespace base_local_planner */