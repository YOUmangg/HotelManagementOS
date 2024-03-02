#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>

#define SHM_SIZE 4096

//communication with the hotel manager process
void tellHotelManager(char decision) {
    key_t key = ftok("admin.c", 'R');
    //permission to read, write to owner, group and others  
    int id = shmget(key, SHM_SIZE, 0666 | IPC_CREAT);
    if (id == -1) {
        perror("shmget");
        exit(1);
    }

    char *addr = shmat(id, (void*)0, 0);
    if (addr == (char *)(-1)) {
        perror("shmat");
        exit(1);
    }

    // Write the decision to shared memory
    sprintf(addr, "%c", decision);

    // Detach from shared memory
    shmdt(addr);
}

int main() {
    char decision;
    
    // Keep displaying message to user  
    printf("Do you want to close the hotel? Enter Y for Yes and N for No.\n");
    
    // Read input
    scanf(" %c", &decision);

    if (decision == 'Y' || decision == 'y') {
        // Inform hotel manager process
        tellHotelManager('Y');
        printf("Hotel closing process initiated.\n");
    } else {
        // Keep running as usual
        printf("Hotel will remain open.\n");
    }

    return 0;
}

