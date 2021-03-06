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
* Author: Eitan Marder-Eppstein
*********************************************************************/
#ifndef NAV_CORE_BASE_GLOBAL_PLANNER_
#define NAV_CORE_BASE_GLOBAL_PLANNER_

#include <geometry_msgs/PoseStamped.h>
#include <costmap_2d/costmap_2d_ros.h>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include "nav_core/nav_goal_manager.h"
#include "nav_core/nav_status.h"
#include <nav_core/nav_core_state.h>
#include <nav_core/base_planner.h>

namespace nav_core {
  /**
   * @class BaseGlobalPlanner
   * @brief Provides an interface for global planners used in navigation. All global planners written as plugins for the navigation stack must adhere to this interface.
   */
  class BaseGlobalPlanner : public BasePlanner
  {
    public:
      typedef boost::shared_ptr<BaseGlobalPlanner> Ptr;
      typedef boost::function <Ptr ()> FetchFunction;

      /**
       * @brief Given a goal pose in the world, compute a plan. The implementation of this method is
       *        responsible for locking the costmap mutex. An empty default implementation is given to
       *        allow for either the 3 arg or 4 arg makePlan to be implemented, without forcing a
       *        boilerplate 3 arg version to be added.
       * @param start The start pose
       * @param goal The goal pose
       * @param plan The plan... filled by the planner
       * @return True if a valid plan was found, false otherwise
       */
      virtual bool makePlan(const geometry_msgs::PoseStamped& start,
          const geometry_msgs::PoseStamped& goal, std::vector<geometry_msgs::PoseStamped>& plan)
      {
        return false;
      }

      /**
       * @brief Given a goal pose in the world, compute a plan. The implementation of this method is
       *        responsible for locking the costmap mutex.
       * @param start The start pose
       * @param goal The goal pose
       * @param plan The plan... filled by the planner
       * @param custom_status The status returned by the planner
       * @return True if a valid plan was found, false otherwise
       */
      virtual bool makePlan(const geometry_msgs::PoseStamped& start,
          const geometry_msgs::PoseStamped& goal,
          std::vector<geometry_msgs::PoseStamped>& plan,
          int& custom_status)
      {
        // Generating a default 3 argument implementation for the plugins
        // that don't provide it. The custom status will mirror the
        // return value of the status free makePlan.
        const bool return_value = makePlan(start, goal, plan);
        if (return_value)
        {
          custom_status = status::OK;
        }
        else
        {
          custom_status = status::FAIL;
        }
        return return_value;
      }


      /**
       * @brief  Initialization function for the BaseGlobalPlanner
       * @param  name The name of this planner
       * @param  costmap_ros A pointer to the ROS wrapper of the costmap to use for planning
       */
      virtual void initialize(std::string name, costmap_2d::Costmap2DROS* costmap_ros) = 0;

      /**
       * @brief Function to prepare the planner for actions post-recovery, whatever that means for each plugin.
       */
      virtual void prepareForPostRecovery(){}

      /**
       * @brief Function to reset the state of the planner (for e.g. reintialize), whatever that means for each plugin.
       */
      virtual void resetPlanner(){}

      /**
       * Set the goal manager
       * @param goal_manager goal manager
       */
      virtual void setGoalManager(NavGoalMananger::Ptr goal_manager)
      {
        goal_manager_ = goal_manager;
      }

      /**
       * Set the common state
       * @param navcore_state common state with pointers to planners and costmaps
       */
      virtual void setNavCoreState(nav_core::State::Ptr navcore_state)
      {
        navcore_state_ = navcore_state;
      }

      /**
       * @brief  Virtual destructor for the interface
       */
      virtual ~BaseGlobalPlanner(){}

      /**
       * @brief Common goal goal manager
       */
      NavGoalMananger::Ptr goal_manager_;

      /**
       * @brief Common state object with pointers to planners and costmaps
       */
      nav_core::State::Ptr navcore_state_;


    protected:
      BaseGlobalPlanner(){}
  };
};

#endif
