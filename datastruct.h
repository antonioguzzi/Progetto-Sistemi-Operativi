#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define MAXDIM 64

#ifndef DATASTRUCT_H
#define DATASTRUCT_H
/*-------------------------------- struct --------------------------------*/
//////////////////////////////////////////// struct tempi cliente
struct ct{
    int id;
    float in_sm;
    float in_queue;
    int moves;
    int products;;
    struct ct* next;
};
typedef struct ct customer_times;
typedef customer_times* customer_timesPtr;
//////////////////////////////////////////// struct tempi
struct t{
    float open_time;
    struct t* next;
};
typedef struct t timer;
typedef timer* timerPtr;
//////////////////////////////////////////// struct cliente
struct c {
        int time_in_supermarket;
        int purchased_products;
        int my_turn;
        int id;
        pthread_cond_t cust_cond;
        struct timespec start;
        struct timespec stop;
        int move_num;
        float time_in_queue;
        float time_in_sm;
};
typedef struct c customer_info;
//////////////////////////////////////////// struct lista
struct n {
        pthread_t new_customers;
        struct n* next;
};
typedef struct n id_node;
typedef id_node* id_nodePtr;
//////////////////////////////////////////// struct coda
struct q {
        customer_info* cust;
        struct q* next;
};
typedef struct q queue;
typedef queue* queuePtr;
//////////////////////////////////////////// struct cassa
struct cash {
        int served_customers;
        int closures;
        int queue_len;
        int open;
        queuePtr q;
        pthread_cond_t empty_cond;
        pthread_mutex_t cash_mutex;
        int elab_product;
        struct timespec start;
        struct timespec stop;
        float service_time;
        int fix_time;
        timerPtr op_tm;
        timerPtr cust_tm;
};
typedef struct cash cashdesk_info;
//////////////////////////////////////////// struct dati
struct d {
        int K;
        int C;
        int E;
        int T;
        int P;
        int PRODUCT_TIME;
        int BOUND_MAX;
        int BOUND_MIN;
        int TIMER;
        char LOGFILE[MAXDIM];
};
typedef struct d data;
/*------------------------------------------------------------------------*/
#endif
