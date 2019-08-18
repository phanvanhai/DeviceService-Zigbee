#include <stdio.h> 
#include <string.h>
#include <stdlib.h>
#include "user_typdef.h"

repDiscovery * takeDiscovery(char *str);
void freeDiscovery(repDiscovery *d);
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
	char *newStr = str;
	char *str_size = NULL;

	ev->name=takeString(&newStr);
	ev->origin=atoi(takeString(&newStr));
	ev->commandDevice=takeString(&newStr);
	str_size = takeString(&newStr);
	ev->size= atoi(str_size);
	if( str_size != NULL )
		free(str_size);

	ev->value= takeValue(str,ev->size,ev);
	return ev;
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


// discovery;
repDiscovery * takeDiscovery(char *str)
{
	repDiscovery *dis=(repDiscovery*)malloc(sizeof(repDiscovery));
	char *newStr=str;
	dis->devname =takeString(&newStr);
	dis->mac =takeString(&newStr);
	dis->profile=takeString(&newStr);
	dis->model =takeString(&newStr);
	return dis;
}
void freeDiscovery(repDiscovery *d)
{
	if(d->devname!=NULL)
		free(d->devname);
	if(d->mac!=NULL)
		free(d->mac);
	if(d->profile!=NULL)
		free(d->profile);
	if(d->model!=NULL)
		free(d->model);
	if(d!=NULL)
		free(d);
}
