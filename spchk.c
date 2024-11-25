#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ctype.h>
#include "arraylist.h"
#define DIRPATH_ALLOCATED 0



//Recursively traverses a directory and opens all of the text files it finds and records their file identifiers
void dirtraverse (arraylist_t* textIDs, char** textNames, int texti, DIR *dirhandle, char* newpath){
   
    struct dirent *sd = readdir(dirhandle);
    int mallocdir = 0;
    char *dirpath;
        while(sd != NULL){
            if(strcmp(sd->d_name, ".") != 0 && strcmp(sd->d_name, "..") != 0){
                char *a = strrchr(sd->d_name,46);
                if(sd->d_type == DT_REG && strcmp(a, ".txt") == 0){
                    char *regpath = malloc(sizeof(newpath) + sizeof(char) + sizeof(sd->d_name));
                    strcat(regpath, newpath);
                    strcat(regpath, "/");
                    strcat(regpath, sd->d_name);
                    int r = open(regpath, O_RDONLY);
                    al_push(textIDs, r);
                    textNames[al_length(textIDs)-1] = malloc(strlen(regpath));
                    strcpy(textNames[al_length(textIDs)-1], regpath);
                }
                else if (sd->d_type == DT_DIR){
                    if(mallocdir>0) free(dirpath);
                    dirpath = malloc((int)((int) strlen(newpath) + sizeof(char) + (int) strlen(sd->d_name)));
                    mallocdir ++;
                    strcat(dirpath, newpath);
                    strcat(dirpath, "/");
                    strcat(dirpath, sd->d_name);
                    DIR *newhandle = opendir(dirpath);
                    dirtraverse(textIDs, textNames, texti, newhandle, dirpath);
                    free(dirpath);
                    mallocdir --;
                   
                }
                else{
                    printf("error\n");
                }
               
            }
            sd = readdir(dirhandle);
        }

}

//A binary search function to be used on the array of valid dictionary words
int binary(int low, int high, char *strip2, char **dictarray){
    int found = 0;
    int c=0;
    while(low<=high && found == 0){
       
        for(int i = 0; i<strlen(strip2); i++){
            if(isalpha(strip2[i])) c++;
        }
        if(c==0) break;
       
        int mid = low + ((high-low) / 2);
        int comp = strcmp(strip2, dictarray[mid]);
        if (comp==0) {
            found = 1;
            return 1;
        }
        else if(comp<0){
            high = mid-1;
        }
        else{
            low= mid+1;
        }
    }
    return 0;
}


static int cmpfunc(const void* a, const void* b)
{
    return strcmp(*(const char**)a, *(const char**)b);
}


int main(int argc, char ** argv){
    //Records the names and file identifiers of all text files
    int dictfd = open(argv[1], O_RDONLY);
    int texti = 0;
    int i = 2;
    arraylist_t textIDs;
    char *pt;
    char** textNames = malloc(sizeof(pt)*20);
    al_init(&textIDs, 1);
    int IDIndex = 0;
    while(argv[i] != NULL){
        struct stat statstruct;
        int r = open(argv[i], O_RDONLY);
        stat(argv[i], &statstruct);
        if(S_ISREG(statstruct.st_mode)){
            al_push(&textIDs, r);
            texti++;
            textNames[al_length(&textIDs)-1] = malloc(strlen(argv[i]));
            strcpy(textNames[al_length(&textIDs)-1], argv[i]);
           
        }
        else if (S_ISDIR(statstruct.st_mode)){
            DIR *dirhandle = opendir(argv[i]);
            dirtraverse(&textIDs, textNames, texti, dirhandle, argv[i]);
        }
        else{
            printf("err\n");
        }
        i++;
    }
    //FD's of each file now loaded to textIDs arraylist
    //FD of dictionary stored in dictfd

    //Create array of dictionary words
    int buflength = 50;
    char *buf = malloc(buflength);
    int bytes;
    int j=0;
    char *p;
    char** dictarray = malloc(sizeof(p) * 20);
    int dictindex = 0;
    int dictsize = 20;
    int pos=0;
    while((bytes = read(dictfd, buf+pos, buflength-pos))>0){
        int maxline;
        for(int i=0; i<buflength; i++){
            if (buf[i] == '\n'){
                maxline = i;
            }
        }
        buf[maxline] = '\0';
        char* token = strtok(buf, "\n");
        //for each token add a regular, initial and caps version.
        do{
            if(dictindex == dictsize){
                dictarray = realloc(dictarray, sizeof(p) * (dictsize+20));
                dictsize+=20;
            }
            dictarray[dictindex] = malloc(strlen(token));
            strcpy(dictarray[dictindex], token);
            dictindex++;
            if(dictindex == dictsize){
                dictarray = realloc(dictarray, sizeof(p) * (dictsize+20));
                dictsize+=20;
            }
            char *initial = token;
            dictarray[dictindex] = malloc(strlen(token));
            *(initial) = toupper((unsigned char) *(initial));
            strcpy(dictarray[dictindex], initial);
            dictindex++;
            if(dictindex == dictsize){
                dictarray = realloc(dictarray, sizeof(p) * (dictsize+20));
                dictsize+=20;
            }
            char *upper = token;
            dictarray[dictindex] = malloc(strlen(token));
            for(int i=0; i<strlen(upper); i++){
                *(upper+i) = toupper((unsigned char) *(upper+i));
            }
            strcpy(dictarray[dictindex], upper);
            dictindex++;
            token = strtok(NULL, "\n");
        } while(token != NULL);
        memmove(buf, buf + maxline + 1, buflength - (maxline+1));
        pos = buflength - (maxline+1);
    }
    qsort(dictarray, dictindex, sizeof(char*), cmpfunc);


    //For each text file create an array of file name/position annotated words and check if they are present in the dictionary array.
    //Prints invalid words
    free(buf);
   
    int mike = 0;
    int textLength = 0;
    for(int j=0; textNames[j] != NULL; j++){
        textLength++;
    }
    int textIdLength = al_length(&textIDs);
    for(int i=0; i<textIdLength; i++){
        int textfd;
        int x = al_pop(&textIDs, &textfd);
        int buflength = 50;
        char *textbuf = malloc(buflength);
        int bytes;
        int j=0;
        int pos=0;
        char** textarray = malloc(sizeof(p) * 20);
        int textindex = 0;
        int textsize = 20;
        int line = 1;
        int col = 1;
        int g=0;


        char *pathname = malloc(strlen(textNames[textLength - 1 - i]));
        strcpy(pathname, textNames[textLength - 1 - i]);
        while((bytes = read(textfd, textbuf+pos, buflength-pos))>0 ){
            int maxline = 0;
            int* rows = malloc(sizeof(int)*50);
            int *cols = malloc(sizeof(int)*50);
            int tupleindex = 0;
            rows[0] = line;
            cols[0] = col;
            tupleindex++;
           
            for(int i=0; i<50; i++){
                if (textbuf[i] == '\n'){
                    maxline = i;
                }
                else if (textbuf[i] == ' '){
                    maxline = i;
                }
            }
           
            for(int i=0; i<=maxline; i++){
                if (textbuf[i] == '\n'){
                    line++;
                    col=1;
                    rows[tupleindex] = line;
                    cols[tupleindex] = col;
                    tupleindex++;
                }
                else if (textbuf[i] == ' '){
                    col++;
                    rows[tupleindex] = line;
                    cols[tupleindex] = col;
                    tupleindex++;
                }
                else{
                    col++;
                }
            }
            if(maxline==0){
               
            }
            else if(bytes != buflength-pos){
                textbuf[g+bytes] = '\0';
            }
            else{
                textbuf[maxline] = '\0';
            }
           
            char delimit[] = " \n";
            char* token = strtok(textbuf, delimit);
            int z = 0;
            do{
                if(textindex == textsize){
                    textarray = realloc(textarray, sizeof(p) * (textsize+20));
                    textsize+=20;
                }
                char *row = malloc(sizeof(rows[z]));
                char *column = malloc(sizeof(cols[z]));
                sprintf(row, "%d", rows[z]);
                sprintf(column, "%d", cols[z]);
                textarray[textindex] = malloc(sizeof(token) + sizeof(row) + sizeof(col) + strlen(pathname) + 6);
                char *fullstring = malloc(sizeof(token) + sizeof(row) + sizeof(col) + strlen(pathname) + 6);
                strcat(fullstring, pathname);
                strcat(fullstring, " ");
                strcat(fullstring, "(");
                strcat(fullstring, row);
                strcat(fullstring, ",");
                strcat(fullstring, column);
                strcat(fullstring, "): ");
                strcat(fullstring, token);
                strcpy(textarray[textindex], fullstring);
                z++;
                free(row);
                free(column);
                textindex++;
                free(fullstring);
                token = strtok(NULL, delimit);
            } while(token != NULL);
            memmove(textbuf, textbuf + maxline + 1, buflength - (maxline+1));
            pos = buflength - (maxline+1);
            free(rows);
            free(cols);
            g= buflength - (maxline+1);
                       
        }
               
        for(int i = 0; i<textindex; i++){
            char delim[] = " ([{\"\'";
            int low = 0;
            int high = dictindex-1;
            int found = 0;
            int hyphen = 0;
            int allvalid = 1;
            char *token = malloc(strlen(textarray[i]));
            strcpy(token, textarray[i]);
            char *strip0 = strtok(token, ": ");
            strip0 = strtok(NULL, ": ");
            strip0 = strtok(NULL, ": ");
            char *strip = strtok(strip0, delim);
            char* strip2 = malloc(strlen(strip));
            strcpy(strip2, strip);
            for(int i=0; i<strlen(strip2); i++){
                if(strip2[i] == '.' || strip2[i] == ',' || strip2[i] == '!' || strip2[i] == '?' || strip2[i]==' '){
                    strip2[i] = '\0';
                    i+=(strlen(strip2));
                }
            }
            for(int i=0; i<strlen(strip2); i++){
                if(strip2[i] == '-'){
                    hyphen++;
                }
            }
            if(hyphen==0){
                low=0;
                high=dictindex-1;
                found = binary(low, high, strip2, dictarray);
            }
            else{
               
                char* partial = strtok(strip2, "-");
                for(int i=0; i<strlen(strip2); i++){
                    if(strip2[i]=='\0'){
                        printf("%d\n", i);
                    }
                }
                do{
                    if(binary(low, high, partial, dictarray) == 0){
                        allvalid--;
                    }
                    partial = strtok(NULL, "-");
                } while(partial != NULL);

                if(allvalid !=1){
                    found = 0;
                }
                else{
                    found = 1;
                }
               
            }
            if(found==0){
                printf("%s\n", textarray[i]);
            }
        }


    }
   
    return 0;
}

