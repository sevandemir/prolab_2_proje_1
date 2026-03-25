#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "textengine.h" 

#define maxFileCount 100

typedef struct fileInformation {
    char unitName[100];
    int isFolder;
} fileInfo;


extern fileInfo fileList[maxFileCount];
extern int fileCount;
extern int fileNumber;
extern char currentOpenedFile[100];
extern int startView;
extern int viewLimit; 

// Fonksiyonlar
void readFiles();
void loadFile(char *filename);
void fileCursorDown();
void fileCursorUp();
void saveFile(char *filename);

#endif