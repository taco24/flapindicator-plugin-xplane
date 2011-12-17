/*
 * settings.h
 *
 */

#ifndef SETTINGS_H_
#define SETTINGS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define bufsize 2048 /*A defined integer for our buffer size*/
#define SETTINGS_FILE "flapindicator.cfg";

int loadSettings();
int saveSettings();

#endif /* SETTINGS_H_ */

