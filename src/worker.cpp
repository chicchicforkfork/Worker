#include "worker.h"
#include <chrono>
#include <condition_variable>
#include <deque>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdint.h>
#include <thread>
#include <unistd.h>
#include <vector>

using namespace std;

Worker::Worker(WorkerManager *worker_manager, int worker_id) {
  _worker_manager = worker_manager;
  _worker_id = worker_id;
  _completed_jobs = 0;
}

shared_ptr<void> Worker::ctx() { //
  return _ctx;
}

void Worker::ctx(shared_ptr<void> ctx) { //
  _ctx = ctx;
}

const WorkerManager *Worker::worker_manager() { //
  return _worker_manager;
}

bool Worker::finished() { //
  return (_running == false);
}

void Worker::terminate() { //
  _running = false;
}

std::size_t Worker::worker_id() { //
  return _worker_id;
}

std::size_t Worker::completed_jobs() { //
  return _completed_jobs;
}

std::size_t Worker::inc_completed_jobs() { //
  return ++_completed_jobs;
}