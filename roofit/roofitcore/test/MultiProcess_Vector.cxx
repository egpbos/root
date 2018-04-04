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

#include <cstdlib>  // std::_Exit
#include <cmath>
#include <vector>
#include <queue>
#include <map>
#include <exception>
#include <memory>  // make_shared
#include <numeric> // accumulate

#include <RooRealVar.h>
#include <../src/BidirMMapPipe.h>

// for NLL tests
#include <TRandom.h>
#include <RooWorkspace.h>
#include <RooAbsPdf.h>
#include <RooDataSet.h>
#include <RooNLLVar.h>

#include "gtest/gtest.h"

class xSquaredPlusBVectorSerial {
 public:
  xSquaredPlusBVectorSerial(double b, std::vector<double> x_init) :
      _b("b", "b", b),
      x(std::move(x_init)),
      result(x.size()) {}

  virtual void evaluate() {
    // call evaluate_task for each task
    for (std::size_t ix = 0; ix < x.size(); ++ix) {
      result[ix] = std::pow(x[ix], 2) + _b.getVal();
    }
  }

  std::vector<double> get_result() {
    evaluate();
    return result;
  }

 protected:
  RooRealVar _b;
  std::vector<double> x;
  std::vector<double> result;
};


namespace RooFit {
  namespace MultiProcess {

    // Messages from master to queue
    enum class M2Q : int {
      terminate = 100,
      enqueue = 10,
      retrieve = 11
    };

    // Messages from queue to master
    enum class Q2M : int {
      terminate = 200,
      retrieve_rejected = 20,
      retrieve_accepted = 21
    };

    // Messages from worker to queue
    enum class W2Q : int {
      terminate = 300,
      dequeue = 30,
      send_result = 31
    };

    // Messages from queue to worker
    enum class Q2W : int {
      terminate = 400,
      dequeue_rejected = 40,
      dequeue_accepted = 41,
      update_parameter = 42,
      switch_work_mode = 43,
      result_received = 44
    };

    // for debugging
#define PROCESS_VAL(p) case(p): s = #p; break;

    std::ostream& operator<<(std::ostream& out, const M2Q value){
      const char* s = 0;
      switch(value){
        PROCESS_VAL(M2Q::terminate);
        PROCESS_VAL(M2Q::enqueue);
        PROCESS_VAL(M2Q::retrieve);
      }
      return out << s;
    }

    std::ostream& operator<<(std::ostream& out, const Q2M value){
      const char* s = 0;
      switch(value){
        PROCESS_VAL(Q2M::terminate);
        PROCESS_VAL(Q2M::retrieve_rejected);
        PROCESS_VAL(Q2M::retrieve_accepted);
      }
      return out << s;
    }

    std::ostream& operator<<(std::ostream& out, const W2Q value){
      const char* s = 0;
      switch(value){
        PROCESS_VAL(W2Q::terminate);
        PROCESS_VAL(W2Q::dequeue);
        PROCESS_VAL(W2Q::send_result);
      }
      return out << s;
    }

    std::ostream& operator<<(std::ostream& out, const Q2W value){
      const char* s = 0;
      switch(value){
        PROCESS_VAL(Q2W::terminate);
        PROCESS_VAL(Q2W::dequeue_rejected);
        PROCESS_VAL(Q2W::dequeue_accepted);
        PROCESS_VAL(Q2W::update_parameter);
        PROCESS_VAL(Q2W::switch_work_mode);
        PROCESS_VAL(Q2W::result_received);
      }
      return out << s;
    }

#undef PROCESS_VAL

  }
}


// stream operators for message enum classes
namespace RooFit {

  BidirMMapPipe &BidirMMapPipe::operator<<(const MultiProcess::M2Q& sent) {
    *this << static_cast<int>(sent);
    return *this;
  }

  BidirMMapPipe &BidirMMapPipe::operator>>(MultiProcess::M2Q& received) {
    int receptor;
    *this >> receptor;
    received = static_cast<MultiProcess::M2Q>(receptor);
    return *this;
  }

  BidirMMapPipe &BidirMMapPipe::operator<<(const MultiProcess::Q2M& sent) {
    *this << static_cast<int>(sent);
    return *this;
  }

  BidirMMapPipe &BidirMMapPipe::operator>>(MultiProcess::Q2M& received) {
    int receptor;
    *this >> receptor;
    received = static_cast<MultiProcess::Q2M>(receptor);
    return *this;
  }

  BidirMMapPipe &BidirMMapPipe::operator<<(const MultiProcess::W2Q& sent) {
    *this << static_cast<int>(sent);
    return *this;
  }

  BidirMMapPipe &BidirMMapPipe::operator>>(MultiProcess::W2Q& received) {
    int receptor;
    *this >> receptor;
    received = static_cast<MultiProcess::W2Q>(receptor);
    return *this;
  }

  BidirMMapPipe &BidirMMapPipe::operator<<(const MultiProcess::Q2W& sent) {
    *this << static_cast<int>(sent);
    return *this;
  }

  BidirMMapPipe &BidirMMapPipe::operator>>(MultiProcess::Q2W& received) {
    int receptor;
    *this >> receptor;
    received = static_cast<MultiProcess::Q2W>(receptor);
    return *this;
  }

}


namespace RooFit {
  namespace MultiProcess {




    // -- BEGIN header for InterProcessQueueAndMessenger --

    // forward declaration
    class Job;

    // some helper types
    using Task = std::size_t;
    using JobTask = std::pair<std::size_t, Task>;  // combined job_object and task identifier type

    // InterProcessQueueAndMessenger (IPQM) handles message passing
    // and communication with a queue of tasks and workers that execute the
    // tasks. The queue is in a separate process that can communicate with the
    // master process (from where this object is created) and the queue process
    // communicates with the worker processes.
    //
    // The IPQM class does work defined by subclasses of the Job class.
    //
    // For message passing, enum class T based on int are used. The implementer
    // must make sure that T can be sent over the BidirMMapPipe, i.e. that
    // BidirMMapPipe::operator<<(T) and >>(T) are implemented. This can be done
    // locally in the implementation file of the messenger, e.g.:
    //
    //    namespace RooFit {
    //      struct T {
    //        int a, b;
    //      };
    //      BidirMMapPipe& BidirMMapPipe::operator<<(const T & sent) {
    //        *this << sent.a << sent.b;
    //        return *this;
    //      }
    //      BidirMMapPipe& BidirMMapPipe::operator>>(T & received) {
    //        *this >> received.a >> received.b;
    //        return *this;
    //      }
    //    }
    //
    // Make sure that activate() is called soon after creation of an IPQM,
    // because everything in between construction and activate() gets executed
    // on all processes (master, queue and slaves). Activate starts the queue
    // loop on the queue process, which means it can start doing its job.
    // Worker processes have to be activated separately from the Job objects
    // themselves. Activate cannot be called from inside the constructor, since
    // the loops would prevent the constructor from returning a constructed
    // object (thus defying its purpose). Note that at the end of activate, the
    // queue and child processes are killed. This is achieved by sending the
    // terminate message, which is done automatically in the destructor of this
    // class, but can also be done manually via terminate().
    //
    // When using everything as intended, i.e. by only instantiating via the
    // instance() method, activate() is called from Job::ipqm() immediately
    // after creation, so one need not worry about the above.
    class InterProcessQueueAndMessenger {
     public:
      static std::shared_ptr<InterProcessQueueAndMessenger> instance(std::size_t N_workers);
      static std::shared_ptr<InterProcessQueueAndMessenger> instance();
      explicit InterProcessQueueAndMessenger(std::size_t N_workers);
      ~InterProcessQueueAndMessenger();
      static std::size_t add_job_object(Job *job_object);
      static Job* get_job_object(std::size_t job_object_id);
      static bool remove_job_object(std::size_t job_object_id);
      void terminate();
      void terminate_workers();
      void activate();
      bool is_activated();
      BidirMMapPipe::PollVector get_poll_vector();
      bool process_queue_pipe_message(M2Q message);
      void retrieve();
      void process_worker_pipe_message(BidirMMapPipe &pipe, W2Q message);
      void queue_loop();
      bool from_queue(JobTask &job_task);
      void to_queue(JobTask job_task);
      bool is_master();
      bool is_queue();
      bool is_worker();
      std::shared_ptr<BidirMMapPipe>& get_worker_pipe();
      std::size_t get_worker_id();
      std::map<JobTask, double>& get_results();

     private:
      std::vector< std::shared_ptr<BidirMMapPipe> > worker_pipes;
      std::shared_ptr<BidirMMapPipe> queue_pipe;
      std::size_t worker_id;
      bool _is_master = false;
      bool _is_queue = false;
      std::queue<JobTask> queue;
      std::size_t N_tasks = 0;  // total number of received tasks
      // TODO: add RooAbsCategory handling to results!
      std::map<JobTask, double> results;
      bool queue_activated = false;

      static std::map<std::size_t, Job *> job_objects;
      static std::size_t job_counter;
      static std::weak_ptr<InterProcessQueueAndMessenger> _instance;
    };

    // -- END header for InterProcessQueueAndMessenger --







    /*
     * @brief interface class for defining the actual work that IPQM must do
     *
     * Think of "job" as in "employment", e.g. the job of a baker, which
     * involves *tasks* like baking and selling bread. The Job must define the
     * tasks through its execution (evaluate_task) and returning its result
     * (get_task_result), based on a task index argument.
     */
    class Job {
     public:
      explicit Job(std::size_t _N_workers) : N_workers(_N_workers) {}

      virtual void evaluate_task(std::size_t task) = 0;
      virtual double get_task_result(std::size_t task) = 0;

      static void worker_loop() {
        assert(InterProcessQueueAndMessenger::instance()->is_worker());
        worker_loop_running = true;
        bool carry_on = true;
        Task task;
        std::size_t job_object_id;
        Q2W message_q2w;
        JobTask job_task;
        BidirMMapPipe& pipe = *InterProcessQueueAndMessenger::instance()->get_worker_pipe();
        while (carry_on) {
          if (work_mode) {
            // try to dequeue a task
            pipe << W2Q::dequeue << BidirMMapPipe::flush;

            // receive handshake
            pipe >> message_q2w;

            switch (message_q2w) {
              case Q2W::terminate: {
                carry_on = false;
                break;
              }

              case Q2W::dequeue_rejected: {
                break;
              }
              case Q2W::dequeue_accepted: {
                pipe >> job_object_id >> task;
                InterProcessQueueAndMessenger::get_job_object(job_object_id)->evaluate_task(task);

                // TODO: add RooAbsCategory handling!
                double result = InterProcessQueueAndMessenger::get_job_object(job_object_id)->get_task_result(task);
                pipe << W2Q::send_result << job_object_id << task << result << BidirMMapPipe::flush;
                pipe >> message_q2w;
                if (message_q2w != Q2W::result_received) {
                  throw std::runtime_error("worker sent result, but did not receive Q2W::result_received handshake!");
                }
                break;
              }

              case Q2W::switch_work_mode: {
                // change to non-work-mode
                work_mode = false;
                break;
              }

              case Q2W::update_parameter: {
                std::cerr << "In worker_loop: update_parameter message invalid in work-mode!" << std::endl;
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
                pipe << W2Q::terminate << BidirMMapPipe::flush;
                carry_on = false;
                break;
              }

              case Q2W::update_parameter: {
                // receive new parameter value and update
                // ...
                break;
              }

              case Q2W::switch_work_mode: {
                // change to work-mode
                work_mode = true;
                break;
              }

              case Q2W::dequeue_accepted:
              case Q2W::dequeue_rejected: {
                std::cerr << "In worker_loop: " << message_q2w << " message invalid in non-work-mode!" << std::endl;
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

      std::shared_ptr<InterProcessQueueAndMessenger> & ipqm() {
        if (!_ipqm) {
          _ipqm = InterProcessQueueAndMessenger::instance(N_workers);
        }

        _ipqm->activate();

        if (!worker_loop_running && _ipqm->is_worker()) {
          Job::worker_loop();
          std::_Exit(0);
        }

        return _ipqm;
      }

     private:
      // do not use _ipqm directly, it must first be initialized! use ipqm()
      std::size_t N_workers;
      std::shared_ptr<InterProcessQueueAndMessenger> _ipqm = nullptr;

      static bool work_mode;
      static bool worker_loop_running;
    };

    // initialize static member
    bool Job::work_mode = true;
    bool Job::worker_loop_running = false;






    // -- BEGIN implementation for InterProcessQueueAndMessenger --

    // static function
    std::shared_ptr<InterProcessQueueAndMessenger> InterProcessQueueAndMessenger::instance(std::size_t N_workers) {
      std::shared_ptr<InterProcessQueueAndMessenger> tmp;
      tmp = _instance.lock();
      if (!tmp) {
        assert(N_workers != 0);
        tmp = std::make_shared<InterProcessQueueAndMessenger>(N_workers);
        // assign to weak_ptr _instance
        _instance = tmp;
      } else {
        // identify yourselves (for debugging)
//        if (tmp->is_worker()) {
//          std::cout << "I'm a worker, PID " << getpid() << std::endl;
//        } else if (tmp->is_master()) {
//          std::cout << "I'm master, PID " << getpid() << std::endl;
//        } else if (tmp->is_queue()) {
//          std::cout << "I'm queue, PID " << getpid() << std::endl;
//        }

        // some sanity checks
        if(tmp->is_master() && N_workers != tmp->worker_pipes.size()) {
          std::cout << "On PID " << getpid() << ": N_workers != tmp->worker_pipes.size())! N_workers = " << N_workers << ", tmp->worker_pipes.size() = " << tmp->worker_pipes.size() << std::endl;
          throw std::logic_error("");
        } else if (tmp->is_worker()) {
          if (tmp->get_worker_id() + 1 != tmp->worker_pipes.size()) {
            std::cout << "On PID " << getpid() << ": tmp->get_worker_id() + 1 != tmp->worker_pipes.size())! tmp->get_worker_id() = " << tmp->get_worker_id() << ", tmp->worker_pipes.size() = " << tmp->worker_pipes.size() << std::endl;
            throw std::logic_error("");
          }
//          if (1 != std::accumulate(tmp->worker_pipes.begin(), tmp->worker_pipes.end(), 0,
//                                   [](int a, std::shared_ptr<BidirMMapPipe>& b) {
//                                     return a + (b->closed() ? 1 : 0);
//                                   })) {
//            std::cout << "On PID " << getpid() << ": worker has multiple open worker pipes, should only be one!" << std::endl;
//            throw std::logic_error("");
//          }

        }
      }
      return tmp;
    }

    // static function
    std::shared_ptr<InterProcessQueueAndMessenger> InterProcessQueueAndMessenger::instance() {
      if (!_instance.lock()) {
        throw std::runtime_error("in InterProcessQueueAndMessenger::instance(): no instance was created yet! Call InterProcessQueueAndMessenger::instance(std::size_t N_workers) first.");
      }
      return _instance.lock();
    }

    // constructor
    // Don't construct IPQM objects manually, use the static instance if
    // you need to run multiple jobs.
    InterProcessQueueAndMessenger::InterProcessQueueAndMessenger(std::size_t N_workers) {
      // This class defines three types of processes:
      // 1. master: the initial main process. It defines and enqueues tasks
      //    and processes results.
      // 2. workers: a pool of processes that will try to take tasks from the
      //    queue. These are first forked from master.
      // 3. queue: communication between the other types (necessarily) goes
      //    through this process. This process runs the queue_loop and
      //    maintains the queue of tasks. It is forked last and initialized
      //    with third BidirMMapPipe parameter false, which makes it the
      //    process that manages all pipes, though the pool of pages remains
      //    on the master process.
      // The reason for using this layout is that we use BidirMMapPipe for
      // forking and communicating between processes, and BidirMMapPipe only
      // supports forking from one process, not from an already forked
      // process (if forked using BidirMMapPipe). The latter layout would
      // allow us to fork first the queue from the main process and then fork
      // the workers from the queue, which may feel more natural.
      //
      // The master/queue processes pipe is created in initialization. Here
      // we manually create workers from the queue process.


      // BidirMMapPipe parameters
      bool useExceptions = true;
      bool useSocketpair = false;
      bool keepLocal_WORKER = true;
      bool keepLocal_QUEUE = false;

      // First initialize the workers.
      // Reserve is necessary! BidirMMapPipe is not allowed to be copied,
      // but when capacity is not enough when using emplace_back, the
      // vector must be resized, which means existing elements must be
      // copied to the new memory locations.

      worker_pipes.reserve(N_workers);
      for (std::size_t ix = 0; ix < N_workers; ++ix) {
        // set worker_id before each fork so that fork will sync it to the worker
        worker_id = ix;
        worker_pipes.push_back(std::make_shared<BidirMMapPipe>(useExceptions, useSocketpair, keepLocal_WORKER));
        if(worker_pipes.back()->isChild()) break;
      }

      // then do the queue and master initialization, but each worker should
      // exit the constructor from here on
      if (worker_pipes.back()->isParent()) {
        queue_pipe = std::make_shared<BidirMMapPipe>(useExceptions, useSocketpair, keepLocal_QUEUE);

        if (queue_pipe->isParent()) {
          _is_master = true;
        } else if (queue_pipe->isChild()) {
          _is_queue = true;
        } else {
          // should never get here...
          throw std::runtime_error("Something went wrong while creating InterProcessQueueAndMessenger!");
        }
      }
    }


    InterProcessQueueAndMessenger::~InterProcessQueueAndMessenger() {
      terminate();
    }


    // static function
    // returns job_id for added job_object
    std::size_t InterProcessQueueAndMessenger::add_job_object(Job *job_object) {
      std::size_t job_id = job_counter++;
      job_objects[job_id] = job_object;
      return job_id;
    }

    // static function
    Job* InterProcessQueueAndMessenger::get_job_object(std::size_t job_object_id) {
      return job_objects[job_object_id];
    }

    // static function
    bool InterProcessQueueAndMessenger::remove_job_object(std::size_t job_object_id) {
      return job_objects.erase(job_object_id) == 1;
    }


    void InterProcessQueueAndMessenger::terminate() {
      if (_is_master && queue_pipe->good()) {
        *queue_pipe << M2Q::terminate << BidirMMapPipe::flush;
        int retval = queue_pipe->close();
        if (0 != retval) {
          std::cerr << "error terminating queue_pipe" << "; child return value is " << retval << std::endl;
        }
      }
    }

    void InterProcessQueueAndMessenger::terminate_workers() {
      if (_is_queue) {
        for (std::shared_ptr<BidirMMapPipe> &worker_pipe : worker_pipes) {
          *worker_pipe << Q2W::terminate << BidirMMapPipe::flush;
        }
      }
    }


    // start message loops on child processes and quit processes afterwards
    void InterProcessQueueAndMessenger::activate() {
      // should be called soon after creation of this object, because everything in
      // between construction and activate gets executed both on the master process
      // and on the slaves
      queue_activated = true; // set on all processes, master, queue and slaves

      if (_is_queue) {
        queue_loop();
        terminate_workers();
        std::_Exit(0);
      }
    }


    bool InterProcessQueueAndMessenger::is_activated() {
      return queue_activated;
    }


    BidirMMapPipe::PollVector InterProcessQueueAndMessenger::get_poll_vector() {
      BidirMMapPipe::PollVector poll_vector;
      poll_vector.reserve(1 + worker_pipes.size());
      poll_vector.emplace_back(queue_pipe.get(), BidirMMapPipe::Readable);
      for (std::shared_ptr<BidirMMapPipe>& pipe : worker_pipes) {
        poll_vector.emplace_back(pipe.get(), BidirMMapPipe::Readable);
      }
      return poll_vector;
    }


    bool InterProcessQueueAndMessenger::process_queue_pipe_message(M2Q message) {
      bool carry_on = true;

      switch (message) {
        case M2Q::terminate: {
          carry_on = false;
        }
        break;

        case M2Q::enqueue: {
          // enqueue task
          Task task;
          std::size_t job_object_id;
          *queue_pipe >> job_object_id >> task;
          JobTask job_task(job_object_id, task);
          to_queue(job_task);
          N_tasks++;
        }
        break;

        case M2Q::retrieve: {
          // retrieve task results after queue is empty and all
          // tasks have been completed
          if (queue.empty() && results.size() == N_tasks) {
            *queue_pipe << Q2M::retrieve_accepted;  // handshake message (master will now start reading from the pipe)
            *queue_pipe << N_tasks;
            for (auto const &item : results) {
              *queue_pipe << item.first.first << item.first.second << item.second;
            }
            // empty results cache
            results.clear();
            // reset number of received tasks
            N_tasks = 0;
            *queue_pipe << BidirMMapPipe::flush;
          } else {
            *queue_pipe << Q2M::retrieve_rejected << BidirMMapPipe::flush;  // handshake message: tasks not done yet, try again
          }
        }
        break;
      }

      return carry_on;
    }


    void InterProcessQueueAndMessenger::retrieve() {
      if (_is_master) {
//          std::cout << "retrieving..." << std::endl;
        bool carry_on = true;
        while (carry_on) {
//          std::cout << "retrieve sent message " << M2Q::retrieve << std::endl;
          *queue_pipe << M2Q::retrieve << BidirMMapPipe::flush;
          Q2M handshake;
          *queue_pipe >> handshake;
//          std::cout << "retrieve got handshake: " << handshake << std::endl;
          if (handshake == Q2M::retrieve_accepted) {
            carry_on = false;
            *queue_pipe >> N_tasks;
            for (std::size_t ix = 0; ix < N_tasks; ++ix) {
              Task task;
              std::size_t job_object_id;
              double result;
              *queue_pipe >> job_object_id >> task >> result;
              JobTask job_task(job_object_id, task);
              results[job_task] = result;
            }
          }
        }
      }
//        std::cout << "retrieved." << std::endl;
    }


    void InterProcessQueueAndMessenger::process_worker_pipe_message(BidirMMapPipe &pipe, W2Q message) {
      Task task;
      std::size_t job_object_id;
      switch (message) {
        case W2Q::dequeue: {
          // dequeue task
          JobTask job_task;
          if (from_queue(job_task)) {
            pipe << Q2W::dequeue_accepted << job_task.first << job_task.second;
          } else {
            pipe << Q2W::dequeue_rejected;
          }
          pipe << BidirMMapPipe::flush;
          break;
        }

        case W2Q::send_result: {
          // receive back task result
          // TODO: add RooAbsCategory handling!
          double result;
          pipe >> job_object_id >> task >> result;
          pipe << Q2W::result_received << BidirMMapPipe::flush;
          JobTask job_task(job_object_id, task);
          results[job_task] = result;
          break;
        }

        case W2Q::terminate: {
          throw std::runtime_error("In queue_loop: received terminate signal from worker, but this signal should only be sent as handshake after queue sends terminate signal first!");
        }
      }
    }


    void InterProcessQueueAndMessenger::queue_loop() {
      if (_is_queue) {
        bool carry_on = true;
        auto poll_vector = get_poll_vector();

        while (carry_on) {
          // poll: wait until status change (-1: infinite timeout)
          int n_changed_pipes = BidirMMapPipe::poll(poll_vector, -1);
          // then process messages from changed pipes; loop while there are
          // still messages remaining after processing one (so that all
          // messages since changed pipe will get read).
          // TODO: Should we do this outside of the for loop, to make sure that both the master and the worker pipes are read from successively, instead of possibly getting stuck on one pipe only?
          // scan for pipes with changed status:
          for (auto it = poll_vector.begin(); n_changed_pipes > 0 && poll_vector.end() != it; ++it) {
            if (!it->revents) {
              // unchanged, next one
              continue;
            }
//              --n_changed_pipes; // maybe we can stop early...
            // read from pipes which are readable
            do { // while there are bytes to read
              if (it->revents & BidirMMapPipe::Readable) {
                // message comes from the master/queue pipe (first element):
                if (it == poll_vector.begin()) {
                  M2Q message;
                  *queue_pipe >> message;
                  carry_on = process_queue_pipe_message(message);
                  // on terminate, also stop for-loop, no need to check other
                  // pipes anymore:
                  if (!carry_on) {
                    n_changed_pipes = 0;
                  }
                } else { // from a worker pipe
                  W2Q message;
                  BidirMMapPipe &pipe = *(it->pipe);
                  pipe >> message;
                  process_worker_pipe_message(pipe, message);
                }
              }
            } while (it->pipe->bytesReadableNonBlocking() > 0);
          }
        }
      }
    }


    // Have a worker ask for a task-message from the queue
    bool InterProcessQueueAndMessenger::from_queue(JobTask &job_task) {
      if (queue.empty()) {
        return false;
      } else {
        job_task = queue.front();
        queue.pop();
        return true;
      }
    }


    // Enqueue a task
    void InterProcessQueueAndMessenger::to_queue(JobTask job_task) {
      if (is_master()) {
        if (!queue_activated) {
          activate();
        }
        *queue_pipe << M2Q::enqueue << job_task.first << job_task.second << BidirMMapPipe::flush;

      } else if (is_queue()) {
        queue.push(job_task);
      } else {
        throw std::logic_error("calling Communicator::to_master_queue from slave process");
      }
    }


    bool InterProcessQueueAndMessenger::is_master() {
      return _is_master;
    }

    bool InterProcessQueueAndMessenger::is_queue() {
      return _is_queue;
    }

    bool InterProcessQueueAndMessenger::is_worker() {
      return !(_is_master || _is_queue);
    }


    std::shared_ptr<BidirMMapPipe>& InterProcessQueueAndMessenger::get_worker_pipe() {
      assert(is_worker());
      return worker_pipes[worker_id];
    }

    std::size_t InterProcessQueueAndMessenger::get_worker_id() {
      return worker_id;
    }


    std::map<JobTask, double>& InterProcessQueueAndMessenger::get_results() {
      return results;
    }

    // initialize static members
    std::map<std::size_t, Job *> InterProcessQueueAndMessenger::job_objects;
    std::size_t InterProcessQueueAndMessenger::job_counter = 0;
    std::weak_ptr<InterProcessQueueAndMessenger> InterProcessQueueAndMessenger::_instance {};

    // -- END implementation for InterProcessQueueAndMessenger --







    // Vector defines an interface and communication machinery to build a
    // parallelized subclass of an existing non-concurrent numerical class that
    // can be expressed as a vector of independent sub-calculations.
    //
    // TODO: update documentation for new InterProcessQueueAndMessenger!
    //
    // A subclass can communicate between master and worker processes using
    // messages that the subclass defines for itself. The interface uses int
    // type for messages. Two messages are predefined:
    // * -1 means terminate the worker_loop
    // *  0 means take a new task from the queue (essentially: no message)
    // * any other value is deferred to the subclass-defined process_message
    //   method.
    //
    template <typename Base>
    class Vector : public Base, public Job {
     public:
      template<typename... Targs>
      Vector(std::size_t _N_workers, Targs ...args) :
          Base(args...),
          Job(_N_workers)
      {
        job_id = InterProcessQueueAndMessenger::add_job_object(this);
      }

      ~Vector() {
        InterProcessQueueAndMessenger::remove_job_object(job_id);
      }

//      void initialize_parallel_work_system() {
//        if (!_ipqm) {
//          _ipqm = InterProcessQueueAndMessenger::instance(_NumCPU);
//        }
//
//        if (ipqm()->is_worker()) {
//          worker_loop();
//          std::_Exit(0);
//        }
//      }


     protected:
      void gather_worker_results() {
        if (!retrieved) {
          ipqm()->retrieve();
          for (auto const &item : ipqm()->get_results()) {
            if (item.first.first == job_id) {
              ipqm_results[item.first.second] = item.second;
            }
          }
        }
      }

      // -- members --
     protected:
      std::size_t job_id;
      std::map<Task, double> ipqm_results;
      bool retrieved = false;
    };
  }
}


using RooFit::MultiProcess::JobTask;

class xSquaredPlusBVectorParallel : public RooFit::MultiProcess::Vector<xSquaredPlusBVectorSerial> {
 public:
  xSquaredPlusBVectorParallel(std::size_t NumCPU, double b_init, std::vector<double> x_init) :
      RooFit::MultiProcess::Vector<xSquaredPlusBVectorSerial>(NumCPU, b_init,
           x_init) // NumCPU stands for everything that defines the parallelization behaviour (number of cpu, strategy, affinity etc)
  {}


  void evaluate() override {
    if (ipqm()->is_master()) {
      // master fills queue with tasks
      retrieved = false;
      for (std::size_t ix = 0; ix < x.size(); ++ix) {
        JobTask job_task(job_id, ix);
        ipqm()->to_queue(job_task);
      }

      // wait for task results back from workers to master
      gather_worker_results();
      // put task results in desired container
      for (std::size_t ix = 0; ix < x.size(); ++ix) {
        result[ix] = ipqm_results[ix];
      }
    }
  }


 private:
  void evaluate_task(std::size_t task) override {
    assert(ipqm()->is_worker());
    result[task] = std::pow(x[task], 2) + _b.getVal();
  }

  double get_task_result(std::size_t task) override {
    assert(ipqm()->is_worker());
    return result[task];
  }

};

class MultiProcessVectorSingleJob : public ::testing::TestWithParam<std::size_t> {
  // You can implement all the usual fixture class members here.
  // To access the test parameter, call GetParam() from class
  // TestWithParam<T>.
};


TEST_P(MultiProcessVectorSingleJob, getResult) {
  // Simple test case: calculate x^2 + b, where x is a vector. This case does
  // both a simple calculation (squaring the input vector x) and represents
  // handling of state updates in b.
  std::vector<double> x{0, 1, 2, 3};
  double b_initial = 3.;

  // start serial test

  xSquaredPlusBVectorSerial x_sq_plus_b(b_initial, x);

  auto y = x_sq_plus_b.get_result();
  std::vector<double> y_expected{3, 4, 7, 12};

  EXPECT_EQ(y[0], y_expected[0]);
  EXPECT_EQ(y[1], y_expected[1]);
  EXPECT_EQ(y[2], y_expected[2]);
  EXPECT_EQ(y[3], y_expected[3]);

  std::size_t NumCPU = GetParam();

  // start parallel test

  xSquaredPlusBVectorParallel x_sq_plus_b_parallel(NumCPU, b_initial, x);

  auto y_parallel = x_sq_plus_b_parallel.get_result();

  EXPECT_EQ(y_parallel[0], y_expected[0]);
  EXPECT_EQ(y_parallel[1], y_expected[1]);
  EXPECT_EQ(y_parallel[2], y_expected[2]);
  EXPECT_EQ(y_parallel[3], y_expected[3]);
}


INSTANTIATE_TEST_CASE_P(NumberOfWorkerProcesses,
                        MultiProcessVectorSingleJob,
                        ::testing::Values(1,2,3));


class MultiProcessVectorMultiJob : public ::testing::TestWithParam<std::size_t> {};

TEST_P(MultiProcessVectorMultiJob, getResult) {
  // Simple test case: calculate x^2 + b, where x is a vector. This case does
  // both a simple calculation (squaring the input vector x) and represents
  // handling of state updates in b.
  std::vector<double> x{0, 1, 2, 3};
  double b_initial = 3.;

  std::vector<double> y_expected{3, 4, 7, 12};

  std::size_t NumCPU = GetParam();

  // define jobs
  xSquaredPlusBVectorParallel x_sq_plus_b_parallel(NumCPU, b_initial, x);
  xSquaredPlusBVectorParallel x_sq_plus_b_parallel2(NumCPU, b_initial + 1, x);

  // do stuff
  auto y_parallel = x_sq_plus_b_parallel.get_result();
  auto y_parallel2 = x_sq_plus_b_parallel2.get_result();

  EXPECT_EQ(y_parallel[0], y_expected[0]);
  EXPECT_EQ(y_parallel[1], y_expected[1]);
  EXPECT_EQ(y_parallel[2], y_expected[2]);
  EXPECT_EQ(y_parallel[3], y_expected[3]);

  EXPECT_EQ(y_parallel2[0], y_expected[0] + 1);
  EXPECT_EQ(y_parallel2[1], y_expected[1] + 1);
  EXPECT_EQ(y_parallel2[2], y_expected[2] + 1);
  EXPECT_EQ(y_parallel2[3], y_expected[3] + 1);
}


INSTANTIATE_TEST_CASE_P(NumberOfWorkerProcesses,
                        MultiProcessVectorMultiJob,
                        ::testing::Values(2,1,3));


enum class RooNLLVarTask {
  all_events,
  single_event,
  bulk_partition,
  interleave
};


class MPRooNLLVar : public RooFit::MultiProcess::Vector<RooNLLVar> {
 public:
  // use copy constructor for the RooNLLVar part
  MPRooNLLVar(std::size_t NumCPU, RooNLLVarTask task_mode, const RooNLLVar& nll) :
      RooFit::MultiProcess::Vector<RooNLLVar>(NumCPU, nll),
      mp_task_mode(task_mode)
  {
    switch (mp_task_mode) {
      case RooNLLVarTask::all_events: {
        N_tasks = 1;
        break;
      }
      case RooNLLVarTask::single_event: {
        N_tasks = static_cast<std::size_t>(_data->numEntries());
        break;
      }
      case RooNLLVarTask::bulk_partition:
      case RooNLLVarTask::interleave: {
        N_tasks = NumCPU;
        break;
      }
    }
  }


  // the const is inherited from RooAbsTestStatistic::evaluate. We are not
  // actually const though, so we use a horrible hack.
  Double_t evaluate() const override {
    return const_cast<MPRooNLLVar*>(this)->evaluate_non_const();
  }

  Double_t evaluate_non_const() {
    if (ipqm()->is_master()) {
      // master fills queue with tasks
      retrieved = false;
      for (std::size_t ix = 0; ix < N_tasks; ++ix) {
        JobTask job_task(job_id, ix);
        ipqm()->to_queue(job_task);
      }

      // wait for task results back from workers to master
      gather_worker_results();
      // sum task results
      for (std::size_t ix = 0; ix < N_tasks; ++ix) {
        result += ipqm_results[ix];
      }
    }
    return result;
  }


 private:
  void evaluate_task(std::size_t task) override {
    assert(ipqm()->is_worker());
    std::size_t N_events = static_cast<std::size_t>(_data->numEntries());
    // "default" values (all events in one task)
    std::size_t first = task;
    std::size_t last  = N_events;
    std::size_t step  = 1;
    switch (mp_task_mode) {
      case RooNLLVarTask::all_events: {
        // default values apply
        break;
      }
      case RooNLLVarTask::single_event: {
        last = task + 1;
        break;
      }
      case RooNLLVarTask::bulk_partition: {
        first = N_events * task / N_tasks;
        last  = N_events * (task + 1) / N_tasks;
        break;
      }
      case RooNLLVarTask::interleave: {
        step = N_tasks;
        break;
      }
    }

    result = evaluatePartition(first, last, step);
  }

  double get_task_result(std::size_t /*task*/) override {
    assert(ipqm()->is_worker());
    return result;
  }

  double result = 0;
  std::size_t N_tasks = 0;
  RooNLLVarTask mp_task_mode;
};


TEST(MultiProcessVectorNLL, DISABLED_getResultAllEvents) {
  // Real-life test: calculate a NLL using event-based parallelization. This
  // should replicate RooRealMPFE results.
  gRandom->SetSeed(1);
  RooWorkspace w;
  w.factory("Gaussian::g(x[-5,5],mu[0,-3,3],sigma[1])");
  auto x = w.var("x");
  RooAbsPdf *pdf = w.pdf("g");
  RooDataSet *data = pdf->generate(RooArgSet(*x), 10000);
  auto nll = pdf->createNLL(*data);

  std::size_t NumCPU = 1;
  RooNLLVarTask mp_task_mode = RooNLLVarTask::all_events;

  auto nominal_result = nll->getVal();

  MPRooNLLVar nll_mp(NumCPU, mp_task_mode, *dynamic_cast<RooNLLVar*>(nll));

  auto mp_result = nll_mp.getVal();

  EXPECT_EQ(nominal_result, mp_result);
}


TEST(MultiProcessVectorNLL, DISABLED_loop) {
  // do a test with a loop where the NLL is calculated each iteration with
  // possibly different parameters

  // TODO: implement and see whether it performs adequately
}
