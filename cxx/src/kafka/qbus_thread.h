// qbus_thread.h: Simple wrappers of pthread API.
#pragma once

#include <pthread.h>
#include "qbus_noncopyable.h"

namespace qbus {
namespace kafka {

class Thread : public noncopyable {
   public:
    typedef void* (*FuncPtr)(void*);

    bool start(FuncPtr func, void* arg);
    bool stop();

    // Don't detach a stopped thread!
    bool detach();

   private:
    pthread_t id_;
};

class ConditionVariable;

// All methods will report error and call exit(1) if failed.
class Mutex : public noncopyable {
    friend class ConditionVariable;

   public:
    Mutex();
    ~Mutex();

    void lock();
    void unlock();

   private:
    pthread_mutex_t handle_;
};

class MutexGuard : public noncopyable {
   public:
    MutexGuard(Mutex& mtx) : mtx_(mtx) { mtx_.lock(); }
    ~MutexGuard() { mtx_.unlock(); }

   private:
    Mutex& mtx_;
};

// All methods will report error and call exit(1) if failed.
class ConditionVariable : public noncopyable {
   public:
    ConditionVariable();
    ~ConditionVariable();

    void wait(Mutex& mtx);
    void notify_one();
    void notify_all();

   private:
    pthread_cond_t handle_;
};

}  // namespace kafka
}  // namespace qbus
