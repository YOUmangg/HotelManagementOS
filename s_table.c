#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#define SHARED_MEMORY_SIZE 512
#define READ_END 0
#define WRITE_END 1
#define MAX_CUST 5
#define BUFFER_SIZE 100
#define HOTEL_KEY 1234
#define TEST 0

struct shared_data_hotel_manager {
    int termination_of_table;
};

struct shared_data {
    int termination_of_waiter;
    int shared_array[BUFFER_SIZE];
    int billing_amt;
    int valid;
    int total_orders;
};

void read_menu() {
    FILE *file;
    char line[1000];

    file = fopen("menu.txt", "r");

    if (file == NULL) {
        perror("Error opening the file");
        exit(EXIT_FAILURE);
    }

    while (fgets(line, sizeof(line), file)) {
        printf("%s", line);
    }

    fclose(file);
}

void order_taking(int pipe_write_end) {
    int orders[BUFFER_SIZE];
    int size = 0;

    printf("\nEnter the serial number(s) of the item(s) to order from the menu. Enter -1 when done : ");

    int num;
    while (1) {
        scanf("%d", &num);
        if (num == -1) {
            break;
        }
        orders[size++] = num;

        if (size >= BUFFER_SIZE) {
            printf("Maximum number of orders reached\n");
            break;
        }
    }

    if (write(pipe_write_end, orders, size * sizeof(int)) == -1) {
        perror("Write error");
        exit(EXIT_FAILURE);
    }
}

void create_customer(int cust, struct shared_data *shared_data) {
    int pipes[MAX_CUST][2];
    int table_orders[BUFFER_SIZE];

    for (int i = 0; i < cust; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("Pipe creation failed");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < cust; i++) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("Failed to fork the process");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            close(pipes[i][READ_END]);
            order_taking(pipes[i][WRITE_END]);
            close(pipes[i][WRITE_END]);
            exit(EXIT_SUCCESS);
        } else {
            close(pipes[i][WRITE_END]);
            wait(NULL);
        }
    }

    int all_orders[BUFFER_SIZE * MAX_CUST];
    int total_orders = 0;

    for (int i = 0; i < cust; i++) {
        int received_data[BUFFER_SIZE];

        ssize_t bytes_read = read(pipes[i][READ_END], received_data, BUFFER_SIZE * sizeof(int));
        if (bytes_read == -1) {
            perror("Read error");
            exit(EXIT_FAILURE);
        } else if (bytes_read == 0) {
            printf("No data read from the pipe for customer %d.\n", i + 1);
        } else {
            for (int j = 0; j < bytes_read / sizeof(int); j++) {
                all_orders[total_orders++] = received_data[j];
            }
        }
        shared_data->total_orders = total_orders;
        close(pipes[i][READ_END]);
    }

    int counter = 0;
    int customer_start_index = 0;
    for (int i = 0; i < cust; i++) {
        for (int j = customer_start_index; j < total_orders; j++) {
            if (all_orders[j] == -1) {
                customer_start_index = j + 1;
                break;
            }
            shared_data->shared_array[counter++] = all_orders[j];
        }
    }
}

int main() {
    int table_no, cust;
    printf("Enter Table Number (between 1 to 10): ");
    scanf("%d", &table_no);

    int shmid = shmget(TEST + table_no, SHARED_MEMORY_SIZE, IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget failed");
        exit(EXIT_FAILURE);
    }

    int shmid2 = shmget(HOTEL_KEY+table_no, SHARED_MEMORY_SIZE, IPC_CREAT | 0666);
    if (shmid2 == -1) {
        perror("shmget failed");
        exit(EXIT_FAILURE);
    }

    struct shared_data *shared_data = shmat(shmid, NULL, 0);
    if (shared_data == (struct shared_data *)(-1)) {
        perror("shmat failed");
        exit(EXIT_FAILURE);
    }

    struct shared_data_hotel_manager *shared_data_hotel_manager = shmat(shmid2, NULL, 0);
    if (shared_data_hotel_manager == (struct shared_data_hotel_manager *)(-1)) {
        perror("shmat failed");
        exit(EXIT_FAILURE);
    }

    shared_data->termination_of_waiter = 0;
    shared_data_hotel_manager->termination_of_table = 0;

    while (1) {
        if(shared_data->valid==1){
            printf("Enter the number of customers at Table (maximum no. of customers can be 5) (-1 to exit): ");
            scanf("%d", &cust);


            if (cust == -1) {
                shared_data->termination_of_waiter = 1;
                break;
            }

            if (cust > MAX_CUST) {
                printf("Maximum number of customers exceeded.\n");
                continue;
            }

            read_menu();
            fflush(stdout);
        }
        create_customer(cust, shared_data);
    }

    printf("The total bill amount is %d INR.",shared_data->billing_amt);
    shared_data_hotel_manager->termination_of_table = 1;

    if (shmdt(shared_data) == -1) {
        perror("shmdt failed");
        exit(EXIT_FAILURE);
    }

    if (shmdt(shared_data_hotel_manager) == -1) {
        perror("shmdt failed");
        exit(EXIT_FAILURE);
    }

    // Cleanup the shared memory segment
    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("shmctl failed");
        exit(EXIT_FAILURE);
    }

    return 0;
}