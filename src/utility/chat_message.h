#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

#define BUFFER_SIZE 255
#define CHUNK 200

/**
 * this functions reads and returns a pointer to the string passed as input
*/
char* readinput_i(FILE *f) {
   char* input = NULL;
   char tempbuf[CHUNK];
   size_t inputlen = 0, templen = 0;
   do {
       fgets(tempbuf, CHUNK, f);
       templen = strlen(tempbuf);
       input = realloc(input, inputlen+templen+1);
       strcpy(input+inputlen, tempbuf);
       inputlen += templen;
    } while (templen==CHUNK-1 && tempbuf[CHUNK-2]!='\n');
    return input;
}

char* readinput() {
    return readinput_i(stdin);
} 

char *readinput_without_newline() {
    char *input = readinput();
    if (input[strlen(input) - 2] == '\n') {
        input[strlen(input) - 2] = '\0';
    }
    if (input[strlen(input) - 1] == '\n') {
        input[strlen(input) - 1] = '\0';
    }
    return input;
}


char *read_fd(int fd) {
    int read_len = 0, buffer_len = 0;
    char *buffer = NULL;
    char *temp_buff = malloc(sizeof(char) * BUFFER_SIZE);
    do {   // if there is something else to read
        memset(temp_buff, 0, BUFFER_SIZE);
        read_len = read(fd, temp_buff, BUFFER_SIZE - 1);
        if (read_len <= 0) {
            return NULL;
        }
        buffer = realloc(buffer, (buffer_len + read_len + 1));
        memset(buffer + buffer_len, 0, read_len);
        strcpy(buffer + buffer_len, temp_buff);
        buffer_len += read_len;
    }while ((temp_buff[read_len-1]!='\0' && temp_buff[read_len-1]!='\n') && (read_len >= (BUFFER_SIZE - 1)));
    
    return buffer;
}