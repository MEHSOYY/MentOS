#include <stdio.h>
#include "syscall.h" 

void print_message(void* ptr) {
    char* message = (char*) ptr;
    printf("%s\n", message);
}

int main() {
    int thread1, thread2;
    char *message1 = "Thread 1";
    char *message2 = "Thread 2";

    // USES MentOS's built-in thread creation syscall
    thread1 = create_thread(print_message, (void*)message1);
    thread2 = create_thread(print_message, (void*)message2);

    // Waiting for the threads to finish 
    wait_thread(thread1);
    wait_thread(thread2);

    return 0;
}
