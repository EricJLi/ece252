#include <stdio.h>    /* for printf(), perror()...   */
#include <stdlib.h>   /* for malloc()                */
#include <errno.h>    /* for errno                   */
#include "lab_png.h"  /* simple PNG data structures  */
#include "crc.h"      /* for crc()                   */
#include "zutil.h"    /* for mem_def() and mem_inf() */

int main(int argc, char **argv) {
    if (argc == 1) {
        fprintf(stderr, "Usage: %s <directory name>\n", argv[0]);
        exit(1);
    }

    U32 crc_val = 0;      /* CRC value                                     */
    int ret = 0;          /* return value for various routines             */
    U8 buffer[500000];
    U8 buffer_def[200000];
    U32 totalHeight = 0;
    U64 bufDefCounter = 0;
    U64 bufCounter = 0;

    for (int x = 1; x < argc; x++) {
        U8 inputLength[4];
        FILE* f = fopen(argv[x], "rb");

        //Read in file height
        fseek(f, 20, SEEK_SET);
        U32 height = 0;
        fread(&height, 4, 1, f);
        totalHeight += ntohl(height);

        //Read IDAT data length
        fseek(f, 33, SEEK_SET);
        fread(inputLength, sizeof(inputLength), 1, f);
        U64 sourceLength = inputLength[0] << 24 | inputLength[1] << 16 | inputLength[2] << 8 | inputLength[3];
        printf("%s : %lu\n", argv[x], sourceLength);

        //Read IDAT data
        fseek(f, 41, SEEK_SET);
        U8 sourceBuffer[sourceLength];
        fread(sourceBuffer, sizeof(sourceBuffer), 1, f);
        U8 destinationBuffer[sourceLength * 500];
        U64 destinationLength;

        //Inflate the data (decompress)
        ret = mem_inf(destinationBuffer, &destinationLength, sourceBuffer, sourceLength);
        if (ret == 0) { /* success */
            printf("original len, destinationLength = %lu, sourceLength = %lu\n", \
                destinationLength, sourceLength);
        } else { /* failure */
            fprintf(stderr,"mem_def failed. ret = %d.\n", ret);
        }

        //append the data to each other
        for (int y = 0; y < destinationLength; y++) {
            buffer[y + bufCounter] = destinationBuffer[y];
        }
        bufCounter += destinationLength;

        //close file
        fclose(f);
    }

    printf("Total Height %u\n", totalHeight);

    //Deflate the data (compress)
    ret = mem_def(buffer_def, &bufDefCounter, buffer, bufCounter, Z_DEFAULT_COMPRESSION);
    if (ret == 0) { /* success */
        printf("bufCounter = %lu, bufDefCounter = %lu\n", bufCounter, bufDefCounter);
    } else { /* failure */
        fprintf(stderr,"mem_def failed. ret = %d.\n", ret);
        return ret;
    }
    
    U32 input32 = 0;
    U64 input64 = 0;
    U8 input8 = 0;
    FILE* fout = fopen("result.png", "w+");
    FILE* fin = fopen(argv[1], "rb");

    //Read/Write header
    fread(&input64, 8, 1, fin);
    fwrite(&input64, 8, 1, fout);

    //Read/Write IHDR
    fread(&input64, 8, 1, fin);
    fwrite(&input64, 8, 1, fout);
    fread(&input32, 4, 1, fin);
    fwrite(&input32, 4, 1, fout);
    fread(&input32, 4, 1, fin);
    totalHeight = htonl(totalHeight);
    //Height
    fwrite(&totalHeight, 4, 1, fout);
    fread(&input32, 4, 1, fin);
    fwrite(&input32, 4, 1, fout);
    fread(&input8, 1, 1, fin);
    fwrite(&input8, 1, 1, fout);
    fread(&input32, 4, 1, fin);
    fwrite(&input32, 4, 1, fout);

    //Read/Write IDAT
    U32 finLength = 0;
    fread(&finLength, 4, 1, fin);
    //bufDefCounter
    U32 NbufDefCounter = (U32)htonl(bufDefCounter);
    fwrite(&NbufDefCounter, 4, 1, fout);
    U8 bufferType[4];
    fread(&bufferType, sizeof(bufferType), 1, fin);
    //New Buffer
    U8 newBuffer[bufDefCounter + 4];
    for (int j = 0; j < 4; j++) {
        newBuffer[j] = bufferType[j];
    }
    for (int m = 0; m < bufDefCounter; m++) {
        newBuffer[m+4] = buffer_def[m];
    }
    U64 newBufferLength = bufDefCounter + 4;
    //Calculate new crc
    crc_val = crc(newBuffer, newBufferLength); // down cast the return val to U32
    printf("crc_val = %u\n", crc_val);
    fwrite(&bufferType, 4, 1, fout);
    finLength = ntohl(finLength);
    U8 readBuffer[finLength];
    fread(&readBuffer, 1, finLength, fin);

    //buffer_def
    fwrite(&buffer_def, 1, bufDefCounter, fout);
    fread(&input32, 4, 1, fin);
    //CRC Val
    crc_val = htonl(crc_val);
    fwrite(&crc_val, 4, 1, fout);
    
    //Read/Write IEND
    fread(&input32, 4, 1, fin);
    fwrite(&input32, 4, 1, fout);
    fread(&input32, 4, 1, fin);
    fwrite(&input32, 4, 1, fout);
    fread(&input32, 4, 1, fin);
    fwrite(&input32, 4, 1, fout);

    fclose(fin);
    fclose(fout);

    return 0;
}