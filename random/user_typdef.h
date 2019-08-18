#ifndef _USER_TYPDEF_H_
#define _USER_TYPDEF_H_ 

#include "edgex/edgex-base.h"
#include "edgex/edgex.h"
#include "edgex/device-mgmt.h"
#include "edgex/devsdk.h"

typedef char* string;

typedef struct repDiscovery
{
	char *devname;
	char *mac;
	char *profile;
	char *model;
} repDiscovery;

typedef struct sensorEvent
{
	char * name;
	uint64_t  origin;
	char *commandDevice;
	uint32_t size;
	string *value;
} sensorEvent;

typedef enum {
  CMD_PUSH = 'a',
  CMD_GET = 'b', 
  CMD_PUT = 'c',
  CMD_DISCOVERY,
  CMD_ADD_DEVICE,
  CMD_PROTOCOL
} user_cmd;

typedef struct edgex_cmdinfo
{
  char *name;
  bool isget;
  unsigned nreqs;
  edgex_device_commandrequest *reqs;
  edgex_propertyvalue **pvals;
  edgex_nvpairs **maps;
  struct edgex_cmdinfo *next;
} edgex_cmdinfo;

// struct edgex_device_config ;
// typedef struct edgex_device_config edgex_device_config;
// typedef void * (*edgex_device_autoevent_start_handler)
// (
//   void *impl,
//   const char *devname,
//   const edgex_protocols *protocols,
//   const char *resource_name,
//   uint32_t nreadings,
//   const edgex_device_commandrequest *requests,
//   uint64_t interval,
//   bool onChange
// );

// typedef void (*edgex_device_autoevent_stop_handler)
// (
//   void *impl,
//   void *handle
// );
// struct edgex_device_service
// {
//   const char *name;
//   const char *version;
//   void *userdata;
//   edgex_device_callbacks userfns;
//   edgex_device_autoevent_start_handler autoevstart;
//   edgex_device_autoevent_stop_handler autoevstop;
//   iot_logger_t *logger;
//   edgex_device_config config;
//   atomic_bool *stopconfig;
//   edgex_rest_server *daemon;
//   edgex_device_operatingstate opstate;
//   edgex_device_adminstate adminstate;
//   uint64_t starttime;

//   edgex_devmap_t *devices;
//   iot_threadpool_t *thpool;
//   iot_scheduler_t *scheduler;
//   pthread_mutex_t discolock;
// };

typedef struct user_push_event
{
    char *device_name;
    char *resource_name;
    int nreqs;
    edgex_device_commandresult *values;
} user_push_event;

typedef struct user_discovery
{
  char *device;
  char *MAC;
  char *devtype;
  char *profile;
} user_discovery;

typedef struct user_add_device
{
  char *MAC;
  char *device;
  char *addr;  
  edgex_device_adminstate adminState;
  edgex_device_operatingstate opState;    
} user_add_device;


#endif