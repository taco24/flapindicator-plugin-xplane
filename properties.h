#ifndef _PROPERTIES_H_
#define _PROPERTIES_H_


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define MAXDATASIZE   1024
#define MAXPROPSIZE   256
#define MAXVALUESIZE  256
#define DELIMITER     "="
#define DELIMITERSIZE 1
#define TRUE          1
#define FALSE         0
#define ISDEBUG       FALSE 

typedef struct list {
    char *property;
    char *value;
    struct list *next;
} PROPERTY;


void loadPropertyFile( char *filename );
void loadProperties();
void listProperties (void);
void cleanup (void);    
void addProperty (char *property, char *value);
void getProperty (char *property, char *value);
int getPropertyCount();
int searchString (const char *search, const char *string, int offset);
static char *trim( char *s);

#endif
