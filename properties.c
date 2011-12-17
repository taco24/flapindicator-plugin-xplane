#include "properties.h"

char fline[MAXDATASIZE];	

int propertyCount =0;
PROPERTY *node= NULL;
FILE *fp;

void loadProperties()
{
	char property[MAXPROPSIZE]={""};
	char value[MAXVALUESIZE]={""};
	int index=-1;
	int lineNumber=1;
										
    while(fgets(fline, MAXDATASIZE, fp) != NULL)
    {
	   if (ISDEBUG) printf("loadProperties() : fline = [%s]\n", fline);	
	   index = searchString (DELIMITER, fline, 0);
	   if (ISDEBUG) printf("loadProperties() : index = [%d]\n", index);
	   if (index<=0) {
	       if (ISDEBUG) 
			   printf("loadProperties() : Unusable line %d\n",lineNumber);
		   continue;	
	   } else {	
		 strncpy(property,&fline[0],index);
		 trim(property);				
		 strncpy(&value[0],&fline[index + DELIMITERSIZE],(strlen(fline)-(index+DELIMITERSIZE)));
		 trim(value);
	   }
       if (ISDEBUG) printf("loadProperties() : property = [%s]\n", property);
       if (ISDEBUG) printf("loadProperties() : value = [%s]\n", value);
	   addProperty(property, value);
	   strncpy(property,"",index);
	   strncpy(&value[0],"",(strlen(fline)-index));	
	   lineNumber++;
    }
}

void loadPropertyFile( char * filename )
{
    if(!(fp = fopen(filename, "r"))) {
        printf("Error: Could not open file: %s\n", filename);
        exit(1);
    }
}

void addProperty (char* property, char* value) 
{
    PROPERTY *new_node;
    new_node= (PROPERTY *)malloc (sizeof (PROPERTY));
    if (new_node == NULL) {				
        printf ("Out of memory!\n");	
        exit (-1);
    }
	if (ISDEBUG) 
		printf ("\naddProperty(): Added new property: %s = %s", property,value);
	new_node->property = strdup(property);
	new_node->value=strdup(value);

    new_node->next= node;
    node= new_node;
	propertyCount++;
}    

int getPropertyCount()
{
	return propertyCount;
}

void cleanup (void)
{
    PROPERTY *del_ptr;
    while (node != NULL) {	
       del_ptr= node;
       node= node->next;
       free(del_ptr);
    }
	if (ISDEBUG) 
		printf ("\n\ncleanup(): Done freeing up memory from the list. \n");
}

void getProperty (char* property, char* value)
{
    PROPERTY *prev_ptr;
											
    if (node == NULL) {
        if (ISDEBUG) 
			printf ("No properties have been loaded yet.\n");
        return;
    }
											
    prev_ptr= node;
    while (prev_ptr != NULL) {
		if (strcmp(prev_ptr->property, property) == 0) {
			if (ISDEBUG) 
				printf ("getProperty(): value = %s \n",prev_ptr->value);
			strcpy(value,prev_ptr->value);
			if (ISDEBUG) 
				printf ("getProperty(): Property %s = %s \n",property,value);
            return;
        }									
        prev_ptr= prev_ptr->next;	
    }
    if (ISDEBUG) 
		printf ("\nProperty [%s] not found.\n", property);
}

void listProperties (void)
{
    PROPERTY *tmp_ptr;
    if (ISDEBUG) printf ("\nAll properties in property file:");
    tmp_ptr= node;
    while (tmp_ptr != NULL) {
		printf ("\n%s : %s ",tmp_ptr->property, tmp_ptr->value);
        tmp_ptr= tmp_ptr->next;
    }
}


int searchString (const char *search, const char *string, int offset)
{
    int stlen=strlen(string);	
    int srlen=strlen(search);
    int counter=0;
    int sublcv=0;
    int start=-1;	

    if(srlen>stlen) {
        return(-2);
    }

    for(counter=offset;counter<=(stlen-srlen); counter++) {

        if(string[counter]==search[sublcv]) {
            while(sublcv<=srlen)  {

                if(search[sublcv]==string[sublcv+counter]) {
                    sublcv++;
                } else {
                    sublcv=srlen+1;
                }
                if(sublcv==srlen)  {
                    start=counter;
                    counter=(stlen-srlen)+1; 
                }
            }
        }
        sublcv=0;
    }
    return(start);
}

static char *trim( char *daddy)
{
	char *input, *output;
	if (daddy) {
		for (input = output = daddy; *input; ) {
			while (*input && (isspace (*input)))
				input++;
			if (*input && (output != daddy))
				*(output++) = ' ';		
			while (*input && (!isspace (*input)))
				*(output++) = *(input++);
		}
			*output=0;
	}
	return (daddy);
}

