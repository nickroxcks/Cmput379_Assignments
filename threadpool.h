#ifndef THREADPOOL_H
#define THREADPOOL_H
#include <pthread.h>
#include <stdbool.h>
#include <vector>
#include <queue>
#include <deque>
#include <list>
//using namespace std;


typedef void (*thread_func_t)(void *arg);

typedef struct ThreadPool_work_t {   //Struct for a work task
    thread_func_t func;              // The function pointer
    void *arg;                       // The arguments for the function
} ThreadPool_work_t;

typedef struct {  //a struct with a work que.
    std:: deque<ThreadPool_work_t> global_que;  //all work tasks assigned to a pool
} ThreadPool_work_queue_t;

typedef struct {
    pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
    pthread_mutex_t mutex_main = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t mutex_thread = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t mutex_job = PTHREAD_MUTEX_INITIALIZER;
    int num_members;
    int num_jobs;  //keep track of threads working
    ThreadPool_work_queue_t work_queue;
    std:: vector<pthread_t> members_vector;  //vector of all threads belonging to pool
} ThreadPool_t;


/**
* A C style constructor for creating a new ThreadPool object
* Parameters:
*     num - The number of threads to create
* Return:
*     ThreadPool_t* - The pointer to the newly created ThreadPool object
*/
ThreadPool_t *ThreadPool_create(int num);

/**
* A C style destructor to destroy a ThreadPool object
* Parameters:
*     tp - The pointer to the ThreadPool object to be destroyed
*/
void ThreadPool_destroy(ThreadPool_t *tp);

/**
* Add a task to the ThreadPool's task queue
* Parameters:
*     tp   - The ThreadPool object to add the task to
*     func - The function pointer that will be called in the thread
*     arg  - The arguments for the function
* Return:
*     true  - If successful
*     false - Otherwise
*/
bool ThreadPool_add_work(ThreadPool_t *tp, thread_func_t func, void *arg);

/**
* Get a task from the given ThreadPool object
* Parameters:
*     tp - The ThreadPool object being passed
* Return:
*     ThreadPool_work_t* - The next task to run
*/
ThreadPool_work_t ThreadPool_get_work(ThreadPool_t *tp);

/**
* Run the next task from the task queue
* Parameters:
*     tp - The ThreadPool Object this thread belongs to
*/
void *Thread_run(ThreadPool_t *tp, ThreadPool_work_t work_task);
#endif
