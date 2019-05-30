#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>   /* for printf().  man 3 printf */
#include <stdlib.h>  /* for exit().    man 3 exit   */
#include <string.h>  /* for strcat().  man strcat   */

int checkIsDir(char* str_path, int flag){
    struct stat buf;
    lstat(str_path, &buf);

    if (S_ISREG(buf.st_mode)){
        flag = 0;
    }  
    if (S_ISDIR(buf.st_mode)){
        flag = 1;
    }
    return flag;  
}

void checkIsPNG(char* str_path){
    char value;
    char str[4];
    int count = 0;
    //open file
    FILE* f = fopen(str_path, "rb");
    //read file
    while ( fread(&value, sizeof(char), 1, f) ) {
        str[count] = value;
        count++;
        if(count == 4) break;
    }
    //close file
    fclose(f);

    //take 8 bytes to see if it is PNG file
    if(str[1] == 'P' && str[2] == 'N' && str[3] == 'G')
        printf("%s\n", str_path);
}

void checkHasSubdir(char* str_path){
    DIR *p_dir1;
    struct dirent *p_dirent1;
    int flag1 = 0;
    char new_path[1000];

    p_dir1 = opendir(str_path);
    while((p_dirent1 = readdir(p_dir1)) != NULL){
        char *str1 = p_dirent1->d_name;
        
        if(strcmp(str1, ".") == 0) continue;
        if(strcmp(str1, "..") == 0) continue;
        //if(*str1 == '.' || *str1 == '..') continue;
        strcpy(new_path, str_path);
        strcat(new_path, "/");
        strcat(new_path, str1);


        flag1 = checkIsDir(new_path, flag1); 
        if (flag1 == 1) 
            checkHasSubdir(new_path);
        else
            checkIsPNG(new_path);

        flag1 = 0;       
    }
    closedir(p_dir1);
}


int main(int argc, char *argv[]) {
    if (argc == 1) {
        fprintf(stderr, "Usage: %s <directory name>\n", argv[0]);
        exit(1);
    }

    checkHasSubdir(argv[1]);
    
    return 0;
}
