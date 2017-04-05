#ifndef _INI_H
#define _INI_H

#define INI_FILENAME "3ds-remote-linux.ini"

int ini_init();
void ini_exit();
int ini_getParameter(const char* param);

#endif