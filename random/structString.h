#include <stdio.h> 
#include<string.h>
#include <stdlib.h>
typedef char* string;

typedef struct sensorEvent
{
	char * name;
	uint64_t  origin;
	char *commandDevice;
	uint32_t size;
	string *value;
} sensorEvent;

char * takeString(char **str);
sensorEvent *takeEvent(char * str);
string *takeValue(char *str,int size,sensorEvent *ev);
void freeEvent(sensorEvent *ev);

char * takeString(char **str)
{
	char *find;
	find=strstr(*str,"#");
	int n =find-*str;
	char *value = strndup(*str,n);
	*str = *str+n+1;
	return value;
}
sensorEvent *takeEvent(char * str)
{
	sensorEvent *ev = (sensorEvent*) malloc(sizeof(sensorEvent));
	char *newStr=str;

	ev->name=takeString(&newStr);
	ev->origin=atoi(takeString(&newStr));
	ev->commandDevice=takeString(&newStr);
	ev->size= atoi(takeString(&newStr));

	ev->value= takeValue(str,ev->size,ev);
}

string *takeValue(char *str,int size,sensorEvent *ev)
{
	ev->value =(string*)malloc(ev->size *sizeof(string));
	char *find;
	find =strstr(str,"value#");
	if(find==NULL) return NULL;
	else
	{
		int i=0;
		find=find + strlen("value#");

		for(i=0;i<size;i++)
		{
			int count=0;
			//ev->value[i] =(char *) malloc(sizeof(char));
			while(find[count]!='#')
			{
				count++;
			}
			ev->value[i]=strndup(find,count);
			count++;
			find = find+count;
		}
		//printf("%s\n",ev->value[2]);

	}
	return ev->value;
}
void freeEvent(sensorEvent *ev)
{
	if(ev->size!=0)
	{
		for(int i=0;i<ev->size;i++)
		{
			free(ev->value[i]);
		}
	}
	if(ev->name!=NULL)
	{
		free(ev->name);
	}
	if(ev->name!=NULL)
	{
		free(ev->commandDevice);
	}
	if(ev!=NULL)
	{
		free(ev);
	}
}
