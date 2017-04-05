#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "global.h"
#include "ini.h"

FILE* iniFp;

int ini_init()
{
    iniFp = fopen(INI_FILENAME, "r");
    if (!iniFp)
    {
        printLog(1, "Couldn't open \"%s\". Creating new...", INI_FILENAME);
        
        iniFp = fopen(INI_FILENAME, "w+");
        if (!iniFp)
        {
            printLog(0, "FATAL: Could not create \"%s\"\n", INI_FILENAME);
            return 1;
        }
        
        fprintf(iniFp, "port=55550\n");
        fprintf(iniFp, "serveraddr=192.168.1.2\n");
        fprintf(iniFp, "loggingVerbosity=1\n");
    }
    
    return 0;
}

void ini_exit()
{
    fclose(iniFp);
}

int ini_getParameter(const char* param)
{
    if (!iniFp)
    {
        printLog(1, "Error: iniFp is NULL\n");
        return 0;
    }
    if (strlen(param) > 255)
        return 0;

    char* paramSearch;
    
    paramSearch = (char*)malloc(strlen(param)+2);
    if (!paramSearch)
        return 0;
        
    strcpy(paramSearch, param);
    strcat(paramSearch, "=");
    
    fseek(iniFp, 0, SEEK_SET);
    while (!feof(iniFp))
    {
        char iniLine[256];
        fgets(iniLine, 256, iniFp);
        
        if (!strncmp(iniLine, paramSearch, strlen(paramSearch)))
        {
            int i;
            sscanf(iniLine+strlen(paramSearch), "%d", &i);
            return i;
        }
    }
    
    printLog(1, "Error: Could not find \"%s\" in .ini file\n", param);
    return 0;
}