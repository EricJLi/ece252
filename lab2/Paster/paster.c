#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <sys/types.h>
#include <unistd.h>
#include "main_write_header_cb.h"

int main( int argc, char** argv ) {
    CURL *curl_handle;
    CURLcode res;
    char url[256];
    RECV_BUF recv_buf;
    char fname[256];
    pid_t pid = getpid();
    char filesRead[50][256];
    memset(filesRead, 0, 50*256);
    int fileCounter = 0;

    
    recv_buf_init(&recv_buf, BUF_SIZE);
    
    if (argc == 1) {
        strcpy(url, IMG_URL); 
    } else {
        strcpy(url, argv[1]);
    }
    printf("%s: URL is %s\n", argv[0], url);

    curl_global_init(CURL_GLOBAL_DEFAULT);

    /* init a curl session */
    curl_handle = curl_easy_init();

    if (curl_handle == NULL) {
        fprintf(stderr, "curl_easy_init: returned NULL\n");
        return 1;
    }

    //set URL
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);

    //set header cb to receive data
    curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, header_cb_curl);
    //set user defined header data structure
    curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, (void *)&recv_buf);

    //set write cb to receive data
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_cb_curl3); 
    //set user defined write data structure
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&recv_buf);

    /* some servers requires a user-agent field */
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
    return 0;
}