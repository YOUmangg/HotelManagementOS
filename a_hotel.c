#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<string.h>
#include<sys/wait.h>
#include<sys/ipc.h>
#include<sys/shm.h>

#define MAX_TABLE_COUNT 10
#define BUF_SIZE 100
#define EARNINGS_FILE "earnings.txt"
#define HOTEL_KEY 1234
#define SHARED_MEMORY_SIZE 512
#define MANAGER_KEY 2321

struct shared_data_hotel_manager {
    int termination_of_table;
};

struct shared_data_hm_waiter{
    int total_bill_amt;
};

void write_earnings(int table_number, int total_earnings) {
    FILE *fp = fopen(EARNINGS_FILE, "a");
    if (fp == NULL) {
        perror("Error opening earnings file");
        exit(EXIT_FAILURE);
    }

    fprintf(fp, "Earning from Table %d: %d INR\n", table_number, total_earnings);
    fclose(fp);
}

void calculate_and_display_totals(int total_earnings) {
    FILE *fp = fopen(EARNINGS_FILE, "a");
    if (fp == NULL) {
        perror("Error opening earnings file");
        exit(EXIT_FAILURE);
    }

	float total_wage = total_earnings*0.4;
	float total_profit = total_earnings*0.6;

	printf("Total Earnings of Hotel: %d INR\n", total_earnings);
    fprintf(fp, "Total Earnings of Hotel: %d INR\n", total_earnings);

	printf("Total Wage of Waiters: %.2f INR\n", total_earnings*0.4);
	fprintf(fp, "Total Wage of Waiters: %.2f INR\n", total_wage);

	printf("Total Profit: %.2f INR\n", total_profit);
	fprintf(fp, "Total Profit: %.2f INR\n", total_earnings*0.6);
	
    fclose(fp);
}

int main(){
    int total_tables;
    printf("Enter the Total Number of Tables at the Hotel: ");
    scanf("%d",&total_tables);

    printf("Please execute the table processes in separate terminals and then press enter");
    getchar();
    
    //Hotel manager tasks:
    int termination_count=0;
    int total_earnings=0;
    int earnings[MAX_TABLE_COUNT+1];
    for(int i=1;i<=10;i++){
        earnings[i] = -1; //clearing the array where earnings will be stored
    }

    while(termination_count<total_tables){
        //all the tables have to be terminated before termination of hotel manager
        for(int i=1;i<=10;i++){
            int shmid = shmget(HOTEL_KEY+i, SHARED_MEMORY_SIZE, 0666);
            if(shmid == -1){
                continue;
            }

            int shmid2 = shmget(MANAGER_KEY+i, SHARED_MEMORY_SIZE, 0666);
            if(shmid2 == -1){
                continue;
            }

            struct shared_data_hotel_manager *shared_data_hotel_manager = shmat(shmid, NULL, 0);
            if (shared_data_hotel_manager == (struct shared_data_hotel_manager *)(-1)) {
                continue;
            }
            
            struct shared_data_hm_waiter *shared_data_hm_waiter = shmat(shmid2, NULL, 0);
            if (shared_data_hm_waiter == (struct shared_data_hm_waiter *)(-1)) {
                continue;
            }

            if(earnings[i]==-1){
                earnings[i] = shared_data_hm_waiter->total_bill_amt;
            }
            else{
                earnings[i] += shared_data_hm_waiter->total_bill_amt;
            }
            total_earnings += earnings[i];

            if(shared_data_hotel_manager->termination_of_table){termination_count++;}

            if (shmdt(shared_data_hm_waiter) == -1) {
                continue;
            }

            if (shmdt(shared_data_hotel_manager) == -1) {
                continue;
            }
        }
    }

    for(int i=1; i<=10; i++){
        if(earnings[i] !=-1 )write_earnings(i,earnings[i]);
    }

    calculate_and_display_totals(total_earnings);
    return 0;
}

