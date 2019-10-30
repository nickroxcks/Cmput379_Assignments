#include "threadpool.h"
#include "mapreduce.h"
#include <pthread.h>
#include <queue>
#include <assert.h>
#include <vector>
#include <map>
#include <string>
#include <cstring>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <iterator>
#include <deque>
//#include <bits/stdc++.h>
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

pthread_mutex_t mutex_main = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_thread = PTHREAD_MUTEX_INITIALIZER;
ThreadPool_t  pool;
int global_num_reducers;  //this global variable is to be passed to other functions
map<unsigned long, multimap<string,string> > inter_data;
//std:: queue<ThreadPool_work_t> global_que;
/*
void *Thread_run(ThreadPool_t *tp){

}
*/

unsigned long MR_Partition(char *key, int num_partitions) {
    cout<<"subthread reached MR_Partition"<<endl;
    unsigned long hash = 5381;
    int c;
    while ((c = *key++) != '\0'){
        hash = hash * 33 + c;
    }

    return hash % num_partitions;
}

//A c++ version of the MR_Partition function
unsigned long computePartitionNumber(string key, int numPartitions)
{      cout<<"we reached computePartition number"<<endl;
    unsigned long hash = 5381;

    for(string::iterator it = key.begin(); it != key.end(); ++it)
    {
        hash = hash * 33 + *it;
    }

    return hash % numPartitions;
}

void * thread_function(void *ptr) {
    
    /*const char *msg = (char *)ptr;
    printf("%s\n", msg);
    return ptr; // return pointer to thread result, can’t be pointing to local variable*/
    cout<<"I am thread ";
    cout<< pthread_self()<<endl;
    while((pool.work_queue).global_que.empty()){
        //spin
    }
    cout<<"A task has been submitted. sub thread is about to execute map"<<endl;
    ThreadPool_work_t struct_function = (pool.work_queue).global_que.at(0);  //grabs the task(front of que)
    (pool.work_queue).global_que.pop_front();  //removes task from que
    (struct_function.func)(struct_function.arg);  //runs the task
    }

ThreadPool_t *ThreadPool_create(int num){

    pool.num_members = num;
    ThreadPool_t * pnt = &pool;
    cout<<"about to create threads"<<endl;
    for(int i=0;i<1;i++){ //for(int i=0;i<pool.num_members;i++){  testing with 1 thread for now
        pthread_t thread_handle;

        pthread_create(&thread_handle, NULL, thread_function, NULL);
    }

    /*
    pthread_t thread_handle;
    const char *msg1 = "I am Thread 1";
    void *ret1;

    pthread_create(&thread_handle, NULL, thread_function, (void *)msg1);
    pthread_join(thread_handle, &ret1); 
    */

    return pnt; 

}





void MR_Emit(char *key, char *value){
    cout<<"subthread reached MR_EMIT"<<endl;
    string stringkey = string(key);
    string stringvalue = string(value);
   // cout<<stringkey<<endl;
    unsigned long placement = computePartitionNumber(stringkey,global_num_reducers);
    cout<<"we executed computePartitionNumber and got placement of"<<placement<<endl;
    
    if(inter_data.count(placement)==0){
        //this means the partition number doesnt exist in data base
        multimap<string,string> temp_map;
        
        temp_map.insert(std::pair<string,string>(stringkey,stringvalue));
        
        inter_data[placement] = temp_map;
        cout<<"Created new partition numbered: "<<placement;
        cout<<"about to print the values we inserted"<<endl;
        for (multimap<string,string>::iterator it= temp_map.begin(); it != temp_map.end(); ++it) {
                cout << it->first << "\t" << it->second << endl ;
        }
    
    }
    else if(inter_data.count(placement)>0){
        cout<<"going to enter the same placement numbered: "<<placement<<endl;
        inter_data[placement].insert(std::pair<string,string>(stringkey,stringvalue));
        /*
        //START DEBUGGING CODE
        cout<<"These are all values in the placement"<<endl;
        for (multimap<string,string>::iterator it= inter_data[placement].begin(); it != inter_data[placement].end(); ++it) {
                cout << it->first << "\t" << it->second << endl ;
        }
        cout<<"attempting to print the value we just inserted(you will see dupes for same key"<<endl;
        typedef std::multimap<string, string>::iterator MMAPIterator;
 
	    // It returns a pair representing the range of elements with key equal to 'c'
        std::pair<MMAPIterator, MMAPIterator> result = inter_data[placement].equal_range("a");
    
        std::cout << "All values for key a are," << std::endl;
    
        // Iterate over the range
        for (MMAPIterator it = result.first; it != result.second; it++){
            std::cout << it->second << std::endl;}
    
        // Total Elements in the range
        int count = std::distance(result.first, result.second);
        std::cout << "Total values for key 'a' are : " << count << std::endl;
        //END DEBUGGING CODE
        */
    }
    else{
        cout<<"we really shouldn't be printing this lol"<<endl;
    }
    
}


void MR_Run(int num_files, char *filenames[], Mapper map, int num_mappers,
Reducer concate, int num_reducers){

    pool = *ThreadPool_create(num_mappers);  //this will create the num_mappers number of threads
    global_num_reducers = num_reducers; //this global variable is to be passed to other functions
    sleep(5);
    for(int i=0;i<num_files;i++){  //TODO: must start longest job first policy
        ThreadPool_work_t struct_function;
        struct_function.func =(thread_func_t) map;  //to call this, go (struct_function.func)(arg)
        struct_function.arg = filenames[i];  //REMEMBER: this is pointer/adress.
        cout<<"the file we are checking is: "<<((char *)(struct_function.arg))<<endl;
        (pool.work_queue).global_que.push_front(struct_function);
        sleep(10);
        //cout<<"main thread is going to try running map"<<endl;
        //(struct_function.func)(struct_function.arg);

    }
    cout<<"MR_RUN done"<<endl;
}


//CODE FOR DEBUGGING

void Map(char *file_name) {
    cout<<"This is the map function. We are looking at the file: " <<(char *)file_name<<endl;
    FILE *fp = fopen(file_name, "r");
    assert(fp != NULL);
    char *line = NULL;
    size_t size = 0;
    while (getline(&line, &size, fp) != -1) {
        char *token, *dummy = line;
        while ((token = strsep(&dummy, " \t\n\r")) != NULL){
            cout<<"we are looking at the word: "<<token<<endl;
            MR_Emit(token, "1");
        }
    }
    free(line);
    fclose(fp);
}

void Reduce(char *key, int partition_number){}

int main(int argc, char *argv[]) {
    /*
    map<string, string> mymap;
    mymap["c"]="hi";
    mymap["a"]="how r you";
    mymap["b"]="hello";

    for (std::map<string, string>::iterator i = mymap.begin(); i != mymap.end(); i++)
    {
        cout << i->second << "\n";
    }
    */
    char *filenames[1] = {"dummy.txt"};
    MR_Run(1, filenames, Map, 10, Reduce, 10);
    //char * test = "dummy.txt";
    return 0;

}
