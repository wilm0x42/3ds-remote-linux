#ifndef _INI_H
#define _INI_H

#define INI_FILENAME "3ds/3ds-remote-linux/3ds-remote-linux.ini"

int ini_init();
void ini_exit();

int ini_getInt(const char* param);

//ini_getString:
// Will return NULL if param cannot be found in the INI file
// If param is found in the INI file, the returned string will
// need to be freed with free(returnedString);
char* ini_getString(const char* param);

#endif