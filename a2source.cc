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

pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex_main = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_thread = PTHREAD_MUTEX_INITIALIZER;
ThreadPool_t  pool;
ThreadPool_work_t reduce_struct;
Reducer global_pnt_reduce;
int global_num_reducers;  //this global variable is to be passed to other functions
map<unsigned long, multimap<string,string> > inter_data;
//std:: queue<ThreadPool_work_t> global_que;
/*
void *Thread_run(ThreadPool_t *tp){

}
*/

size_t find_File_size(char* filename) {
    struct stat temp;
    if(stat(filename, &temp) != 0) {
        return 0;}
    return temp.st_size;   
}
struct MyFile
{
    char * name;
    size_t size;
};

struct file_less_than
{
    inline bool operator() (const MyFile& first_struct, const MyFile& second_struct)
    {
        return (first_struct.size > second_struct.size);
    }
};

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
//iterate through partion, and for each key, call Reduce. Reduce will then call Mr_next at some point
void MR_ProcessPartition(int partition_number){
    
    //this for loop looks at the partion, and finds each key once(even if the key is repeated)
    //locking is not required as we are simply reading from a database
    for(  multimap<string,string>::iterator it = inter_data[partition_number].begin(), end = inter_data[partition_number].end(); 
    it != end; it = inter_data[partition_number].upper_bound(it->first))
    {
        
        string key_name =  it->first;
        cout<<"about to call reduce for the key: "<<key_name<<endl;
        char * cString = (char *) key_name.c_str();
        (global_pnt_reduce)(cString,partition_number);  //calling the reduce function

    }

}

void * thread_function(void *ptr) {
    
    /*const char *msg = (char *)ptr;
    printf("%s\n", msg);
    return ptr; // return pointer to thread result, can’t be pointing to local variable*/
    cout<<"I am thread a thread! My id is ";
    cout<< pthread_self()<<endl;
    /*
    while((pool.work_queue).global_que.empty()){
        //spin
    }
    cout<<"A task has been submitted. sub thread is about to execute map"<<endl;
    ThreadPool_work_t struct_function = (pool.work_queue).global_que.at(0);  //grabs the task(front of que)
    (pool.work_queue).global_que.pop_front();  //removes task from que
    (struct_function.func)(struct_function.arg);  //runs the task
    */
   while(true){
        pthread_mutex_lock(&mutex_main);
        while ((pool.work_queue).global_que.empty()) {
            pthread_cond_wait(&cond, &mutex_main);
        }
        /* operate on x and y */
        ThreadPool_work_t struct_function = (pool.work_queue).global_que.at(0);  //grabs the task(front of que)
        (pool.work_queue).global_que.pop_front();  //removes task from que
        pthread_mutex_unlock(&mutex_main);
        (struct_function.func)(struct_function.arg);
    }
    }

void * thread_function_reduce(void *args_p) {
    cout<<"I am thread a thread! My id is ";
    cout<< pthread_self();
    int* num = (int *) args_p;  //some cute casting
    cout<<" and I am handling partition " << *num <<endl;
    MR_ProcessPartition(*num);

}

ThreadPool_t *ThreadPool_create(int num){

    pool.num_members = num;
    ThreadPool_t * pnt = &pool;
    cout<<"about to create threads"<<endl;
    for(int i=0;i<num;i++){ //for(int i=0;i<pool.num_members;i++){  testing with 1 thread for now
        pthread_t thread_handle;
        cout<<"thread being made"<<endl;
        pthread_create(&thread_handle, NULL, thread_function, NULL);
        (pool.members_vector).push_back(thread_handle);  //this is used for join later
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


char *MR_GetNext(char *key, int partition_number){
    cout<<"made it to MR_GetNext"<<endl;
    int num_erased;  //this is used so we know when to return NULL.
    typedef std::multimap<string, string>::iterator MMAPIterator;
    string stringkey = string(key);
    std::pair<MMAPIterator, MMAPIterator> result = inter_data[partition_number].equal_range(stringkey);
    string value;
    // multi map functions tend to returns ranges for iterators. This is why the loop is here
    for (MMAPIterator it = result.first; it != result.second; it++){
        value= it->second;
        pthread_mutex_lock(&mutex_thread);
        inter_data[partition_number].erase(it);  //we delete the first occurence of key in the map, and exit loop. No need for more iterations
        pthread_mutex_unlock(&mutex_thread);
        return (char *) value.c_str();
        }

    return NULL;  //if we reach here, means no pair exists
}


void MR_Emit(char *key, char *value){
   // cout<<"subthread reached MR_EMIT"<<endl;
    string stringkey = string(key);
    string stringvalue = string(value);
   // cout<<stringkey<<endl;
    unsigned long placement = computePartitionNumber(stringkey,global_num_reducers);
   // cout<<"we executed computePartitionNumber and got placement of"<<placement<<endl;
    
    if(inter_data.count(placement)==0){
        //this means the partition number doesnt exist in data base
        multimap<string,string> temp_map;
        
        temp_map.insert(std::pair<string,string>(stringkey,stringvalue));
        pthread_mutex_lock(&mutex_main); //just in case there is concurrent writing to database
        inter_data[placement] = temp_map;
        pthread_mutex_unlock(&mutex_main);
        //cout<<"Created new partition numbered: "<<placement;
       // cout<<"about to print the values we inserted"<<endl;
        for (multimap<string,string>::iterator it= temp_map.begin(); it != temp_map.end(); ++it) {
                cout << it->first << "\t" << it->second << endl ;
        }
    
    }
    else if(inter_data.count(placement)>0){
        //cout<<"going to enter the same placement numbered: "<<placement<<endl;
        pthread_mutex_lock(&mutex_main);
        inter_data[placement].insert(std::pair<string,string>(stringkey,stringvalue));
        pthread_mutex_unlock(&mutex_main);
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

    //begins sorting
    vector<MyFile> file_vec;

    for(int i=0;i<num_files;i++){
        size_t temp_size = find_File_size(filenames[i]);
        MyFile temp_struct;
        temp_struct.name = filenames[i];
        temp_struct.size = temp_size;
        file_vec.push_back(temp_struct);
    }

    sort(file_vec.begin(), file_vec.end(), file_less_than());

    for(int i=0;i<file_vec.size();i++){
        cout<<file_vec.at(i).name << endl;
    }
    
    for(int i=0;i<num_files;i++){  //TODO: must start longest job first policy
        ThreadPool_work_t struct_function;
        struct_function.func =(thread_func_t) map;  //to call this, go (struct_function.func)(arg)
        struct_function.arg = file_vec.at(i).name;  //REMEMBER: this is pointer/adress.
        //cout<<"the file we are checking is: "<<((char *)(struct_function.arg))<<endl;
       // cout<<"taking our time. this is task "<<i<<endl;
        //sleep(10);
        pthread_mutex_lock(&mutex_main);
        (pool.work_queue).global_que.push_front(struct_function);
        pthread_mutex_unlock(&mutex_main);
       // cout<<"main is going to send the signal"<<endl;
        pthread_cond_signal(&cond);
        }
        //wait for all threads in pool to finish
        //TODO:implement destruction function
        cout<<"waiting for everything to finish"<<endl;
        sleep(10);
        for(int i=0;i<pool.members_vector.size();i++){

            pthread_cancel(pool.members_vector.at(i));
        }
        //(struct_function.func)(struct_function.arg);

    
    cout<<"Reduce phase done"<<endl;
    cout<<"verifying partitions";
    //vector<string,string> v;
      for (std::map<unsigned long,multimap<string,string> >::iterator it=inter_data.begin(); it!=inter_data.end(); ++it){
            cout<<it->first <<" "<<endl;
      }
        cout<<"partition 0"<<endl;
      for (multimap<string,string>::iterator it= inter_data[0].begin(); it != inter_data[0].end(); ++it) {
                cout << it->first << "\t" << it->second << endl ;
        }
        cout<<"p1"<<endl;
        for (multimap<string,string>::iterator it= inter_data[1].begin(); it != inter_data[1].end(); ++it) {
                cout << it->first << "\t" << it->second << endl ;
        }
        cout<<"p2"<<endl;
        for (multimap<string,string>::iterator it= inter_data[2].begin(); it != inter_data[2].end(); ++it) {
                cout << it->first << "\t" << it->second << endl ;
        }
        cout<<"p3"<<endl;
        for (multimap<string,string>::iterator it= inter_data[3].begin(); it != inter_data[3].end(); ++it) {
                cout << it->first << "\t" << it->second << endl ;
        }
        cout<<"p4"<<endl;
        for (multimap<string,string>::iterator it= inter_data[4].begin(); it != inter_data[4].end(); ++it) {
                cout << it->first << "\t" << it->second << endl ;
        }
        cout<<"p5"<<endl;
        for (multimap<string,string>::iterator it= inter_data[5].begin(); it != inter_data[5].end(); ++it) {
                cout << it->first << "\t" << it->second << endl ;
        }
        cout<<"p6"<<endl;
        for (multimap<string,string>::iterator it= inter_data[6].begin(); it != inter_data[6].end(); ++it) {
                cout << it->first << "\t" << it->second << endl ;
        }

    //Beginning reducing phase
    vector<pthread_t> reducer_vector;
    //reduce_struct.func = (thread_func_t) concate;  //the function to be used for reduce(global from global struct)
    int reduce_array[num_reducers];  //used to refer to an address of a value when calling pthread_create
    global_pnt_reduce = concate;  //function pointer used to call the user defined reducer function
    cout<<"about to create Reduce threads"<<endl;
    for(int i = 0;i<num_reducers;i++){//for(int i=0;i<num_reducers;i++){ 
        pthread_t thread_handle;
        cout<<"thread being made with partition "<< i<<endl;
        reduce_array[i] = i;  //this is done because main thread could finish loop before all subthreads begin
        pthread_create(&thread_handle, NULL, thread_function_reduce, &reduce_array[i]);
        reducer_vector.push_back(thread_handle);  //this is used for join later
    }
    sleep(5);
    cout<<"ALL FINISHED"<<endl;
    
}


//CODE FOR DEBUGGING

void Map(char *file_name) {
   // cout<<"This is the map function. We are looking at the file: " <<(char *)file_name<<endl;
    FILE *fp = fopen(file_name, "r");
    assert(fp != NULL);
    char *line = NULL;
    size_t size = 0;
    while (getline(&line, &size, fp) != -1) {
        char *token, *dummy = line;
        while ((token = strsep(&dummy, " \t\n\r")) != NULL){
            //cout<<"we are looking at the word: "<<token<<endl;
            MR_Emit(token, "1");
        }
    }
    free(line);
    fclose(fp);
}

void Reduce(char *key, int partition_number) {
    cout<<"made it to the Reduce function"<<endl;
    int count = 0;
    char *value, name[100];
    while ((value = MR_GetNext(key, partition_number)) != NULL)
        count++;
    sprintf(name, "result-%d.txt", partition_number);
    FILE *fp = fopen(name, "a");
    printf("%s: %d\n", key, count);
    fprintf(fp, "%s: %d\n", key, count);
    fclose(fp);
}

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
    char *filenames[3] = {"dummy.txt","dummy2.txt","dummy3.txt"};
    MR_Run(3, filenames, Map, 10, Reduce, 10);
    //char * test = "dummy.txt";
    return 0;

}
