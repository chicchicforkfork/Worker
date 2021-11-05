#include "worker.h"
#include <iostream>
#include <pthread.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

using namespace std;
using namespace chkchk;

WorkerManager::WorkerManager(const string &name, int worker_num,
                             int wait_for_ms) {
  int i = 0;
  _name = name;
  _worker_num = worker_num;
  _wait_for_ms = chrono::milliseconds(wait_for_ms);

  auto logfilepath = "/tmp/" + name + ".log";
  auto file_logger = spdlog::basic_logger_mt(name, logfilepath);
  spdlog::set_default_logger(file_logger);
  spdlog::set_level(spdlog::level::trace);

  /// initialize job workers
  /// for multi worker
  for (; i < worker_num; i++) {
    _workers.push_back(make_shared<Worker>(this, i));
    _job_Q.push_back(deque<shared_ptr<Job>>());
    _job_M.push_back(make_shared<mutex>());
    _job_CV.push_back(make_shared<condition_variable>());
    _called_workers.push_back(map<string, uint64_t>());
  }
}

WorkerManager::~WorkerManager() { //
  terminate();
}

void WorkerManager::terminate() {
  shared_ptr<Worker> W;
  shared_ptr<mutex> M;
  shared_ptr<condition_variable> CV;
  deque<shared_ptr<Job>> Q;

  _running = false;

  if (!_joinable) {
    return;
  }

  // 종료를 위해 wait를 깨운다.
  for (size_t i = 0; i < _worker_num; i++) {
    W = _workers[i];
    M = _job_M[i];
    CV = _job_CV[i];
    Q = _job_Q[i];

    while (true) {
      {
        lock_guard<mutex> lock(*M);
        CV->notify_all();
      }
      if (W->finished()) {
        break;
      }
      this_thread::sleep_for(_wait_for_ms);
    }
  }
}

void WorkerManager::init_handler(worker_init_fn_t worker_init_handler) {
  _worker_init_handler = worker_init_handler;
}

void WorkerManager::select_worker_handler(
    select_worker_fn_t select_worker_handler) {
  _select_worker_handler = select_worker_handler;
}

string WorkerManager::worker_name() { return _name; }

string WorkerManager::report() {
  string result = "[WorkerManager::report]\n";
  size_t total_job = 0;

  for (auto worker : _workers) {
    total_job += worker->completed_jobs();
    result += "worker#" + to_string(worker->worker_id()) + ": " +
              to_string(worker->completed_jobs()) + "\n";
  }
  result += "total job: " + to_string(total_job) + "\n";
  return result;
}

void WorkerManager::monitoring() {
  spdlog::trace("[WorkerManager]-----------------------");
  std::lock_guard<std::recursive_mutex> guard(_called_workers_lock);
  for (int i = 0; i < (int)_called_workers.size(); i++) {
    auto &w = _called_workers[i];
    spdlog::trace("<thread #{} (job: {})>", i, w.size());
    for (auto &mon : w) {
      spdlog::trace("\t{}: called {}", mon.first.c_str(), mon.second);
    }
  }
}

void WorkerManager::add_job(void *data, job_handler_t handler) {
  add_job("", data, handler, false);
}

void WorkerManager::add_job(const string &name, void *data,
                            job_handler_t handler) {
  add_job(name, data, handler, true);
}

void WorkerManager::add_job(const string &name, void *data,
                            job_handler_t handler, bool affinity) {
  size_t job_id;
  size_t worker_id;
  shared_ptr<condition_variable> CV;
  shared_ptr<mutex> M;
  deque<shared_ptr<Job>> *Q;

  /// job_id 구하기 (MUST thread-safe)
  job_id = _job_seq.fetch_add(1);
  // printf("job_id: %ld\n", job_id);

  /// select worker
  /// TODO: job 분뱁 RR(Round Robin)
  if (affinity) {
    if (_select_worker_handler) {
      worker_id = _select_worker_handler(name) % _worker_num;
    } else {
      auto hashcode = hash<string>{}(name);
      worker_id = hashcode % _worker_num;
    }
    {
      std::lock_guard<std::recursive_mutex> guard(_called_workers_lock);
      auto &w = _called_workers[worker_id];
      auto mon = w.find(name);
      if (mon != w.end()) {
        mon->second++;
      } else {
        w[name] = 1;
      }
    }
  } else {
    worker_id = job_id % _worker_num;
  }

  Q = &_job_Q[worker_id];
  M = _job_M[worker_id];
  CV = _job_CV[worker_id];

  {
    lock_guard<mutex> lock(*M);
    Q->push_back(make_shared<Job>(job_id, data, handler, affinity));
    /// worker에게 작업이 추가되었음을 알림
    CV->notify_all();
  }
}

void WorkerManager::run(bool block) {
  auto f = [&](shared_ptr<Worker> worker, //
               deque<shared_ptr<Job>> *Q, //
               shared_ptr<mutex> M,       //
               shared_ptr<condition_variable> CV) {
    shared_ptr<Job> job;
    job_handler_t handler;

    pthread_setname_np(pthread_self(), _name.c_str());

    /// worker initialize
    if (_worker_init_handler) {
      _worker_init_handler(worker);
    }

    while (true) {
      unique_lock<mutex> lock(*M);
      CV->wait(lock,   //
               [&]() { //
                 return (!Q->empty() || !_running);
               });

      if (!_running && Q->empty()) {
        break;
      }

      handler = nullptr;
      if (!Q->empty()) {
        job = Q->front();
        job->worker_id(worker->worker_id());
        handler = job->handler();
        Q->pop_front();

        /// increment completed job count
        worker->inc_completed_jobs();
      }
      if (handler != nullptr)
        handler(worker, job); // call job user defined function
    }

    /// worker thread 종료 되었음을 알림
    worker->terminate();
    pthread_exit(NULL);
    return;
  };

  _run(f, block);
}

void WorkerManager::_run(thread_handler_t f, bool block) {
  /// create job worker thread
  for (auto worker : _workers) {
    // thread create and run
    auto th = make_shared<thread>(thread(f,                            //
                                         worker,                       //
                                         &_job_Q[worker->worker_id()], //
                                         _job_M[worker->worker_id()],  //
                                         _job_CV[worker->worker_id()]));

    _worker_threads.push_back(th);
  }

  // wait for terminated thread
  for (auto th : _worker_threads) {
    if (block) {
      /// block thread
      if ((*th).joinable()) {
        (*th).join();
      }
    } else {
      /// non block thread
      (*th).detach();
    }
  }
}