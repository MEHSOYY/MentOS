#include <stdio.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <unistd.h>

int main() {
    int shmid = shmget(IPC_PRIVATE, 1024, 0666|IPC_CREAT);
    char *shm = (char*) shmat(shmid, NULL, 0);

    if (fork() == 0) {
        sprintf(shm, "Hello from child process!");
        shmdt(shm);
    } else {
        // Block for a short period to allow the child to finish writing
        sleep(1);

        printf("Parent process reads: %s\n", shm);
        shmdt(shm);
        shmctl(shmid, IPC_RMID, NULL);
    }
    return 0;
}
