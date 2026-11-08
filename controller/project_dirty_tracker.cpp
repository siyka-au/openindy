#include "project_dirty_tracker.h"

#include "oi_job.h"

ProjectDirtyTracker::~ProjectDirtyTracker() { this->detach(); }

void ProjectDirtyTracker::attach(oi::OiJob* newJob) {
  this->detach();
  this->job = newJob;
  if (this->job != nullptr) {
    this->job->addObserver(this);
  }
  this->markClean();
}

void ProjectDirtyTracker::detach() {
  if (this->job != nullptr) {
    this->job->removeObserver(this);
    this->job = nullptr;
  }
}

void ProjectDirtyTracker::onJobDestroyed(oi::OiJob* destroyed) {
  if (destroyed == this->job) {
    this->job = nullptr;
  }
}
