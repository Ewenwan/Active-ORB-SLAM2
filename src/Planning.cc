#include "Planning.h"

#include <cmath>

namespace ORB_SLAM2 {

Planning::Planning(cv::Mat goal_pose, Map* pMap){
    mpMap = pMap;
    std::cout << "OMPL version: " << OMPL_VERSION << std::endl;
    plannerType p_type = PLANNER_RRTSTAR;
    planningObjective o_type = OBJECTIVE_PATHLENGTH;

    q_start = {0, 0, 3.14/4};
	q_goal = {5.584, -2.0431, -1.5707};
}

void Planning::Run() {
    while (1) {
        // Run planner when Tracking thread send request.
        if (CheckHasRequest()) {
            // Call Planner with currKF and currPose.

            // Set planned trajectory.
        }
    }
}

// This function is called from Tracking thread (System).
void Planning::SendPlanningRequest(cv::Mat pose, KeyFrame* kf) {
    unique_lock<mutex> lock(mMutexRequest);
    currPose = pose;
    currKF = kf;
    hasRequest = true;
}

void Planning::RequestFinish() {
    unique_lock<mutex> lock(mMutexFinish);
    mbFinishRequested = true;
}

float GetTranslationMatrixDistance(const cv::Mat& pose1,
                                   const cv::Mat& pose2) {
    return pow(pose1.at<float>(0, 3) - pose2.at<float>(0, 3), 2) +
           pow(pose1.at<float>(1, 3) - pose2.at<float>(1, 3), 2) +
           pow(pose1.at<float>(2, 3) - pose2.at<float>(2, 3), 2);
}

// Given a Tsc, find the set of possibly visible points from the closest key
// frame.
std::set<MapPoint*> Planning::GetVisiblePoints(cv::Mat pose) {
    unique_lock<mutex> lock(mpMap->mMutexMapUpdate);
    std::vector<KeyFrame*> key_frames = mpMap->GetAllKeyFrames();
    // Find the closest key frame.
    double min_dist =
            GetTranslationMatrixDistance(pose, key_frames.front()->GetPose());
    KeyFrame* min_kf = key_frames.front();
    for (auto* kf : key_frames) {
        float curr_dist = GetTranslationMatrixDistance(pose, kf->GetPose());
        if (min_dist > curr_dist) {
            min_dist = curr_dist;
            min_kf = kf;
        }
    }
    // Find all visible points from the key frame.
    std::set<MapPoint*> visible_mps = min_kf->GetMapPoints();
    for (auto* kf : min_kf->GetConnectedKeyFrames()) {
        std::set<MapPoint*> connected_visible_mps = kf->GetMapPoints();
        visible_mps.insert(connected_visible_mps.begin(),
                           connected_visible_mps.end());
    }
    cout << "#visible points=" << visible_mps.size() << endl;
    return visible_mps;
}

bool Planning::CheckFinish() {
    unique_lock<mutex> lock(mMutexFinish);
    return mbFinishRequested;
}

void Planning::SetFinish() {
    unique_lock<mutex> lock(mMutexFinish);
    mbFinished = true;
    unique_lock<mutex> lock2(mMutexStop);
    mbStopped = true;
}

bool Planning::isFinished() {
    unique_lock<mutex> lock(mMutexFinish);
    return mbFinished;
}

bool Planning::CheckHasRequest() {
    unique_lock<mutex> lock(mMutexRequest);
    return hasRequest;
}

int Planning::GetNumNewKeyFrames() {
    unique_lock<mutex> lock(mMutexKeyFrameQueue);
    return mKeyFrameQueue.size();
}

// This function is called from Tracking thread.
void Planning::InsertKeyFrame(KeyFrame* pKF) {
    unique_lock<mutex> lock(mMutexKeyFrameQueue);
    if (pKF->mnId > 0) {
        mKeyFrameQueue.push_back(pKF);
    }
}

KeyFrame* Planning::PopKeyFrameQueue(int num_pop) {
    // Pops the first num_pop items in the queue and
    // returns the last element popped.
    unique_lock<mutex> lock(mMutexKeyFrameQueue);
    KeyFrame* poppedKF;
    for (int i = 0; i < num_pop; i++) {
        poppedKF = mKeyFrameQueue.front();
        mKeyFrameQueue.pop_front();
    }
    return poppedKF;
}

}  // namespace ORB_SLAM
