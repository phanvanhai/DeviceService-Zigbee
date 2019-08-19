/*
 * Copyright (c) 2018-2019
 * IoTech Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include "edgex/devsdk.h"
#include "edgex/edgex.h"
#include "edgex/eventgen.h"
#include "edgex/registry.h"
#include "iot/thread.h"

#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include "user.h"

#include "user_queue.h"

int serial_port;
uint16_t address_zigbee = 4689;
char *name_master = "RandomDevice1";

#define ERR_CHECK(x) if (x.code) { fprintf (stderr, "Error: %d: %s\n", x.code, x.reason); edgex_device_service_free (service); free (impl); return x.code; }
edgex_device_service *service;
char *svcname;
char *regURL;
char *profile;

pthread_t ThreadPush;
pthread_t ThreadTX;
pthread_t ThreadRX;
Queue *QueuePush;
Queue *QueueTX;
Queue *QueueRX;
Queue *QueueGetHandler;
Queue *QueueDiscovery;

void *RX(void *t)
{
  char test[255];
  while(1)
  {
    int n = user_serial_read(serial_port, test, sizeof(test));
    if( n > 0)
    {      
      int cmd = test[0];
      printf("\n---------------> Serial RX:\n\t%s\n",test);
      switch (cmd)
      {
      case CMD_PUSH:
        // AddNode(&QueuePush, test+2, &mutex_Push);
        enQueue(QueuePush, test+2);
        break;

      case CMD_GET:
        enQueue(QueueGetHandler, test+2);
        break;
      
      case CMD_DISCOVERY:        
        enQueue(QueueDiscovery, test+2);
        break;

      default:
        break;
      }
    }
  }
}

void *TX(void *t)
{
  char *str_tx;
  while(1)
  {    
    if( NULL != (str_tx = deQueue(QueueTX)) )
    {
      printf("\n---------------> Serial TX:\n\t%s\n",str_tx);    
      user_serial_write(serial_port, str_tx, strlen(str_tx));      
      free(str_tx);      
    }
  }
}

void *PushEvent(void *t)
{  
  char *str_push;
  while(1)
  {            
    if( NULL != (str_push = deQueue(QueuePush)) )
    {                      
      printf("\n---------------> Push\n");
      user_push_event *push_event = user_push_event_read(service, str_push);    
      free(str_push);
      if( push_event != NULL)
      {
        edgex_device_post_readings (service, push_event->device_name ,push_event->resource_name, push_event->values);        
        user_push_event_free(push_event);     
      }
    }            
  }  
}


static bool user_add_new_device(char *input_json)
{
  const char *profile_name;  
  edgex_error err;
  char addr_str[4];
  sprintf(addr_str, "%u", address_zigbee); // so nguyen ko dau
  
  repDiscovery *rep = takeDiscovery(input_json);
  uint32_t lengRepDevice = 1+4+strlen(rep->mac)+strlen(rep->profile)+strlen(rep->model) +5;
  // cmd +add_str +MAC +profile +model +5 (#)
  char * repDevice = malloc(lengRepDevice);
  sprintf(repDevice,"%c#%s#%s#%s#%s#",CMD_ADD_DEVICE,rep->devname,rep->mac,addr_str,addr_str);
  

  profile_name = rep->profile;
  
  edgex_protocols *newprot = malloc (sizeof (edgex_protocols));
  newprot->name = strdup("zigbee");
  edgex_nvpairs *nv = malloc(sizeof(edgex_nvpairs));
  nv->name = strdup("addresss");
  nv->value = strdup(addr_str);
  nv->next = NULL;
  newprot->properties = nv;
  newprot->next = NULL;

  edgex_strings *labels = (edgex_strings *) malloc (sizeof (edgex_strings));
  labels->str = strdup("new device");
  labels->next = NULL;

  char * id_dev =  edgex_device_add_device (service, addr_str, NULL, labels, profile_name, newprot, NULL, &err );
  
  edgex_strings_free(labels);
  
  if( id_dev != NULL )
  {
    printf("-------------------------------> \n\tda them 1 thiet bi ID =%s\n", id_dev);
    printf("-------------%s\n",repDevice);    
    enQueue(QueueTX, repDevice);
    free(id_dev);
    freeDiscovery(rep);
    free(repDevice);
    return true;
  }
  else
  {
    printf("-------------------------------> \n\tKHONG them dc 1 thiet bi\n");
    freeDiscovery(rep);
    free(repDevice);
    return false;
  }
  
}

static void random_discover (void *impl) {
  printf("--------------------------------------> discovery\n");  
  int count = 5; // 10s
  char *str_discovery;
  while(count-- > 0)
  {    
    enQueue(QueueTX, "d#newdevice#");
    sleep(2);        
    if( NULL != (str_discovery = deQueue(QueueDiscovery)) )
    {
      bool result = user_add_new_device(str_discovery);
      free(str_discovery);
      if( result )
        printf("\t\tokkkkkkkkkkkk\n");
      else
      {
        printf("\t\tfalseeeeeeeeeeee\n");
      }      
    }        
  }   
}

typedef struct random_driver
{
  iot_logger_t * lc;
  bool state_flag;
  pthread_mutex_t mutex;
} random_driver;

static bool random_init
(
  void *impl,
  struct iot_logger_t *lc,
  const edgex_nvpairs *config
)
{
  random_driver *driver = (random_driver *) impl;
  driver->lc = lc;
  driver->state_flag=false;
  pthread_mutex_init (&driver->mutex, NULL);
  iot_log_debug(driver->lc,"Init");
/*
  // edgex_error *err;
  // edgex_nvpairs *c = edgex_device_getConfig (service);  
  // if( c != NULL)
  // {    
  //   uint16_t current_addr_zigbee = get_nv_config_uint16(driver->lc, c, "Driver/CurrentAddrZigbee", err);
  //   printf("\n-------------------> Driver/CurrentAddrZigbee: GET = %d <----------------------\n", current_addr_zigbee);  
  //   edgex_registry * registry = edgex_registry_get_registry (driver->lc, NULL, regURL);
  //   if( registry != NULL )
  //   {
  //     edgex_nvpairs *c_update = (edgex_nvpairs *) malloc(sizeof(edgex_nvpairs));
  //     char * update_value = malloc(32);
  //     current_addr_zigbee += 5;
  //     sprintf(update_value, "%d", current_addr_zigbee);

  //     c_update->name = strdup(c->name);
  //     c_update->value = update_value;
  //     c_update->next = NULL;
  //     edgex_registry_put_config (registry, svcname, profile, c_update, err);
  //     printf("\n-------------------> Driver/CurrentAddrZigbee: UPDATE = %d <----------------------\n", current_addr_zigbee);  
  //     if(err->reason != NULL)
  //       printf("\n loi put config:%s\n\n", err->reason);
  //     edgex_registry_free (registry);      
  //     edgex_nvpairs_free(c_update);
  //   }
  //   printf("\n-------------------> Driver/CurrentAddrZigbee = %d<------------------------\n", current_addr_zigbee);
  //   edgex_nvpairs_free (c);
  // }  
 */   
  return true;
}

static bool random_get_handler
(
  void *impl,
  const char *devname,
  const edgex_protocols *protocols,
  uint32_t nreadings,
  const edgex_device_commandrequest *requests,
  edgex_device_commandresult *readings
)
{
  bool result;
  
  char *str_request = user_gethandler_write( devname, nreadings, requests);  // struct ->string-json
  if( str_request == NULL )
  {
    return false;
  }
  printf("\n-----------------> GET send command:\n\t%s\n", str_request);         
  enQueue(QueueTX,str_request);  
  free(str_request);  

  result = false;  
  int count = 5000;     // 3s
  
  char *str_get;
  while(count-- > 0)
  {
    usleep(1000);
    lock_queue(QueueGetHandler);
    str_get = readQNode(QueueGetHandler);
    if( NULL != str_get )
    {            
      result = user_repget_event_read(readings,devname, requests, nreadings, str_get);            
      free(str_get);            
      if( result )
      { 
        clearQNode(QueueGetHandler);
        unlock_queue(QueueGetHandler);
        printf("\nda nhan phan hoi cua :%s\n", devname);
        return true;
      }     
    }
    unlock_queue(QueueGetHandler);      
  }
  
//   // result = false  
//   // update opState
   return false;
}

static bool random_put_handler
(
  void *impl,
  const char *devname,
  const edgex_protocols *protocols,
  uint32_t nvalues,
  const edgex_device_commandrequest *requests,
  const edgex_device_commandresult *values
)
{
  random_driver *driver = (random_driver *) impl;
  bool result = true;
  char *str_request = user_puthandler_write(devname, nvalues, requests, values);
  if( str_request == NULL )
  {
    return false;
  }
  printf("\n---------------> PUT send command:\n\t%s\n", str_request);
  

  enQueue(QueueTX, str_request);
  printf("sau put sen\n ");
  free(str_request);
  return result;
}

static bool random_disconnect (void *impl, edgex_protocols *device)
{
  return true;
}

static void random_stop (void *impl, bool force) {}


static void usage (void)
{
  printf ("Options: \n");
  printf ("   -h, --help           : Show this text\n");
  printf ("   -n, --name=<name>    : Set the device service name\n");
  printf ("   -r, --registry=<url> : Use the registry service\n");
  printf ("   -p, --profile=<name> : Set the profile name\n");
  printf ("   -c, --confdir=<dir>  : Set the configuration directory\n");
}

static bool testArg (int argc, char *argv[], int *pos, const char *pshort, const char *plong, char **var)
{
  if (strcmp (argv[*pos], pshort) == 0 || strcmp (argv[*pos], plong) == 0)
  {
    if (*pos < argc - 1)
    {
      (*pos)++;
      *var = argv[*pos];
      (*pos)++;
      return true;
    }
    else
    {
      printf ("Option %s requires an argument\n", argv[*pos]);
      exit (0);
    }
  }
  char *eq = strchr (argv[*pos], '=');
  if (eq)
  {
    if (strncmp (argv[*pos], pshort, eq - argv[*pos]) == 0 || strncmp (argv[*pos], plong, eq - argv[*pos]) == 0)
    {
      if (strlen (++eq))
      {
        *var = eq;
        (*pos)++;
        return true;
      }
      else
      {
        printf ("Option %s requires an argument\n", argv[*pos]);
        exit (0);
      }
    }
  }
  return false;
}

static volatile sig_atomic_t running = true;
static void inthandler (int i)
{
  char  c;
  
  printf("OUCH, did you hit Ctrl-C?\n"
        "Do you really want to quit? [y/n] ");
  scanf(" %c", &c);
  if (c == 'y' || c == 'Y')
      running = false;
  else
      random_discover(NULL);  
}


int main (int argc, char *argv[])
{
  profile = "";
  char *confdir = "";
  svcname = "device-random";
  regURL = getenv ("EDGEX_REGISTRY");

  random_driver * impl = malloc (sizeof (random_driver));
  memset (impl, 0, sizeof (random_driver));
  ////////////////////////////////
  serial_port = user_serial_init("/dev/ttyUSB0", (speed_t) 9600);
  if( serial_port < 0 )
  {
    printf("\n\n-------------------- loi --------------------------\n\n");
      exit(0);
  }
QueuePush       = createQueue(10);
QueueTX         = createQueue(10);
QueueRX         = createQueue(10);
QueueGetHandler = createQueue(10);
QueueDiscovery  = createQueue(10);
iot_thread_create(&ThreadPush,PushEvent, NULL, NULL);
iot_thread_create(&ThreadRX,RX, NULL, NULL);
iot_thread_create(&ThreadTX,TX, NULL, NULL);


  int n = 1;
  while (n < argc)
  {
    if (strcmp (argv[n], "-h") == 0 || strcmp (argv[n], "--help") == 0)
    {
      usage ();
      return 0;
    }
    if (testArg (argc, argv, &n, "-r", "--registry", &regURL))
    {
      continue;
    }
    if (testArg (argc, argv, &n, "-n", "--name", &svcname))
    {
      continue;
    }
    if (testArg (argc, argv, &n, "-p", "--profile", &profile))
    {
      continue;
    }
    if (testArg (argc, argv, &n, "-c", "--confdir", &confdir))
    {
      continue;
    }

    printf ("Unknown option %s\n", argv[n]);
    usage ();
    return 0;
  }

  edgex_error e;
  e.code = 0;

  /* Device Callbacks */
  edgex_device_callbacks randomImpls =
  {
    random_init,         /* Initialize */
    random_discover,     /* Discovery */
    random_get_handler,  /* Get */
    random_put_handler,  /* Put */
    random_disconnect,   /* Disconnect */
    random_stop          /* Stop */
  };

  /* Initalise a new device service */
  service = edgex_device_service_new
    (svcname, "1.0", impl, randomImpls, &e);
  ERR_CHECK (e);

  /* Start the device service*/
  edgex_device_service_start (service, regURL, profile, confdir, &e);
  ERR_CHECK (e);

  signal (SIGINT, inthandler);
  running = true;
/*  
  edgex_device *  device_master = edgex_device_get_device_byname(service, name_master);  
  if( device_master != NULL)
  {
    if (device_master->description != NULL)
    {
      if( strlen (device_master->description))
      {
        printf("old description = %s\n", device_master->description);
        char *description_master = strdup(device_master->description);  
        sscanf(description_master, "%hd", &address_zigbee);
        printf("old addr zigbee = %hd\n", address_zigbee);
        free(description_master);
      }
      else
      {
        address_zigbee = 0;
      }      
    }
    else
    {
      address_zigbee = 0;
    }    
    edgex_device_free_device(device_master);
  }

  address_zigbee++;
  printf("new addr zigbee = %hd\n", address_zigbee);
  printf("\n\nupdate newDescription\n");
  char str_addr[32];
  sprintf(str_addr, "%d", address_zigbee);
  printf("new addr zigbee = %s", str_addr);
  edgex_device_update_device(service,NULL, name_master,str_addr,NULL,NULL, &e);
  if(e.code != 0) 
    printf("\n---------update  :%s\n",e.reason); 
*/
  while (running)
  {  
    sleep(1);
  }

  /* Stop the device service */
  edgex_device_service_stop (service, true, &e);
  ERR_CHECK (e);

  close(serial_port);
  edgex_device_service_free (service);
  free (impl);
  return 0;
}
