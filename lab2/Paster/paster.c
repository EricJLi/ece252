#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>    /* for errno                   */
#include "lab_png.h"  /* simple PNG data structures  */
#include "crc.h"      /* for crc()                   */
#include "zutil.h"    /* for mem_def() and mem_inf() */
#include <curl/curl.h>
#include <sys/types.h>
#include <pthread.h>
#include <unistd.h>
#include "main_write_header_cb.h"

struct thread_args              /* thread input parameters struct */
{
    char url[256];
};

struct thread_file
{
    char *buffer;
    size_t buffer_size;
};

int fileCounter;
char filesRead[50][256];
struct thread_file fileData[50];

void *getImage(void *arg);
int catPng();

int main(int argc, char** argv) {
    fileCounter = 0;
    memset(filesRead, 0, 50*256);

    int numThreads = 1;
    int picNum = 1;
    int c;

    while ((c = getopt (argc, argv, "t:n:")) != -1) {
        switch (c) {
        case 't':
            numThreads = strtoul(optarg, NULL, 10);
        break;
        case 'n':
            picNum = strtoul(optarg, NULL, 10);
        break;
        default:
            return -1;
        }
    }

    pthread_t p_tids[numThreads];
    struct thread_args in_params[numThreads];

    curl_global_init(CURL_GLOBAL_DEFAULT);

    for (int x = 0; x < numThreads; x++) {
        char url[256];
        int threadNumber = x % 3;
        if (threadNumber == 0) {
            threadNumber = 3;
        }
        sprintf(url, "http://ece252-%d.uwaterloo.ca:2520/image?img=%d", threadNumber, picNum);
        printf("URL: %s\n", url);
        strcpy(in_params[x].url, url);
        pthread_create(&p_tids[x], NULL, getImage, in_params + x);
    }

    for (int x = 0; x < numThreads; x++) {
        pthread_join(p_tids[x], NULL);
    }

    catPng();
    
    for (int x = 0; x < 50; x++) {
        free(fileData[x].buffer);
    }

    return 0;
}

void *getImage(void *arg) {
    CURLcode res;
    CURL *curl_handle;
    RECV_BUF recv_buf;
    struct thread_args *p_in = arg;

    //sprintf(in_params[0], "./output_%d.png", pid);

    recv_buf_init(&recv_buf, BUF_SIZE);

    curl_handle = curl_easy_init();
    curl_easy_setopt(curl_handle, CURLOPT_URL, p_in->url);
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
        
        if (strlen(filesRead[recv_buf.seq]) == 0) {
            sprintf(filesRead[recv_buf.seq], "output_%d.png", recv_buf.seq);
            fileData[recv_buf.seq].buffer = malloc(recv_buf.size);
            memcpy(fileData[recv_buf.seq].buffer, recv_buf.buf, recv_buf.size);
            fileData[recv_buf.seq].buffer_size = recv_buf.size;
            fileCounter++;
        }

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

int catPng() {
    U32 totalHeight = 0;
    U64 bufCounter = 0;
    U64 totalCounter = 0;
    U32 width = 0;

    memcpy(&width, &fileData[0].buffer[16], 4);
    width = ntohl(width);

    for (int k = 0; k < 50; k++) {
        //totalHeight
    	U32 height = 0;
        memcpy(&height, &fileData[k].buffer[20], 4);
        totalHeight += ntohl(height);
    }

    U64 totalBufferSize = ((width * 4) + 1) * totalHeight;
    U8 buffer[500000];
    U8 total[totalBufferSize];

    for(int a = 0; a < 50; a++){
        //IDAT data
        U32 IDAT_1st4bytes = 0;
        memcpy(&IDAT_1st4bytes, &fileData[a].buffer[33], 4);
        U64 source_len = ntohl(IDAT_1st4bytes);

        U8 source[source_len];
        memcpy(&source, &fileData[a].buffer[41], source_len);

        U64 dest_len = 0;
        U8 dest[source_len * 500];

        mem_inf(dest, &dest_len, source, source_len);

        //将每个PNG中的data放到一个大的buffer[]中,前面接着后面
        for(int y = 0; y < dest_len; y++){
            buffer[y + bufCounter] = dest[y];
        }
        bufCounter += dest_len;
    }

    mem_def(total, &totalCounter, buffer, bufCounter, Z_DEFAULT_COMPRESSION);

    unsigned int input32 = 0;
    unsigned long int input64 = 0;
    unsigned char input8 = 0;
    FILE* fnew = fopen("outpng.png", "w+");

    //header
    memcpy(&input64, &fileData[0].buffer[0], 8);
    fwrite(&input64, 8, 1, fnew);

    //IHDR
    unsigned int oIHDRLength = 0;
    memcpy(&oIHDRLength, &fileData[0].buffer[8], 4);
    fwrite(&oIHDRLength, 4, 1, fnew);
    memcpy(&input32, &fileData[0].buffer[12], 4);
    fwrite(&input32, 4, 1, fnew);
    memcpy(&input32, &fileData[0].buffer[16], 4);
    fwrite(&input32, 4, 1, fnew);
    //memcpy(&input32, &fileData[0].buffer[20], 4);
    totalHeight = htonl(totalHeight);
    //Height
    fwrite(&totalHeight, 4, 1, fnew);
    //IHDR Data field Hight 之后剩下的
    memcpy(&input32, &fileData[0].buffer[24], 4);
    fwrite(&input32, 4, 1, fnew);
    memcpy(&input8, &fileData[0].buffer[28], 1);
    fwrite(&input8, 1, 1, fnew);

    //新PNG中读取IHDR的Type field & Data field
    //已知长度是17
    fseek(fnew, 12, 0);
    unsigned long int IHDR_crc_check_length = 17;
    unsigned char IHDR_crc_check[IHDR_crc_check_length];
    fread(IHDR_crc_check, 1, IHDR_crc_check_length, fnew);

    //得到新crc return
    U64 crc_IHDR = crc(IHDR_crc_check, IHDR_crc_check_length);
    U32 crc_IHDR1 = htonl(crc_IHDR);
    fseek(fnew, 29, 0);

    //IHDR CRC field
    fwrite(&crc_IHDR1, 4, 1, fnew);

    //IDAT
	U32 nIDATLength = htonl(totalCounter);
    fwrite(&nIDATLength, 4, 1, fnew);

    //IDAT Type field
    memcpy(&input32, &fileData[0].buffer[37], 4);
    fwrite(&input32, 4, 1, fnew);

    //IDAT Data field
    fwrite(total, totalCounter, 1, fnew);

    //新PNG中读取IDAT的Type field & Data field
    fseek(fnew, 37, 0);
    U8 IDAT_crc_check[4+totalCounter];
    fread(IDAT_crc_check, 1, 4+totalCounter, fnew);
    //得到新crc return
    U64 crc_IDAT = crc(IDAT_crc_check, 4+totalCounter);
    U32 crc_IDAT1 = htonl(crc_IDAT);

    //IDAT CRC field
    fwrite(&crc_IDAT1, 4, 1, fnew);    

    //IEND
    memcpy(&input32, &fileData[0].buffer[45+totalCounter], 4);
    fwrite(&input32, 4, 1, fnew);
    memcpy(&input32, &fileData[0].buffer[49+totalCounter], 4);
    fwrite(&input32, 4, 1, fnew);
    memcpy(&input32, &fileData[0].buffer[53+totalCounter], 4);
    fwrite(&input32, 4, 1, fnew);
  
    fclose(fnew);

    return 0;
}