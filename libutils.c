#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <time.h>

#include "libutils.h"
#include "datastruct.h"
#define MAXDIM 64

#define DISABLE_PRINTF
#ifdef DISABLE_PRINTF
    #define printf(fmt, ...) (0)
#endif

/*dati per statistiche globali*/
static int served_cust = 0;
static pthread_mutex_t mutex_served_cust = PTHREAD_MUTEX_INITIALIZER;
static int total_purchased_products = 0;
static pthread_mutex_t mutex_purchased_products = PTHREAD_MUTEX_INITIALIZER;
/*----------------------------*/

static pthread_mutex_t mutex_removed = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mutex_is_closed = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mutex_cust_id = PTHREAD_MUTEX_INITIALIZER;
extern pthread_mutex_t mutex_queue_info;
static int removed = 0;
static int cust_id = 0;
volatile sig_atomic_t is_closed  = 0;
extern int info_queue_modify;
data d;

void incr_removed() {
        pthread_mutex_lock(&mutex_removed);
        removed++;
        pthread_mutex_unlock(&mutex_removed);
        return;
}

void reset_removed() {
        pthread_mutex_lock(&mutex_removed);
        removed -= d.E;
        pthread_mutex_unlock(&mutex_removed);
        return;
}

int read_removed(){
        pthread_mutex_lock(&mutex_removed);
        int n = removed;
        pthread_mutex_unlock(&mutex_removed);
        return n;
}

int supermarket_is_closed(){
        pthread_mutex_lock(&mutex_is_closed);
        int n = is_closed;
        pthread_mutex_unlock(&mutex_is_closed);
        return n;
}

int read_served_cust(){
        pthread_mutex_lock(&mutex_served_cust);
        int n = served_cust;
        pthread_mutex_unlock(&mutex_served_cust);
        return n;
}

void incr_served_cust(int i) {
        pthread_mutex_lock(&mutex_served_cust);
        served_cust += i;
        pthread_mutex_unlock(&mutex_served_cust);
        return;
}

int random_choice(){
        unsigned int seed = rand();
        int choice;
        choice = rand_r(&seed) % d.K;
        return choice;
}

int read_mutex_queue_info () {
        pthread_mutex_lock(&mutex_queue_info);
        int n = info_queue_modify;
        pthread_mutex_unlock(&mutex_queue_info);
        return n;
}

int open_file(char const *file_name) {
        char string[MAXDIM];
        char val[MAXDIM];
        FILE* fptr;
        fptr = fopen(file_name, "r");
        if(fptr == NULL) {
                perror("apertura file.");
                return -1;
        }
        while (!feof(fptr)) {
                fscanf(fptr,"%s %s", string, val); //aggiungere controlli
                if (strncmp(string, "K", MAXDIM) == 0) d.K = atoi(val);
                if (strncmp(string, "C", MAXDIM) == 0) d.C = atoi(val);
                if (strncmp(string, "E", MAXDIM) == 0) d.E = atoi(val);
                if (strncmp(string, "T", MAXDIM) == 0) d.T = atoi(val);
                if (strncmp(string, "P", MAXDIM) == 0) d.P = atoi(val);
                if (strncmp(string, "PRODUCT_TIME", MAXDIM) == 0) d.PRODUCT_TIME = atoi(val);
                if (strncmp(string, "BOUND_MAX", MAXDIM) == 0) d.BOUND_MAX = atoi(val);
                if (strncmp(string, "BOUND_MIN", MAXDIM) == 0) d.BOUND_MIN = atoi(val);
                if (strncmp(string, "TIMER", MAXDIM) == 0) d.TIMER = atoi(val);
                if (strncmp(string, "LOGFILE", MAXDIM) == 0) strncpy(d.LOGFILE, val, MAXDIM);
        }
        if (fclose(fptr) == EOF) {
                perror("chiusura del file.");
                return -1;
        }
        return 0;
}

int read_cust_id(){
        pthread_mutex_lock(&mutex_cust_id);
        int n = cust_id;
        pthread_mutex_unlock(&mutex_cust_id);
        return n;
}

void new_cust_id(int i) {
        pthread_mutex_lock(&mutex_cust_id);
        cust_id += i;
        pthread_mutex_unlock(&mutex_cust_id);
}

int read_total_purchased_products() {
        pthread_mutex_lock(&mutex_purchased_products);
        int n = total_purchased_products;
        pthread_mutex_unlock(&mutex_purchased_products);
        return n;
}

void incr_total_purchased_products(int i) {
        pthread_mutex_lock(&mutex_purchased_products);
        total_purchased_products += i;
        pthread_mutex_unlock(&mutex_purchased_products);
        return;
}
