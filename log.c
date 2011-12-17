/*
 * log.c
 */
#include "log.h"

FILE *pLogFile = NULL;
char *logFileName = LOG_FILE;
int logCounter = 0;

int openLog() {
	int rc = 0;
	pLogFile = fopen(logFileName, "w");
	if (pLogFile == NULL) {
		printf("log error: can't open file.\n");
		rc = -1;
	} else {
		printf("log info : File opened.\n");
	}
	return rc;
}

int closeLog() {
	int rc = 0;
	if (pLogFile != NULL) {
		fclose(pLogFile);
	}
	return rc;
}

int initLog() {
	int rc = 0;
	char logString[50];
	rc = openLog();
	if (rc == 0) {
		time_t t1;
		(void) time(&t1);
		srand((long) t1);
		struct tm *local;
		local = localtime(&t1);
		sprintf(logString, "Logdatei erstellt am: %s\n", asctime(local));
		fputs(logString, pLogFile);
		fflush(pLogFile);
	}
	return rc;
}

int writeLog(char *text) {
	char logString[50];
	int rc = 0;
	struct timeval tv;
	struct timezone tz;
	struct tm *tm;
	gettimeofday(&tv, &tz);
	tm = localtime(&tv.tv_sec);
	sprintf(logString, "%d:%02d:%02d %d", tm->tm_hour, tm->tm_min, tm->tm_sec,
			tv.tv_usec);
	logCounter++;
	char string[50];
	sprintf(string, " - (%d): ", logCounter);
	if (pLogFile != NULL) {
		fputs(logString, pLogFile);
		fputs(string, pLogFile);
		fputs(text, pLogFile);
		fflush(pLogFile);
		rc = 1;
	}
	return rc;
}
