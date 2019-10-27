#include "threadpool.h"
#include "mapreduce.h"
#include <pthread.h>


#include <vector>
#include <string>
#include <cstring>
#include <iostream>
#include <fstream>
#include <algorithm>
using namespace std;

#include <errno.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>

/*
void *Thread_run(ThreadPool_t *tp){

}
*/
/*
void *thread_function(void *ptr) {
 const char *msg = (char *)ptr;
 printf("%s\n", msg);
 return ptr; // return pointer to thread result, can’t be pointing to local variable
}

ThreadPool_t *ThreadPool_create(int num){

    ThreadPool_t  pool;
    pool.num_members = num;
    ThreadPool_t * pnt = &pool;

    pthread_t thread_handle;
    int num_to_pass = 42;
    void *ret1;

    pthread_create(&thread_handle, NULL, thread_function, &num_to_pass);
    pthread_join(thread_handle, &ret1); 

    return pnt; 

}








void MR_Run(int num_files, char *filenames[],
Mapper map, int num_mappers,
Reducer concate, int num_reducers){

ThreadPool_t tpool = *ThreadPool_create(num_mappers);



}


//CODE FOR DEBUGGING

void Map(char *file_name) {}
void Reduce(char *key, int partition_number){}
*/
int main(int argc, char **argv) {
   // MR_Run(argc - 1, &(argv[1]), Map, 10, Reduce, 10);
    //char * test = "dummy.txt";


}