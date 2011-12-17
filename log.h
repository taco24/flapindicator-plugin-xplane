/*
 * log.h
 */

#ifndef LOG_H_
#define LOG_H_

#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>

#define LOG_FILE "colomboard.log";

int initLog();
int writeLog(char *text);
int closeLog();

#endif /* LOG_H_ */
