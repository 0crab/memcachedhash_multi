#include "my_memcached.h"
#include "assoc.h"
#include "jenkins_hash.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#define ITEM_NUM 1000000
#define BUF_LEN 30
#define ITEM_LEN (sizeof(item)+BUF_LEN)
#define ITEMADDR(i) ((item *)(itemlist+i*ITEM_LEN))
#define THREAD_NUM 4
void * itemlist;
static uint64_t curr_items;


void *thread_set(uint32_t id){
    uint64_t insert_count=0;
    printf("%d starting set...\n",id);
    uint32_t hv;
    for(int i=0;i<ITEM_NUM;i++){
        //assoc_start_expand(curr_items);
        hv=jenkins_hash(ITEM_key(ITEMADDR(i)), ITEMADDR(i)->nkey);
        //if(assoc_insert(ITEMADDR(i), hv) == 1){
        item_lock(hv);
        bool ret=my_memcached_set(hv,ITEMADDR(i));
        item_unlock(hv);
        if(ret){
            //printf("insert key:%ld\tvalue%ld\n")
            insert_count++;
            curr_items++;
        }
    }
    printf("%d finish set :%ld\n",id,insert_count);
}
uint64_t get_runtime(struct timeval start,struct timeval end){
    return (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
}


int main(){
    struct timeval t1,t2,t3;
    itemlist=calloc(ITEM_LEN,ITEM_NUM);
    memset(itemlist,0, ITEM_LEN*ITEM_NUM);


    uint32_t nkey =8;//the length of uint64
    for(int i=0;i<ITEM_NUM;i++){
        uint64_t def_key=i,def_value=i*10;
        ITEMADDR(i)->nkey=nkey;
        *(uint64_t*)ITEM_key(ITEMADDR(i))=def_key;
        *(uint64_t*)ITEM_data(ITEMADDR(i))=def_value;
        //printf("def %ld--%ld\n",*(uint64_t*)ITEM_key(ITEMADDR(i)),*(uint64_t*)ITEM_data(ITEMADDR(i)));
    }
    printf("finish def\n");


    //item_lock_init();
    assoc_init(27);//set number of bucket to 2^27
    //start_assoc_maintenance_thread();
    item_lock_init();

    gettimeofday(&t1,NULL);
    pthread_t pid[THREAD_NUM];
    for(int i=0;i<THREAD_NUM;i++){
        if(pthread_create(&pid[i],NULL,thread_set,i)!=0){
            printf("create pthread error\n");
            return 0;
        }
    }
    for(int i=0;i<THREAD_NUM;i++){
        pthread_join(pid[i],NULL);
    }

    gettimeofday(&t2,NULL);
    uint64_t find_count=0;
    for(int i=0;i<10000;i++){
        int tmp=rand()%ITEM_NUM;
        uint64_t key=tmp;
        uint32_t hv=jenkins_hash(&key,nkey);
        
        item * it=assoc_find(&key,nkey,hv);
        if(it!=NULL){
            find_count++;
            //printf("%ld\t%ld\t%ld\n",key,*(uint64_t*)ITEM_key(it),*(uint64_t*)ITEM_data(it));
        }else{
            printf("%d  %s null\n",i,key);
        }
    }
    printf("find:%d\n",find_count);

    gettimeofday(&t3,NULL);
    printf("insert time:%ld\n",get_runtime(t1,t2));
    printf("find time:%ld\n",get_runtime(t2,t3));
    return 0;
}
