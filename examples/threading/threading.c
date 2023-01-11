#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg, ...)
// #define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg, ...) printf("threading ERROR: " msg "\n", ##__VA_ARGS__)

void *threadfunc(void *thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    // struct thread_data* thread_func_args = (struct thread_data *) thread_param;

    struct thread_data *new_thrdata = (struct thread_data *)thread_param;
    new_thrdata->thread_complete_success = false;

    usleep(new_thrdata->wait_obtain*1000);

    int errStatus = pthread_mutex_lock(new_thrdata->mutex);
    if (!errStatus)
    {
        usleep(new_thrdata->wait_release*1000);
        errStatus = pthread_mutex_unlock(new_thrdata->mutex);
        if (!errStatus)
            new_thrdata->thread_complete_success=true;
    }

    return thread_param;
}

bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex, int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */

    if (!thread || !mutex)
    {
        ERROR_LOG("thread or mutext pointers are not valid.\n");
        return false;
    }

    struct thread_data *thrdata = (struct thread_data *)malloc(sizeof(struct thread_data));
    if (!thrdata)
    {
        ERROR_LOG("allocating thread_data failed\r\n");
        return false;
    }

    memset(thrdata, 0, sizeof(struct thread_data));
    thrdata->thread = thread;
    thrdata->mutex = mutex;
    thrdata->wait_obtain = wait_to_obtain_ms;
    thrdata->wait_release = wait_to_release_ms;
    thrdata->thread_complete_success = false;

    int err = pthread_create(thread, NULL, threadfunc, thrdata);
    if(!err || thrdata->thread_complete_success)
        return true;
    
    free(thrdata);
    return false;
}
