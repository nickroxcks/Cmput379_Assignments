Nicholas Serrano 1508361
MapReduce library

Please read over all the function descriptions to ensure how to use the libraries to their
highest potential.

STRUCTS:

ThreadPool_t:
Every threadpool object made with ThreadPool_create will have a associated ThreadPool_t struct. The 
struct contains contains mutexes and condition variables to allow communication and synchronization
between different threads. Some other members include a vector storing all subthreads in the threadpool,
and a work que struct that links to another struct containing the que of tasks to be done for the pool.
Lastly of note, there is a num_jobs variable. The purpose of this variable is to allow the main threadpool
to determine when to destroy a threadpool when ThreadPool_destroy(ThreadPool_t *tp) is called. In the event
destory is called and threads are still working on a task, we want to wait for the sub threads to finish its
tasks first, otherwise the thread will be destroyed in the middle of a task, which could cause unexpected 
results. Num_jobs is incremented right before the main thread pushes a task to the work que, and decremented
when a sub thread has finished its assigned work task. This design choice was also made because it is faster,
and prevents the need for the main thread to check every thread to see if they are working on a task. 

ThreadPool_work_queue_t:
A struct that simply has a deque stored inside it. The deque has ThreadPool_work_t elements, which are the 
work tasks that sub threads pop from. 

ThreadPool_work_t:
Struct that represent a work task for a thread to run. It has the function in which we want the thread to run,
as well as the arguements to be passed to that function.

FUNCTIONS:

ThreadPool_create(num_threads):

When called, this function make a threadpool with the specified number of N threads taken as an
arguement. It simply loops N times and calls pthread_create to create the thread. Each creation
also adds the sub thread to the threadpools members vector. It should also be noted that each thread
created will initially run thread_function(void *ptr). 

thread_function(void *ptr):

When called, each thread will initially aquire the shared main mutex lock defined in the threadpool. After
which, the thread enters a infinite loop to allow the thread to continue to run tasks. In this infinite 
loop, there is loop that iterates over the condition that the threadpool's work que is empty. The idea
here is that we want the sub thread to wait for a ask to be pushed to the que, and then work on that task.
However, we need to provide a means of synchronization between all subthreads. This is why inside the inner loop,
the sub thread will call pthread_cond_wait. Remeber that at this point before calling wait, the sub thread has 
obtained the main mutex lock before hand. Once wait is called, the lock is released, and will allow other 
subthreads to be created. When a task is pushed to the que by the main process, a condition signal is sent from
the main process, to allow a thread to wake up and work on the pushed task. The inner while loop ensures that 
no matter what happens, a thread will only start working when the que is not empty. When a sub thrad, wakes up,
it aquires the main mutex lock, ensuring only one sub thread in the pool will work on the task. Before working,
the thread calls the ThreadPool_get_work function(remeber at this point the sub thread stil has the lock) and will
grab the task from the work que. The locking prevents multiple threads deleting or inserting from the work que at
the same time. After obtaining a task, the thread releases its lock, allowing others to obtain a task, and the calls
Thread_run() to execute the task. Upon completion, the thread will return to the top of the infinite loop and repeat this 
process.

ThreadPool_get_work(&pool):

When called, allows the threads to obtain the first task inside the work que(note that in MR_RUN, an initial sort occurs
to ensure that the tasks grabbed follow the LJF design). 

Thread_run(ThreadPool_t *tp,ThreadPool_work_t work_task):

When calls, allows the calling thread to execute the given task. Important note is that the sub thread upon completion
of task will decrease the num_jobs variable in the threads respective thread pool by 1. This allows main thread to keep track
of threads working.

void ThreadPool_destroy(ThreadPool_t *tp):

Upon calling, the main thread will first check wheter any threads in the threadpool are still working. As mentioned before, this is done using 
num_jobs. For this, the main thread stays in a while loop constantly checking for num_jobs to reach 0. 

ThreadPool_add_work(ThreadPool_t *tp, thread_func_t func, void *arg):

This will usually be called by the main thread. It adds work to thethreadpools work que to allow other subthreads to work on that
task. Important things of note are that before pushing a task to que, the main thread obtains the main mutex lock to ensure that no 
other threads are trying to pop from the que at the same time. This library ensures that the user can implement however task assign aproach
they wish. So if a user wishes to follow a LJF approach for the threadpool, the main thread simply sort all the tasks, and then pushes them
to the work que in that respective order. Another note is that before pushing a task, the main thread will add 1 to the num_jobs variable for 
the pool.

MR_RUN:

Note: A global pool ojects is initially created globally for ease of use throughout this function.

Creates a pool, and then sorts a list of tasks to ensure the threadpool follows the LJF algorithm. Doing one intial sort before pushing
allows the sorting to only occur once, saving run time, and ensuring the proper order of pushing to the work que. The main process 
then initially assigns the num_jobs for the pool to 0, and begins pushea all the tasks to the que one by one. After all the pushing,
the main process immediately calls the ThreadPool_destroy function, to wait for all threads to finish their tasks, and destroy the 
threadpool. It then proceeds to the reduce phase, and will create a vector used to store all of the reduce vectors. It will then create 
R threads(the number of reducer threads specified by user) and then wait for the reducer threads to finish their tasks. The main thread
does this waiting by calling pthread_join for each created reducer thread to ensure that the reducer threads are all finished their tasks.
Note that each reducer thread created calls thread_function_reduce, where the sub thread will eventually terminate and allow the join 
to occur. 

DATA STRUCTURE:

In order to mimic the partition data structure as mentioned in the assignment, I have chosen a nested hash map to store all the keys 
and values being read. The maps are defined as the following:

map<unsigned long, multimap<string,string> > inter_data

The outer map, stores each partition indexed by a defined partition number(unsigned long), where each partition is its own multimap.
Multimap was chosen as the parition data structure as it allow insertion of multiple keys of the same name. That is, a key name does 
not need to be unique. Since the Map_Reduce library requires insertion of multiple keys of the same name, this design was chosen. Another 
benefit to this design is upon insertion of a key and its value. The multimap will automatically insert the key in such a way that
the overal map maintains ascending key order, which allows the reduce phase to easily partion through data. Insertion runs at O(logn) time.

thread_function_reduce():

This function is called by the reducer threads upon creation (reminder that thread pool was not used, so global variables were used instead).
This is a middle ground function which just passes a number(the assigned partition number for that reducer thread) to MR_ProcessPartition.

MR Emit:

The Mr Emit function assumes that it is being called inside a user defined map function. 

This function takes in a key and value to be placed into the data structure as mentioned above. It begins by deciding which partion 
to assign the key/value pair to. This is done using a hash function called computePartitionNumber. This function is the exact same 
as MR_Partition, but is more freindly for c++ and will take in string inputs, which match the parameters of the multimap data 
structure. Since a user specifies R partitions(R reducer thread), the  hashing must decide between at most R partitions. 
This hash function worst case will run at O(R) time, constant to the users defined R. 

Upon recieving a partion number, Mr_Emit will then decide what to do with the key/vaue pair. If the partion number assigned to the pair
has not yet been created inside the Data Structure, a new partion with that partion number is added to the data structure, with the 
key/value pair inserted into that partion( O(logR) ).

If the partion number exists already in the database, the key/value pair is then inserted to that already existing partion. Note that 
with each insertion into the database, a lock is obtained by the callingthread to ensure that there is not concurrent writing to the 
database(preventing unexpected results). This lock is specific to the sub threads inside the threadpool, as the main process 
does not interact with the database during the map phase. Since this is variable to the n number of entries in that partion, this insertion 
runs at O(logn) time. Thus, a call to MR_Emit will take at most O(logn + R) time. 

MR_ProcessPartition:

This function is called by a reducer thread. Since each reducer thread handles one partition(ith reducer handles ith partition),
we must iterate through the database to process each key. Hence, a loop in this function iterates through the entire partition.
Each key found in an iteration, we call the user defined Reduce function. Note that since no key is unique in a multimap,
the loop ensures that each key is only proccessed once.

MR GetNext:

This function is assumed to be called within a user defined Reduce function.This function will return values for a given
key in a particular partion number. In this function you will notice a loop, but worry not it is only there because multimap built
in functions return iterators. This loop will only run once. Inside this loop, we find the first occurence of the key/value pair
to return to the users reduce function. Before returning however, we also delete this entry from the database, allowing us
to proccess a different key of the same key name. Looking up the value and retrieving it costs log(n) time, where n is the size
of the partion. Deletion in multi map by using an iterator for an element(your not using ranged deletion) is log(n) time. This gives 
us a total of 2log(n). In the event the key value pair is not found, the loop will have not executed at all, and insted the function
would return NULL to the user defined reduce function.

Note that locking occurs upon deletion to prevent concurrent deleting in the database and prevent unexpected results.

Other notes:

-Assignment description asks to terminate a reducer thread upon task completion. However there is no need for this in my implementation as 
the reducer thread will automatically terminate once it reacehes the end of its reducing(no thread pool). Thus, this still allows for 
resources to be opened up.
-In the future, you should provide students with c++ starter code as well, and allow Class's instead of Structs. Reduces much of the confusion
in making the threadpool.