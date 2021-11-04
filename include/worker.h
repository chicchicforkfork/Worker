#ifndef __WORKER_H__
#define __WORKER_H__

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <stdint.h>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>

namespace chkchk {

class WorkerManager;
class Worker;
class Job;

/**
 * @brief job(작업) 처리 함수 타입
 */
using job_handler_t =
    std::function<void(std::shared_ptr<Worker>, std::shared_ptr<Job>)>;

using worker_init_t = std::function<void(std::shared_ptr<Worker>)>;

using thread_handler_t = std::function<void(
    std::shared_ptr<Worker>, std::deque<std::shared_ptr<Job>> *,
    std::shared_ptr<std::mutex>, std::shared_ptr<std::condition_variable>)>;
/**
 * @brief job(작업)를 처리하기 worker(작업)를 관리하기 위한 객체
 */
class Worker {
private:
  /// worker ID
  std::size_t _worker_id;
  /// 처리 완료된 작업 수
  std::size_t _completed_jobs;
  /// 작업 관리자
  WorkerManager *_worker_manager;
  /// worker(작업자) thread 종료되었는지 여부
  bool _running = true;
  /// User data
  std::shared_ptr<void> _ctx;

public:
  /**
   * @brief Worker 생성자
   * @param worker_id worker ID
   */
  Worker(WorkerManager *worker_manager, int worker_id);

  std::shared_ptr<void> ctx();

  void ctx(std::shared_ptr<void> ctx);

  /**
   * @brief 작업 관리자 반환WorkerManager
   * @return const WorkerManager*
   */
  const WorkerManager *worker_manager();

  /**
   * @brief worker thread 종료 여부
   * @return true
   * @return false
   */
  bool finished();

  /**
   * @brief worker(작업자) thread를 종료하라고 알림
   * @return true
   * @return false
   */
  void terminate();

  /**
   * @brief worker ID를 반환
   * @return std::size_t
   */
  std::size_t worker_id();

  /**
   * @brief worker가 완료 처리한 job(작업) 수를 반환
   * @return std::size_t
   */
  std::size_t completed_jobs();

  /**
   * @brief worker가 완료 처리한 job(작업) 수 1 증가
   */
  std::size_t inc_completed_jobs();
};

/**
 * @brief 작업 객체
 */
class Job {
private:
  /// 작업 처리 함수
  job_handler_t _handler;
  /// job(작업)을 할당 받은 작업자(worker) ID
  std::size_t _worker_id = 0;
  /// job(작업) ID
  std::size_t _job_id = 0;
  /// 작업 처리를 특정 worker에게 할당 여부
  bool _affinity;
  void *_job_data;

public:
  /**
   * @brief Job 생성자
   * @param name 작업 이름
   * @param handler 작업 처리 함수
   * @param affinity 작업 처리 특정 worker에게 할당 할지 여부
   */
  Job(size_t job_id, void *job_data, job_handler_t &handler,
      bool affinity = false);

  /**
   * @brief 작업처리 affinity 여부
   * @return true
   * @return false
   */
  bool affinity();

  /**
   * @brief 작업 처리 함수 반환
   * @return job_handler_t
   */
  job_handler_t handler();

  /**
   * @brief worker(작업자) ID 할당
   * @param worker_id
   */
  void worker_id(std::size_t worker_id);

  /**
   * @brief job(작업) ID 할당
   * @param job_id
   */
  void job_id(std::size_t job_id);

  /**
   * @brief 할당된 worker 쓰레드 ID
   * @return std::size_t
   */
  std::size_t worker_id() const;

  /**
   * @brief worker(작업자) 쓰레드별 할당 받은 job 순서 번호
   * @return std::size_t
   */
  std::size_t job_id() const;

  void *job_data() const;
};

/**
 * @brief worker(작업자) 스레드가 job(작업)을 처리하는 라이브러리.
 * 하나의 job이 Queue(큐)에 추가되면 여러개의 worker 중 하나의 worker가 선택되어
 * job을 처리한다. 여기서 어떠한 worker가 선택될지는 모른다.
 */
class WorkerManager {
protected:
  /// 작업 관리 이름
  std::string _name;
  /// worker(작업자)에게 할당을 요청하는 선택 대기 Queue(큐)
  std::vector<std::deque<std::shared_ptr<Job>>> _job_Q;
  /// worker(작업자) 선택 대기 Queue 동시 접근을 위한 lock
  std::vector<std::shared_ptr<std::mutex>> _job_M;
  /// worker(작업자) 선택 대기 Queue 동시 접근 변수
  std::vector<std::shared_ptr<std::condition_variable>> _job_CV;
  /// 작업자(worker) 목록
  std::vector<std::shared_ptr<Worker>> _workers;
  /// 작업자(worker) 스레드 목록
  std::vector<std::shared_ptr<std::thread>> _worker_threads;
  /// worker(작업자)가 job(작업) 요청을 대기하는 시간으로 해당 시간 안에 작업
  /// 요청이 없으면 persist handle(지속적으로 처리해야 작업)이 호출됨
  std::chrono::milliseconds _wait_for_ms;
  /// 모든 worker(작업자) thread 실행 여부
  bool _running = true;
  /// worker(작업자) 수
  std::size_t _worker_num;
  std::atomic<size_t> _job_seq;
  ///
  size_t _job_count = 0;
  ///
  worker_init_t _worker_init_handler = nullptr;
  ///
  bool _joinable = true;

private:
  std::recursive_mutex _called_workers_lock;
  std::vector<std::map<std::string, uint64_t>> _called_workers;

protected:
  void _run(thread_handler_t f, bool block = false);

public:
  /**
   * @brief WorkerManager 생성자
   * @param worker_num worker(작업자) 수
   * @param wait_for_ms job(작업) 용처 대기 시간 (millisecond)
   */
  WorkerManager(const std::string &name, int worker_num, int wait_for_ms = 10);

  /**
   * @brief WorkerManager 소멸자
   */
  ~WorkerManager();

  /**
   * @brief worker thread initialize
   * @param handler
   */
  void initialize(worker_init_t handler);

  /**
   * @brief 모든 worker(작업자) thread 종료할 것을 알림
   */
  void terminate();

  /**
   * @brief worker thread가 모두 종료 될 때 까지 대기
   */
  void join();

  std::string worker_name();

  std::string report();

  /**
   * @brief 여러 worker 중 하나의 worker에게 작업 요청
   * @param name
   * @param handler
   * @param affinity
   */
  void add_job(const std::string &name, void *data, job_handler_t handler,
               bool affinity);
  void add_job(void *data, job_handler_t handler);
  void add_job(const std::string &name, void *data, job_handler_t handler);
  /**
   * @brief WorkerManager 수행
   * @param block YES인 경우 run 함수 block되어 뒤에 실행 코드가 수행 안됨\n
   * FALSE인 경우 run 함수 호출 후 바로 리턴됨
   */
  void run(bool block = false);

  void monitoring();
};

} // namespace chkchk
#endif