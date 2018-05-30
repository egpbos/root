/*****************************************************************************
 * Project: RooFit                                                           *
 * Package: RooFitCore                                                       *
 * @(#)root/roofitcore:$Id$
 * Authors:                                                                  *
 *   PB, Patrick Bos,     NL eScience Center, p.bos@esciencecenter.nl        *
 *   IP, Inti Pelupessy,  NL eScience Center, i.pelupessy@esciencecenter.nl  *
 *                                                                           *
 * Redistribution and use in source and binary forms,                        *
 * with or without modification, are permitted according to the terms        *
 * listed in LICENSE (http://roofit.sourceforge.net/license.txt)             *
 *****************************************************************************/

#include <MultiProcess/Job.h>
#include <MultiProcess/TaskManager.h>
#include <MultiProcess/messages.h>

namespace RooFit {
  namespace MultiProcess {
    Job::Job(std::size_t _N_workers) : N_workers(_N_workers) {}

    double Job::call_double_const_method(std::string /*key*/) {
      throw std::logic_error("call_double_const_method not implemented for this Job");
    }

    // This default sends back only one double as a result; can be overloaded
    // e.g. for RooAbsCategorys, for tuples, etc. The queue_loop and master
    // process must implement corresponding result receivers.
    void Job::send_back_task_result_from_worker(std::size_t task) {
      double result = get_task_result(task);
      get_manager()->send_from_worker_to_queue(id, task, result);
    }

    // static function
    void Job::worker_loop() {
      assert(TaskManager::instance()->is_worker());
      worker_loop_running = true;
      bool carry_on = true;
      Task task;
      std::size_t job_id;
      Q2W message_q2w;
      JobTask job_task;
      BidirMMapPipe& pipe = *TaskManager::instance()->get_worker_pipe();

      // use a flag to not ask twice
      bool dequeue_acknowledged = true;

      while (carry_on) {
        if (work_mode) {
          // try to dequeue a task
          if (dequeue_acknowledged) {  // don't ask twice
            pipe << W2Q::dequeue << BidirMMapPipe::flush;
            dequeue_acknowledged = false;
          }

          // receive handshake
          pipe >> message_q2w;

          switch (message_q2w) {
            case Q2W::terminate: {
              carry_on = false;
              break;
            }

            case Q2W::dequeue_rejected: {
              dequeue_acknowledged = true;
              break;
            }
            case Q2W::dequeue_accepted: {
              dequeue_acknowledged = true;
              pipe >> job_id >> task;
              TaskManager::get_job_object(job_id)->evaluate_task(task);

              pipe << W2Q::send_result;
              TaskManager::get_job_object(job_id)->send_back_task_result_from_worker(task);
              pipe >> message_q2w;
              if (message_q2w != Q2W::result_received) {
                std::cerr << "worker " << getpid() << " sent result, but did not receive Q2W::result_received handshake! Got " << message_q2w << " instead." << std::endl;
                throw std::runtime_error("");
              }
              break;
            }

            case Q2W::switch_work_mode: {
              // change to non-work-mode
              work_mode = false;
              break;
            }

            case Q2W::call_double_const_method:
            case Q2W::update_real: {
              std::cerr << "In worker_loop: " << message_q2w << " message invalid in work-mode!" << std::endl;
              break;
            }

            case Q2W::result_received: {
              std::cerr << "In worker_loop: " << message_q2w << " message received, but should only be received as handshake!" << std::endl;
              break;
            }
          }
        } else {
          // receive message
          pipe >> message_q2w;

          switch (message_q2w) {
            case Q2W::terminate: {
              carry_on = false;
              break;
            }

            case Q2W::update_real: {
              std::size_t ix;
              double val;
              bool is_constant;
              pipe >> job_id >> ix >> val >> is_constant;
              TaskManager::get_job_object(job_id)->update_real(ix, val, is_constant);
              break;
            }

            case Q2W::call_double_const_method: {
              std::string key;
              pipe >> job_id >> key;
              Job * job = TaskManager::get_job_object(job_id);
//                double (* method)() = job->get_double_const_method(key);
              double result = job->call_double_const_method(key);
              pipe << result << BidirMMapPipe::flush;
              break;
            }

            case Q2W::switch_work_mode: {
              // change to work-mode
              work_mode = true;
              break;
            }

            case Q2W::dequeue_accepted:
            case Q2W::dequeue_rejected: {
              if (!dequeue_acknowledged) {
                // when switching from work to non-work mode, often a dequeue
                // message from the worker must still be processed by the
                // queue process
                dequeue_acknowledged = true;
              } else {
                std::cerr << "In worker_loop: " << message_q2w << " message invalid in non-work-mode!" << std::endl;
              }
              break;
            }

            case Q2W::result_received: {
              std::cerr << "In worker_loop: " << message_q2w << " message received, but should only be received as handshake!" << std::endl;
              break;
            }

          }
        }
      }
    }

    std::shared_ptr<TaskManager> & Job::get_manager() {
      if (!_manager) {
        _manager = TaskManager::instance(N_workers);
      }

      // N.B.: must check for activation here, otherwise get_manager is not callable
      //       from queue loop!
      if (!_manager->is_activated()) {
        _manager->activate();
      }

      if (!worker_loop_running && _manager->is_worker()) {
        Job::worker_loop();
        std::_Exit(0);
      }

      return _manager;
    }

    // initialize static members
    bool Job::work_mode = false;
    bool Job::worker_loop_running = false;

  } // namespace MultiProcess
} // namespace RooFit