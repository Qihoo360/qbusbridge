#include "qbus_thread.h"
#include <stdlib.h>
#include <string.h>
#include "util/logger.h"

namespace qbus {
namespace kafka {

bool Thread::start(FuncPtr func, void* arg) {
    int err = pthread_create(&id_, NULL, func, arg);
    if (err != 0) {
        ERROR("pthread_create: " << strerror(err));
        return false;
    }
    return true;
}

bool Thread::stop() {
    int err = pthread_join(id_, NULL);
    if (err != 0) {
        ERROR("pthread_join: " << strerror(err));
        return false;
    }
    return true;
}

bool Thread::detach() {
    int err = pthread_detach(id_);
    if (err != 0) {
        ERROR("pthread_detach: " << strerror(err));
        return false;
    }
    return true;
}

Mutex::Mutex() {
    int err = pthread_mutex_init(&handle_, NULL);
    if (err != 0) {
        ERROR("pthread_mutex_init: " << strerror(err));
        abort();
    }
}

Mutex::~Mutex() {
    int err = pthread_mutex_destroy(&handle_);
    if (err != 0) {
        ERROR("pthread_mutex_destroy: " << strerror(err));
        abort();
    }
}

void Mutex::lock() {
    int err = pthread_mutex_lock(&handle_);
    if (err != 0) {
        ERROR("pthread_mutex_lock: " << strerror(err));
        abort();
    }
}

void Mutex::unlock() {
    int err = pthread_mutex_unlock(&handle_);
    if (err != 0) {
        ERROR("pthread_mutex_unlock: " << strerror(err));
        abort();
    }
}

ConditionVariable::ConditionVariable() {
    int err = pthread_cond_init(&handle_, NULL);
    if (err != 0) {
        ERROR("pthread_cond_init: " << strerror(err));
        abort();
    }
}

ConditionVariable::~ConditionVariable() {
    int err = pthread_cond_destroy(&handle_);
    if (err != 0) {
        ERROR("pthread_cond_destroy: " << strerror(err));
        abort();
    }
}

void ConditionVariable::wait(Mutex& mtx) {
    int err = pthread_cond_wait(&handle_, &mtx.handle_);
    if (err != 0) {
        ERROR("pthread_cond_wait: " << strerror(err));
        abort();
    }
}

void ConditionVariable::notify_one() {
    int err = pthread_cond_signal(&handle_);
    if (err != 0) {
        ERROR("pthread_cond_signal: " << strerror(err));
        abort();
    }
}

void ConditionVariable::notify_all() {
    int err = pthread_cond_broadcast(&handle_);
    if (err != 0) {
        ERROR("pthread_cond_broadcast: " << strerror(err));
        abort();
    }
}

}  // namespace kafka
}  // namespace qbus
