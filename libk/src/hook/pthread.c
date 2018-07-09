#include <pthread.h>
#include <k/sys.h>

int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void*), void *arg) {
    return MAKE_SYSCALL(unimplemented, "pthread_create", true);
}

int pthread_join(pthread_t thread, void **value_ptr) {
    return MAKE_SYSCALL(unimplemented, "pthread_join", true);
}

int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr) {
    return MAKE_SYSCALL(unimplemented, "pthread_mutex_init", true);
}

int pthread_mutex_destroy(pthread_mutex_t *mutex) {
    return MAKE_SYSCALL(unimplemented, "pthread_mutex_destroy", true);
}

int pthread_mutex_lock(pthread_mutex_t *mutex) {
    return MAKE_SYSCALL(unimplemented, "pthread_mutex_lock", true);
}

int pthread_mutex_trylock(pthread_mutex_t *mutex) {
    return MAKE_SYSCALL(unimplemented, "pthread_mutex_trylock", true);
}

int pthread_mutex_unlock(pthread_mutex_t *mutex) {
    return MAKE_SYSCALL(unimplemented, "pthread_mutex_unlock", true);
}

int pthread_cond_init(pthread_cond_t *restrict cond, const pthread_condattr_t *restrict attr) {
    return MAKE_SYSCALL(unimplemented, "pthread_cond_init", true);
}

int pthread_cond_destroy(pthread_cond_t *cond) {
    return MAKE_SYSCALL(unimplemented, "pthread_cond_destroy", true);
}

int pthread_cond_signal(pthread_cond_t *cond) {
    return MAKE_SYSCALL(unimplemented, "pthread_cond_signal", true);
}

int pthread_cond_broadcast(pthread_cond_t *cond) {
    return MAKE_SYSCALL(unimplemented, "pthread_cond_broadcast", true);
}

int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex) {
    return MAKE_SYSCALL(unimplemented, "pthread_cond_wait", true);
}

int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *abstime) {
    return MAKE_SYSCALL(unimplemented, "pthread_cond_timedwait", true);
}
