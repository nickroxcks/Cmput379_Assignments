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
pthread_mutex_t mutex_job = PTHREAD_MUTEX_INITIALIZER;

ThreadPool_t  pool;
ThreadPool_work_t reduce_struct;
Reducer global_pnt_reduce;
int global_num_reducers;  //this global variable is to be passed to other functions
map<unsigned long, multimap<string,string> > inter_data;  //intermediate data structure

//find the size of a file(in local directory)
size_t find_File_size(char* filename) {
    struct stat temp;
    if(stat(filename, &temp) != 0) {
        return 0;}
    return temp.st_size;   
}
//strucct object used to associate a file with its size
struct MyFile
{
    char * name;
    size_t size;
};
//struct used to define comparator operator when calling sort()
struct file_less_than
{
    inline bool operator() (const MyFile& first_struct, const MyFile& second_struct)
    {
        return (first_struct.size > second_struct.size);
    }
};

//A c++ version of the MR_Partition function
unsigned long computePartitionNumber(string key, int numPartitions)
{      //cout<<"we reached computePartition number"<<endl;
    unsigned long hash = 5381;

    for(string::iterator it = key.begin(); it != key.end(); ++it)
    {
        hash = hash * 33 + *it;
    }

    return hash % numPartitions;
}
//original version of MR_Partition(for header file completeness)
unsigned long MR_Partition(char *key, int num_partitions) {
    unsigned long hash = 5381;
    int c;
    while ((c = *key++) != '\0')
    hash = hash * 33 + c;
    return hash % num_partitions;
}
//iterate through partion, and for each key, call Reduce. User should have called MR_GetNext inside Reduce
void MR_ProcessPartition(int partition_number){
    
    //this for loop looks at the partion, and finds each key once(even if the key is repeated)
    //locking is not required as we are simply reading from a database
    for(  multimap<string,string>::iterator it = inter_data[partition_number].begin(), end = inter_data[partition_number].end(); 
    it != end; it = inter_data[partition_number].upper_bound(it->first))
    {
        
        string key_name =  it->first;
        char * cString = (char *) key_name.c_str();
        (global_pnt_reduce)(cString,partition_number);  //calling the user defined reduce function

    }

}

bool ThreadPool_add_work(ThreadPool_t *tp, thread_func_t func, void *arg){
        ThreadPool_work_t struct_function;
        struct_function.func =(thread_func_t) func;  
        struct_function.arg = arg; 

        pthread_mutex_lock(&(tp->mutex_job));  //prevents concurrent changes to number of jobs
        tp->num_jobs = tp->num_jobs + 1;
        cout<<"main thread just increased job count. the job count is: "<<tp->num_jobs<<endl;
        pthread_mutex_unlock(&(tp->mutex_job));
        pthread_mutex_lock(&(tp->mutex_main));  //prevents concurrent changes to que
        (tp->work_queue).global_que.push_front(struct_function);
        pthread_mutex_unlock(&(tp->mutex_main));
        pthread_cond_signal(&(tp->cond));  //send notification to threadpool
        return 1;
}

void ThreadPool_destroy(ThreadPool_t *tp){
        //cout<<"waiting for everything to finish"<<endl;
        //in the event a thread is still working, wait for all threads in pool to finish
        while(tp->num_jobs !=0){
            
            ;//spinning
        }
        for(int i=0;i<tp->members_vector.size();i++){
            cout<<"going to kill "<<tp->members_vector.at(i)<<endl;;
            pthread_cancel(tp->members_vector.at(i));
        }
        //delete tp;
}

//recomended to have lock before calling this function and unlock afterwards
ThreadPool_work_t ThreadPool_get_work(ThreadPool_t *tp){
    ThreadPool_work_t struct_function = (tp->work_queue).global_que.at(0);  //grabs the task(front of que)
    (tp->work_queue).global_que.pop_front();  //removes task from que
    return struct_function;
}

void *Thread_run(ThreadPool_t *tp,ThreadPool_work_t work_task){
    cout<<" My id is ";
    cout<< pthread_self();
    cout<<" and i am working on the file" << (char *) work_task.arg<<endl;
    (work_task.func)(work_task.arg);
    pthread_mutex_lock(&(tp->mutex_job));
    cout<<"inside subthread that finished task. current num job before stopping is: "<<tp->num_jobs<<endl;
    tp->num_jobs = tp->num_jobs - 1;
    pthread_mutex_unlock(&(tp->mutex_job));
}
//Function run by new map threads
void * thread_function(void *ptr) {

   while(true){
        pthread_mutex_lock(&(pool.mutex_main));  //wait for a task to appear in que
        while ((pool.work_queue).global_que.empty()) {
            pthread_cond_wait(&(pool.cond), &(pool.mutex_main));
        }
        ThreadPool_work_t struct_function = ThreadPool_get_work(&pool);  //obtain the task
        cout<<"subthread has a task!"<<endl;
        cout<<struct_function.arg<<endl;
        pthread_mutex_unlock(&(pool.mutex_main));  //allow other threads to obtain tasks
        Thread_run(&pool,struct_function);  //run the task
        cout<<"looks like it ran nani"<<endl;
    }
    }
//Function called by new reduce threads. args_p is input taken upon creation of the thread
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
    for(int i=0;i<num;i++){
        pthread_t thread_handle;
        cout<<"thread being made"<<endl;
        pthread_create(&thread_handle, NULL, thread_function, NULL);
        (pool.members_vector).push_back(thread_handle);  //this is used for join later
    }
    return pnt; 
}


char *MR_GetNext(char *key, int partition_number){

    typedef std::multimap<string, string>::iterator MMAPIterator;
    string stringkey = string(key);
    std::pair<MMAPIterator, MMAPIterator> result = inter_data[partition_number].equal_range(stringkey);
    string value;
    // multi map functions returns ranges for iterators. This is why the loop is here. Only loops once
    for (MMAPIterator it = result.first; it != result.second; it++){
        value = it->second;
        pthread_mutex_lock(&mutex_thread);  //because we are removing from database, lock to prevent concurrent deletion
        inter_data[partition_number].erase(it);  //we delete the first occurence of key in the map, and exit loop. No need for more iterations
        pthread_mutex_unlock(&mutex_thread);
        return (char *) value.c_str();
        }

    return NULL;  //if we reach here, means no pair exists
}


void MR_Emit(char *key, char *value){

    string stringkey = string(key);
    string stringvalue = string(value);

    unsigned long placement = computePartitionNumber(stringkey,global_num_reducers);
    
    if(inter_data.count(placement)==0){
        //if we reach here, means the partition number doesnt exist in data base yet
        multimap<string,string> temp_map;
        
        temp_map.insert(std::pair<string,string>(stringkey,stringvalue));
        pthread_mutex_lock(&mutex_main); //just in case there is concurrent writing to database
        inter_data[placement] = temp_map;  //inter_data<unsigned long, multimap<string,string> >
        pthread_mutex_unlock(&mutex_main);
  
        for (multimap<string,string>::iterator it= temp_map.begin(); it != temp_map.end(); ++it) {
                cout << it->first << "\t" << it->second << endl ;
        }
        
    }
    else if(inter_data.count(placement)>0){
        //if we reach here, we are adding to a existing partion in database
        pthread_mutex_lock(&mutex_main);
        inter_data[placement].insert(std::pair<string,string>(stringkey,stringvalue));
        pthread_mutex_unlock(&mutex_main);
    }
    else{
        cout<<"Unkown Error Has Occured"<<endl;
    }
    
}

void MR_Run(int num_files, char *filenames[], Mapper map, int num_mappers,
Reducer concate, int num_reducers){

    pool = *ThreadPool_create(num_mappers);  //the thread pool object for mapping is global for ease of use
    global_num_reducers = num_reducers; //this global variable is to be passed to other functions

    //begin sorting
    vector<MyFile> file_vec;

    for(int i=0;i<num_files;i++){
        size_t temp_size = find_File_size(filenames[i]);
        MyFile temp_struct;
        temp_struct.name = filenames[i];
        temp_struct.size = temp_size;
        file_vec.push_back(temp_struct);
    }

    sort(file_vec.begin(), file_vec.end(), file_less_than());

    pool.num_jobs = 0;  //initializing number of jobs
    for(int i=0;i<num_files;i++){  //file_vec is a vector of files sorted for LJF
        ThreadPool_add_work(&pool,(thread_func_t)map,file_vec.at(i).name);
        }
    ThreadPool_destroy(&pool);

    cout<<"Map phase done"<<endl;

    //Beginning reducing phase
    vector<pthread_t> reducer_vector;
    int reduce_array[num_reducers];  //used so that threads have different partition #'s to work on
    global_pnt_reduce = concate;  //global function pointer used to call the user defined reducer function
    cout<<"about to create Reduce threads"<<endl;
    
    for(int i = 0;i<num_reducers;i++){
        pthread_t thread_handle;
        cout<<"thread being made with partition "<< i<<endl;
        reduce_array[i] = i;  //this is also done because main thread could finish loop before all subthreads begin
        pthread_create(&thread_handle, NULL, thread_function_reduce, &reduce_array[i]);
        reducer_vector.push_back(thread_handle);  //this is used for join later
    }
    for(int i = 0;i<num_reducers;i++){
        pthread_join(reducer_vector.at(i),NULL);  //main process only stops when all sub threads terminate
    }
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
    //cout<<"made it to the Reduce function"<<endl;
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
