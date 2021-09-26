#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include "datastruct.h"
#include "libutils.h"
#include "libsm.h"

#define DISABLE_PRINTF
#ifdef DISABLE_PRINTF
    #define printf(fmt, ...) (0)
#endif

extern cashdesk_info* cashes;
extern int* info_queue;
extern data d;
extern queuePtr wait_list;
extern pthread_mutex_t mutex_authorization;
extern customer_timesPtr times_info;
extern pthread_mutex_t customer_time_info;

int wait_list_is_empty(){
        int n;
        pthread_mutex_lock(&mutex_authorization);
        if(wait_list == NULL) n = 1;
        else n = 0;
        pthread_mutex_unlock(&mutex_authorization);
        return n;
}

void exit_custom(queuePtr* q) {
        if(*q == NULL) return;
        (*q)->cust->my_turn = 1;
        pthread_cond_signal(&(((*q)->cust)->cust_cond));
        *q = (*q)->next;
        incr_served_cust(1);
        return;
}

int cashes_init() {
        for (size_t i = 0; i < d.K; i++) {
                unsigned int seed = rand();
                cashes[i].served_customers = 0;
                cashes[i].closures = 0;
                cashes[i].service_time = 0;
                cashes[i].fix_time = 20 + rand_r(&seed) % 61;
                cashes[i].queue_len = 0;
                cashes[i].elab_product = 0;
                cashes[i].open = 1;
                cashes[i].q = NULL;
                cashes[i].op_tm = NULL;
                cashes[i].cust_tm = NULL;
                clock_gettime(CLOCK_MONOTONIC, &cashes[i].start);
                if (pthread_cond_init(&cashes[i].empty_cond,NULL) != 0) {
                        perror("cond. cassa.");
                        return -1;
                }
                if (pthread_mutex_init(&cashes[i].cash_mutex,NULL) != 0) {
                        pthread_cond_destroy(&cashes[i].empty_cond);
                        perror("mutex cassa.");
                        return -1;
                }
        }
        return 0;
}

int tail_insert(queuePtr* q, customer_info* customer,int i) {
        queuePtr new_node = malloc(sizeof(queue));
        if(new_node == NULL) {
                perror("memoria esaurita.");
                return -1;
        }
        new_node->cust = customer;
        queuePtr prev = NULL;
        queuePtr curr = *q;
        while (curr != NULL) {
                prev = curr;
                curr = curr->next;
        }
        if(prev == NULL) {
                new_node->next = *q;
                *q = new_node;
        }else{
                prev->next = new_node;
                new_node->next = NULL;
        }
        cashes[i].queue_len++;
        return 0;
}

int wait_list_insert(queuePtr* q, customer_info* customer) {
        queuePtr new_node = malloc(sizeof(queue));
        if(new_node == NULL) {
                perror("memoria esaurita.");
                return -1;
        }
        new_node->cust = customer;
        queuePtr prev = NULL;
        queuePtr curr = *q;
        while (curr != NULL) {
                prev = curr;
                curr = curr->next;
        }
        if(prev == NULL) {
                new_node->next = *q;
                *q = new_node;
        }else{
                prev->next = new_node;
                new_node->next = NULL;
        }
        return 0;
}

int time_insert(timerPtr* op_tm, float open_timer) {
        timerPtr new_node = malloc(sizeof(timer));
        if(new_node == NULL) {
                perror("memoria esaurita.");
                return -1;
        }
        new_node->open_time = open_timer;
        new_node->next = NULL;
        timerPtr prev = NULL;
        timerPtr curr = *op_tm;

        while(curr != NULL) {
                prev = curr;
                curr = curr->next;
        }
        if(prev == NULL) {
                new_node->next = *op_tm;
                *op_tm = new_node;
        } else {
                prev->next = new_node;
                new_node->next = NULL;
        }
        return 0;
}

int head_insert(id_nodePtr* list, pthread_t new_thread) {
        id_nodePtr new_node = malloc(sizeof(id_node));
        if(new_node == NULL) {
                perror("memoria esaurita.");
                return -1;
        }
        new_node->new_customers = new_thread;
        new_node->next = *list;
        *list = new_node;
        return 0;
}

customer_info* init_customer(unsigned int seed, int i){
        customer_info* cust;
        cust = malloc(sizeof(customer_info));
        if(cust == NULL) {
                perror("allocazione cliente.");
                return NULL;
        }
        cust->time_in_supermarket = 10 + rand_r(&seed) % d.T - 10 + 1;
        cust->purchased_products = rand_r(&seed) % d.P + 1;
        if (pthread_cond_init(&cust->cust_cond,NULL) != 0) {
                perror("cond. cliente.");
                _exit(EXIT_FAILURE);
        }
        cust->my_turn = 0;
        cust->id = i;
        cust->move_num = 0;
        cust->time_in_queue = 0;
        cust->time_in_sm = 0;
        return cust;
}

void wake_up_customers(queuePtr *q) {
        while ((*q) != NULL) {
                pthread_cond_signal(&(((*q)->cust)->cust_cond));
                queuePtr tmp = *q;
                (*q) = (*q)->next;
                free(tmp);
        }
        return;
}

int elab_customer(queuePtr* q, int i) {
        if(*q == NULL) return 0;
        struct timespec t;
        t.tv_sec = (time_t)((cashes[i].fix_time + (((*q)->cust)->purchased_products * d.PRODUCT_TIME)) / 1000);
        float wait_time = ((cashes[i].fix_time + (((*q)->cust)->purchased_products * d.PRODUCT_TIME)) % 1000);
        t.tv_nsec = wait_time * 1000000;
        queuePtr tmp;
        float accum;

        clock_gettime(CLOCK_MONOTONIC, &(*q)->cust->stop);
        accum = (float)((*q)->cust->stop.tv_sec - (*q)->cust->start.tv_sec) + (float)((*q)->cust->stop.tv_nsec - (*q)->cust->start.tv_nsec)/(float)1000000000;
        (*q)->cust->time_in_queue = accum;
        (*q)->cust->my_turn = 1;
        nanosleep(&t, NULL);

        incr_total_purchased_products(((*q)->cust)->purchased_products);
        cashes[i].elab_product += ((*q)->cust)->purchased_products;
        float tmp_accum = ((float)(cashes[i].fix_time + (((*q)->cust)->purchased_products * d.PRODUCT_TIME)) / (float)1000);
        cashes[i].service_time += tmp_accum;
        if (time_insert(&cashes[i].cust_tm,tmp_accum) == -1) return -1;
        (*q)->cust->time_in_sm = accum + ((float)((*q)->cust->time_in_supermarket)/(float)1000) + ((float)(cashes[i].fix_time + (((*q)->cust)->purchased_products * d.PRODUCT_TIME)) / (float)1000);
        printf("CASSA (%d): il cliente %d è stato servito.\n", i, ((*q)->cust)->id);
        update_times_list(((*q)->cust)->id, ((*q)->cust)->time_in_sm, ((*q)->cust)->time_in_queue, ((*q)->cust)->move_num, ((*q)->cust)->purchased_products);
        cashes[i].served_customers++;

        tmp = *q;
        pthread_cond_signal(&(((*q)->cust)->cust_cond));
        *q = (*q)->next;
        free(tmp);

        incr_served_cust(1);
        cashes[i].queue_len--;
        incr_removed();
        return 0;
}

int cashdesk_is_open(int i){
        int n;
        pthread_mutex_lock(&cashes[i].cash_mutex);
        n = cashes[i].open;
        pthread_mutex_unlock(&cashes[i].cash_mutex);
        return n;
}

int check_closing_cashdesk(int count, int* open) {
        for (size_t i = 0; i < d.K; i++) {
                if(cashdesk_is_open(i) && info_queue[i] <= 1)
                        count++;
        }
        if(supermarket_is_closed() == 0 && count >= d.BOUND_MIN && (*open) >= 2) {
                count = 0;
                float accum;
                int choice;

                do {
                        choice = random_choice();
                } while(!cashdesk_is_open(choice));

                pthread_mutex_lock(&cashes[choice].cash_mutex);
                printf("==== CHIUDO LA CASSA %d CON %d CLIENTI ====\n",choice, cashes[choice].queue_len);
                cashes[choice].open = 0;
                cashes[choice].closures++;
                (*open)--;

                clock_gettime(CLOCK_MONOTONIC, &cashes[choice].stop);
                accum = (float)((cashes[choice].stop).tv_sec - (cashes[choice].start).tv_sec) + (float)((cashes[choice].stop).tv_nsec - (cashes[choice].start).tv_nsec)/(float)1000000000;
                if(supermarket_is_closed() == 0) {
                    if (time_insert(&cashes[choice].op_tm, accum) == -1){
                        fprintf(stderr, "errore inserimento del tempo di apertura.\n");
                        pthread_mutex_unlock(&cashes[choice].cash_mutex);
                        return -1;
                    }
                }

                int pick;
                while (supermarket_is_closed() == 0 && cashes[choice].q != NULL) {
                        do {
                                pick = random_choice();
                                while (pick == choice) pick = random_choice();
                        } while(!cashdesk_is_open(pick));

                        pthread_mutex_lock(&cashes[pick].cash_mutex);
                        if(supermarket_is_closed() == 0) {
                                printf("==== SPOSTO UN CLIENTE DA %d A %d ====\n",choice, pick);
                                if (tail_insert(&cashes[pick].q, (cashes[choice].q)->cust, pick) == -1){
                                    fprintf(stderr, "errore di spostamento del cliente %d.\n",(cashes[choice].q)->cust->id);
                                    pthread_mutex_unlock(&cashes[pick].cash_mutex);
                                    return -1;
                                }
                                (cashes[choice].q)->cust->move_num++;
                                pthread_cond_signal(&cashes[pick].empty_cond);
                                pthread_mutex_unlock(&cashes[pick].cash_mutex);
                                queuePtr tmp = cashes[choice].q;
                                cashes[choice].q = tmp->next;
                                free(tmp);
                                cashes[choice].queue_len--;
                        }else pthread_mutex_unlock(&cashes[pick].cash_mutex);
                }
                pthread_mutex_unlock(&cashes[choice].cash_mutex);
        }
        return 0;
}

void check_open_cashdesk(int* open) {
        int i = 0;
        while (i < d.K && info_queue[i] < d.BOUND_MAX) i++;
        if(supermarket_is_closed() == 0 && i < d.K && (*open) < d.K) {
                int choice;

                do {
                        choice = random_choice();
                } while(cashdesk_is_open(choice));
                pthread_mutex_lock(&cashes[choice].cash_mutex);
                if(supermarket_is_closed() == 0) {
                        printf("==== APRO LA CASSA %d ====\n",choice);
                        cashes[choice].open = 1;
                        clock_gettime(CLOCK_MONOTONIC, &cashes[choice].start);
                        (*open)++;
                }
                pthread_mutex_unlock(&cashes[choice].cash_mutex);
        }
        return;
}

void cust_times_insert(customer_timesPtr* list, int id, float in_sm, float in_queue, int moves, int products){
        customer_timesPtr new_node = malloc(sizeof(customer_times)); //controllare
        if(new_node == NULL) {
                perror("memoria esaurita.");
                return;
        }

        new_node->id = id;
        new_node->in_sm = in_sm;
        new_node->in_queue = in_queue;
        new_node->moves = moves;
        new_node->products = products;
        new_node->next = NULL;

        customer_timesPtr prev = NULL;
        customer_timesPtr curr = *list;
        while(curr != NULL && curr->id < new_node->id) {
                prev = curr;
                curr = curr->next;
        }
        if(prev == NULL) {
                new_node->next = *list;
                *list = new_node;
        } else {
                prev->next = new_node;
                new_node->next = curr;
        }

        return;
}

void update_times_list(int id, float in_sm, float in_queue, int moves, int products) {
        pthread_mutex_lock(&customer_time_info);
        cust_times_insert(&times_info,id,in_sm,in_queue,moves,products);
        pthread_mutex_unlock(&customer_time_info);
        return;
}

int write_in_logfile() {
        FILE* logPtr;
        logPtr = fopen(d.LOGFILE, "w");
        if(logPtr == NULL) {
                perror("apertura file.");
                return -1;
        }

        customer_timesPtr tmp = times_info;
        while (tmp != NULL) {
                fprintf(logPtr,"CLIENTE:%d\n", tmp->id);
                fprintf(logPtr,"\tPRODOTTI ACQUISTATI:%d\n", tmp->products);
                fprintf(logPtr,"\tTEMPO NEL SUPERMERCATO:%f\n", tmp->in_sm);
                fprintf(logPtr,"\tTEMPO IN CODA:%f\n",tmp->in_queue);
                fprintf(logPtr,"\tCAMBI DI CODA:%d\n",tmp->moves);
                tmp = tmp->next;
        }
        fprintf(logPtr,"-------------------------------------------\n");
        for (size_t i = 0; i < d.K; i++) {
                fprintf(logPtr,"CASSA:%ld\n", i);
                fprintf(logPtr,"\tCLIENTI SERVITI:%d\n",cashes[i].served_customers);
                fprintf(logPtr,"\tCHIUSURE:%d\n",cashes[i].closures);
                fprintf(logPtr,"\tTEMPO DI SERVIZIO:%f\n",cashes[i].service_time);
                fprintf(logPtr,"\tPRODOTTI BATTUTI:%d\n",cashes[i].elab_product);
                fprintf(logPtr,"\tTEMPI DI APERTURA:");
                while (cashes[i].op_tm != NULL) {
                        timerPtr tmp = cashes[i].op_tm;
                        fprintf(logPtr,"%f,",tmp->open_time);
                        cashes[i].op_tm = (cashes[i].op_tm)->next;
                        free(tmp);
                }
                fprintf(logPtr,"\n");
                fprintf(logPtr,"\tTEMPI DI ELABORAZIONE DEI CLIENTI: ");
                while (cashes[i].cust_tm != NULL) {
                        timerPtr tmp = cashes[i].cust_tm;
                        fprintf(logPtr,"%f,",tmp->open_time);
                        cashes[i].cust_tm = (cashes[i].cust_tm)->next;
                        free(tmp);
                }
                fprintf(logPtr,"\n");
        }
        fprintf(logPtr,"-------------------------------------------\n");
        fprintf(logPtr,"SUPERMERCATO: clienti totali serviti %d, prodotti totali acquistati %d\n",read_served_cust(), read_total_purchased_products());

        if(fclose(logPtr)== EOF) {
                perror("chiusura del file di log.");
                return -1;
        }
        return 0;
}

void free_times() {
    while (times_info != NULL) {
            customer_timesPtr tmp = times_info;
            times_info = times_info->next;
            free(tmp);
    }
    return;
}
