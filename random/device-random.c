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
#include "queue1.h"

int serial_port;
// pthread_mutex_t a_mutex; /*Khai báo biến mutex toàn cục*/
// char *json_read_event = "{\"device\":\"RandomDeviceH\",\"origin\":60,\"readings\":{\"SensorOne\":{\"SensorOne\":\"45\"}}}\n";
// char *json_read_rep = "{\"device\":\"RandomDevice1\",\"origin\":60,\"readings\":{\"SensorOne\":{\"SensorOne\":\"59\"}}}\n";
// char *rep_discovery = "{\"device\":\"new device\",\"MAC\":\"47447\",\"devtype\":\"sensor\",\"profile\":\"ExampleSensor\"}\n";
#define ERR_CHECK(x) if (x.code) { fprintf (stderr, "Error: %d: %s\n", x.code, x.reason); edgex_device_service_free (service); free (impl); return x.code; }
edgex_device_service *service;
char *svcname;
char *regURL;
char *profile;

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
        AddNode(&QueuePush, test+2, &mutex_Push);
        break;

      case CMD_GET:
        AddNode(&QueueGetHandler, test+2, &mutex_Gethandler);              
        break;
      
      case CMD_DISCOVERY:
        AddNode(&QueueDiscovery, test+2, &mutex_Discovery);
        break;

      default:
        break;
      }
    }
  }
}

void *TX(void *t)
{
  char test[255];
  while(1)
  {
    if(TakeNode(&QueueTX,test, &mutex_TX))
    {
      printf("\n---------------> Serial TX:\n\t%s\n",test);
      user_serial_write(serial_port, test, strlen(test));
    }
  }
}

void *PushEvent(void *t)
{
  char test[255];
  while(1)
  {            
    if(TakeNode(&QueuePush,test, &mutex_Push))
    {                      
      printf("\n---------------> Push\n");
      user_push_event *push_event = user_push_event_read(service, test);    
      if( push_event != NULL)
      {
        edgex_device_post_readings (service, push_event->device_name ,push_event->resource_name, push_event->values);        
        user_push_event_free(push_event);     
      }
    }            
  }  
}
int address_zigbee = 0x1250;

static bool user_add_new_device(char *input_json)
{
  const char *profile_name;  
  edgex_error err;
  char addr_str[4];
  sprintf(addr_str, "%u", address_zigbee);

  user_discovery * discovery = user_discovery_read(input_json);
  profile_name = discovery->profile;
  
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
  user_discovery_free(discovery);
  if( id_dev != NULL )
  {
    printf("-------------------------------> \n\tda them 1 thiet bi ID =%s\n", id_dev);
    free(id_dev);
    return true;
  }
  else
  {
    printf("-------------------------------> \n\tKHONG them dc 1 thiet bi\n");
    return false;
  }
  
}

static void random_discover (void *impl) {
  printf("--------------------------------------> discovery\n");
  char *dis = user_discovery_write();
  if( dis == NULL )
  {
    return;
  }
  char * str_send = malloc( strlen(dis) + 2);  
  sprintf(str_send, "%c%s",CMD_DISCOVERY, dis);  
  free(dis); 

  char test[255];
  int count = 10; // 10s
  while(count-- > 0)
  {
    AddNode(&QueueTX, str_send, &mutex_TX);
    sleep(1);    
    if(TakeNode(&QueueDiscovery,test, &mutex_Discovery))
    {
      bool result = user_add_new_device(test);
      if( result )
        printf("\t\tokkkkkkkkkkkk\n");
      else
      {
        printf("\t\tfalseeeeeeeeeeee\n");
      }      
    }        
  } 
  free(str_send);
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
  // random_driver *driver = (random_driver *) impl;
  char *str_request = user_gethandler_write( devname, nreadings, requests);  // struct ->string-json
  if( str_request == NULL )
  {
    return false;
  }
  printf("\n-----------------> GET send command:\n\t%s\n", str_request);         
  AddNode(&QueueTX,str_request, &mutex_TX);
  free(str_request);

  // // printf("---------------------GET 2--------------------\n");

  result = false;
  char test[255]="\0";//"RandomDeviceH#60#SensorOne#3#value#100#50#60#";
  int count = 30; // 3s
  //result = user_repget_event_read(readings,devname, requests, nreadings, test);
  while(count-- > 0)
  {
    usleep(100000);
    if(TakeNode(&QueueGetHandler,test, &mutex_Gethandler))
    {      
      result = user_repget_event_read(readings,devname, requests, nreadings, test);
     if( result )
      {
        printf("\nphan hoi thanh cong\n");
        return true;
      }     
    }  
  // printf("\nwaiting QueueGethandler\n");
  //return result;    
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
  

  AddNode(&QueueTX, str_request, &mutex_TX);
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
  QueueInit();
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




  // edgex_protocols *newprot = malloc (sizeof (edgex_protocols));
  // newprot->name = strdup("zibee");
  // edgex_nvpairs *nv = malloc(sizeof(edgex_nvpairs));
  // nv->name = strdup("addr");
  // nv->value = strdup("1234");
  // nv->next = NULL;
  // newprot->properties = nv;
  // newprot->next = NULL;

  // char * prot = user_protocol_write ( "test write protocol", newprot);
  // printf("\n-----------------> add prototcol: %s\n\n", prot);
  // edgex_protocols_free(newprot);
  // free(prot);  
      
  while (running)
  {
    sleep(5);
  }

  /* Stop the device service */
  edgex_device_service_stop (service, true, &e);
  ERR_CHECK (e);

  close(serial_port);
  edgex_device_service_free (service);
  free (impl);
  return 0;
}
