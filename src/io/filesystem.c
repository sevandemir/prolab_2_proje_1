#include "filesystem.h"
#include "textengine.h"
#include "UI.h"
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

// Gerçek Değişken Tanımları
fileInfo fileList[maxFileCount];
int fileCount = 0;
int fileNumber = 0;
char currentOpenedFile[100] = ""; 

void readFiles() {
    DIR *directory;
    struct dirent *dir;
    struct stat st;

    directory = opendir(".");
    fileCount = 0;

    if (directory) {
        while ((dir = readdir(directory)) != NULL && fileCount < maxFileCount) {
            // "." (mevcut klasör) gizle, ama ".." (üst klasör) kalsın ki geri çıkabilelim
            if (strcmp(dir->d_name, ".") == 0) {
                continue;
            }

            strcpy(fileList[fileCount].unitName, dir->d_name);

            if (stat(dir->d_name, &st) == 0) {
                fileList[fileCount].isFolder = S_ISDIR(st.st_mode);
            }
            
            fileCount++;
        }
        closedir(directory);
    }
}

void loadFile(char *filename) {
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) return;

    // Açılan dosyanın adını hafızaya al
    strcpy(currentOpenedFile, filename);

    clearEditor(); 
    
    char c;
    while ((c = fgetc(fp)) != EOF) {
        if (c == '\r') continue;
        if (c == '\n') {
            addnewline();
        } else {
            letterEntry(c); 
        }
    }
    fclose(fp);
    
    cursor = headline->dummynode;
    currentline = headline;
}

void saveFile(char *filename) {
    if (filename == NULL || strlen(filename) == 0) return;

    FILE *fp = fopen(filename, "w"); // "w" modu dosyayı baştan yaratır veya üstüne yazar
    if (fp == NULL) {
        printf("Dosya acilamadi!\n");
        return;
    }

    line_x *templine = headline;
    while (templine != NULL) {
        node_x *tempnode = templine->dummynode->next; // Dummy node'u atla
        
        while (tempnode != NULL) {
            fputc(tempnode->letter, fp);
            tempnode = tempnode->next;
        }

        // Eğer bir sonraki satır varsa, satır sonuna newline ekle
        if (templine->nextline != NULL) {
            fputc('\n', fp);
        }
        
        templine = templine->nextline;
    }

    fclose(fp);
}

//File Browser Arrow Functions
/*

 void fileCursorDown() {

    if(fileNumber < fileCount - 1){
        fileNumber++;

        if (fileNumber >= startView + viewLimit) {
            startView++;
        }
    }
    else{
        fileNumber = 0; 
        startView=0;
    }
}

void fileCursorUp() {

    if (fileNumber > 0) {
        fileNumber--;
        if (fileNumber < startView) {
            startView--;
        }
    } 
    else{
        fileNumber = fileCount - 1;
        startView = fileCount - viewLimit;
        if(startView < 0) startView = 0; // Az dosya varsa 0'da kalsın
    }
}

*/