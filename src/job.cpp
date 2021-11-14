

#include "worker.h"

using namespace std;

Job::Job(size_t job_id, void *job_data, job_handler_t &handler, bool affinity) {
  _job_id = job_id;
  _handler = handler;
  _job_data = job_data;
  _affinity = affinity;
}

bool Job::affinity() { return _affinity; }

job_handler_t Job::handler() { return _handler; }

void Job::worker_id(std::size_t worker_id) { _worker_id = worker_id; }

void Job::job_id(std::size_t job_id) { _job_id = job_id; }

size_t Job::worker_id() const { return _worker_id; }

size_t Job::job_id() const { return _job_id; }

void *Job::job_data() const { return _job_data; }