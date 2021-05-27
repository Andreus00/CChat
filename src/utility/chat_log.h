#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>

char *get_ascii_time_from_long(long t) {
    struct tm * timeinfo;
    timeinfo = localtime ( &t );
    char *ascii_time = asctime (timeinfo);
    ascii_time[strlen(ascii_time) - 1] = '\0';
    return ascii_time;
}

char *get_current_time() {
    time_t rawtime;
    time(&rawtime);
    return get_ascii_time_from_long(rawtime);
}

char *get_current_time_u() {
    #define RET_LEN 20
    struct timeval  t;
    gettimeofday(&t, NULL);
    char *t_str = calloc(RET_LEN, sizeof(char));
    snprintf(t_str, RET_LEN, "%lu", t.tv_sec * 1000000 + t.tv_usec);
    return t_str;
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