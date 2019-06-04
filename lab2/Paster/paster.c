#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <sys/types.h>
#include <pthread.h>
#include <unistd.h>
#include "main_write_header_cb.h"

#define NUMT 2

struct thread_args              /* thread input parameters struct */
{
    int x;
    int y;
};

struct thread_ret               /* thread return values struct   */
{
    int sum;
    int product;
};

char url[256];
int fileCounter;
char filesRead[50][256];

void *getImage(void *arg);

int main(int argc, char** argv) {
    int numThreads = 0;
    int picNum = 0;
    pthread_t p_tids[NUMT];
    fileCounter = 0;
    memset(filesRead, 0, 50*256);
    int c;
    while ((c = getopt (argc, argv, "t:n:")) != -1) {
        printf("Loop %i", c);
        switch (c) {
        case 't':
            numThreads = strtoul(optarg, NULL, 10);
            printf("option -t specifies a value of %d.\n", numThreads);
        case 'n':
            picNum = strtoul(optarg, NULL, 10);
	        printf("option -n specifies a value of %d.\n", picNum);
        default:
            return -1;
        }
    }
    
    if (argc == 1) {
        strcpy(url, IMG_URL); 
    } else {
        strcpy(url, argv[1]);
    }
    printf("%s: URL is %s\n", argv[0], url);

    CURLcode code = curl_global_init(CURL_GLOBAL_DEFAULT);

    for (int x = 0; x < NUMT; x++) {
        pthread_create(&p_tids[x], NULL, getImage, (void*)&p_tids[x]);
    }

    for (int x = 0; x < NUMT; x++) {
        pthread_join(p_tids[x], NULL);
    }

    return 0;
}

void *getImage(void *arg) {
    CURLcode res;
    char fname[256];
    CURL *curl_handle;
    RECV_BUF recv_buf;
    pid_t pid = getpid();
    //sprintf(in_params[0], "./output_%d.png", pid);

    recv_buf_init(&recv_buf, BUF_SIZE);

    curl_handle = curl_easy_init();
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, header_cb_curl);
    curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, (void *)&recv_buf);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_cb_curl3); 
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&recv_buf);
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    
    while(fileCounter != 50) {
        res = curl_easy_perform(curl_handle);

        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            break;
        } else {
            printf("%lu bytes received in memory %p with sequence number %i and fileCounter %i\n", recv_buf.size, recv_buf.buf, recv_buf.seq, fileCounter);
        }
        
        sprintf(fname, "./output_%d.png", pid);

        if (strlen(filesRead[recv_buf.seq - 1]) == 0) {
            strcpy(filesRead[recv_buf.seq - 1], fname);
            fileCounter++;
        }

        // write_file(fname, recv_buf.buf, recv_buf.size);

        if (fileCounter != 50) {
            recv_buf_cleanup(&recv_buf);
            recv_buf_init(&recv_buf, BUF_SIZE);
        }
    }

    /* cleaning up */
    curl_easy_cleanup(curl_handle);
    curl_global_cleanup();
    recv_buf_cleanup(&recv_buf);
    return NULL;
}