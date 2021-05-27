#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>

char *get_current_time() {
    time_t rawtime;
    struct tm * timeinfo;
    time ( &rawtime );
    timeinfo = localtime ( &rawtime );
    char *ascii_time = asctime (timeinfo);
    ascii_time[strlen(ascii_time) - 1] = '\0';
    struct timeval  start;
    gettimeofday(&start, NULL);
    printf("Time: %lu\n", start.tv_sec);

    printf("Time 2: %lu\n", rawtime);
    return ascii_time;
}

void chat_log(chat_message *msg, enum chat_mode m) {
    char log_file_name[16];
    strcpy(log_file_name, "log_file");
    FILE *f;
    if(m == RECEIVE_MODE) {
        unsigned int msg_len = strlen(msg->sender) + strlen(msg->time) + strlen(msg->message) + 7;
        char assembled_message[msg_len];
        snprintf(assembled_message, msg_len, "[%s, %s] %s\n", msg->sender, msg->time, msg->message);
        strcat(log_file_name, "RCV.txt");
        f = fopen(log_file_name, "a");
        fwrite(assembled_message, sizeof(char), msg_len, f);
    }
    else {
        strcat(log_file_name, "TMS.txt");
        f = fopen(log_file_name, "a");
        fwrite(msg->message, sizeof(char), strlen(msg->message), f);
    }
    fclose(f);
}