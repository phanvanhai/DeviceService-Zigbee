#include "user.h"
#include "structString.h"
//#include"queue1.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include <fcntl.h>		// Contains file controls like O_RDWR
#include <errno.h>		// Error integer and strerror() function

#define SAFE_STR(s) (s ? s : "NULL")
#define SAFE_STRDUP(s) (s ? strdup(s) : NULL)

// static const char *rwstrings[2][2] = { { "", "W" }, { "R", "RW" } };

static const char *adstatetypes[] = { "LOCKED", "UNLOCKED" };

static const char *edgex_adminstate_tostring (edgex_device_adminstate ad)
{
  return adstatetypes[ad];
}

static const char *opstatetypes[] = { "ENABLED", "DISABLED" };

static const char *edgex_operatingstate_tostring
  (edgex_device_operatingstate op)
{
  return opstatetypes[op];
}

static char *get_string (const JSON_Object *obj, const char *name)
{
  const char *str = json_object_get_string (obj, name);
  return strdup (str ? str : "");
}

static char *get_array_string (const JSON_Array *array, size_t index)
{
  const char *str = json_array_get_string (array, index);
  return strdup (str ? str : "");
}

static bool get_boolean (const JSON_Object *obj, const char *name, bool dflt)
{
  int i = json_object_get_boolean (obj, name);
  return (i == -1) ? dflt : i;
}

/* chuyen gia tri Value dang String sang gia tri thuc
*/
static bool populateValue (edgex_device_commandresult *cres, const char *val)
{
  size_t sz;
  switch (cres->type)
  {
    case String:
      cres->value.string_result = strdup (val);
      return true;
    case Bool:
      if (strcasecmp (val, "true") == 0)
      {
        cres->value.bool_result = true;
        return true;
      }
      if (strcasecmp (val, "false") == 0)
      {
        cres->value.bool_result = false;
        return true;
      }
      return false;
    case Uint8:
      return (sscanf (val, "%" SCNu8, &cres->value.ui8_result) == 1);
    case Uint16:
      return (sscanf (val, "%" SCNu16, &cres->value.ui16_result) == 1);
    case Uint32:
      return (sscanf (val, "%" SCNu32, &cres->value.ui32_result) == 1);
    case Uint64:
      return (sscanf (val, "%" SCNu64, &cres->value.ui64_result) == 1);
    case Int8:
      return (sscanf (val, "%" SCNi8, &cres->value.i8_result) == 1);
    case Int16:
      return (sscanf (val, "%" SCNi16, &cres->value.i16_result) == 1);
    case Int32:
      return (sscanf (val, "%" SCNi32, &cres->value.i32_result) == 1);
    case Int64:
      return (sscanf (val, "%" SCNi64, &cres->value.i64_result) == 1);
    case Float32:
      if (strlen (val) == 8 && val[6] == '=' && val[7] == '=')
      {
        size_t sz = sizeof (float);
        return edgex_b64_decode (val, &cres->value.f32_result, &sz);
      }
      else
      {
        return (sscanf (val, "%e", &cres->value.f32_result) == 1);
      }
    case Float64:
      if (strlen (val) == 12 && val[11] == '=')
      {
        size_t sz = sizeof (double);
        return edgex_b64_decode (val, &cres->value.f64_result, &sz);
      }
      else
      {
        return (sscanf (val, "%le", &cres->value.f64_result) == 1);
      }
    case Binary:
      sz = edgex_b64_maxdecodesize (val);
      cres->value.binary_result.size = sz;
      cres->value.binary_result.bytes = malloc (sz);
      return edgex_b64_decode
        (val, cres->value.binary_result.bytes, &cres->value.binary_result.size);
  }
  return false;
}

static edgex_nvpairs *nvpairs_read (const JSON_Object *obj)
{
  edgex_nvpairs *result = NULL;
  size_t count = json_object_get_count (obj);
  for (size_t i = 0; i < count; i++)
  {
    edgex_nvpairs *nv = malloc (sizeof (edgex_nvpairs));
    nv->name = strdup (json_object_get_name (obj, i));
    nv->value = strdup (json_value_get_string (json_object_get_value_at (obj, i)));
    nv->next = result;
    result = nv;
  }
  return result;
}

static JSON_Value *nvpairs_write (const edgex_nvpairs *e)
{
  JSON_Value *result = json_value_init_object ();
  JSON_Object *obj = json_value_get_object (result);

  for (const edgex_nvpairs *nv = e; nv; nv = nv->next)
  {
    json_object_set_string (obj, nv->name, nv->value);
  }

  return result;
}

static edgex_protocols *protocols_read (const JSON_Object *obj)
{
  edgex_protocols *result = NULL;
  size_t count = json_object_get_count (obj);
  for (size_t i = 0; i < count; i++)
  {
    JSON_Value *pval = json_object_get_value_at (obj, i);
    edgex_protocols *prot = malloc (sizeof (edgex_protocols));
    prot->name = strdup (json_object_get_name (obj, i));
    prot->properties = nvpairs_read (json_value_get_object (pval));
    prot->next = result;
    result = prot;
  }
  return result;
}

static JSON_Value *protocols_write (const edgex_protocols *e)
{
  JSON_Value *result = json_value_init_object ();
  JSON_Object *obj = json_value_get_object (result);

  for (const edgex_protocols *prot = e; prot; prot = prot->next)
  {
    json_object_set_value (obj, prot->name, nvpairs_write (prot->properties));
  }
  return result;
}

uint16_t get_nv_config_uint16
(
  iot_logger_t *lc,
  const edgex_nvpairs *config,
  const char *key,
  edgex_error *err
)
{
  for (const edgex_nvpairs *iter = config; iter; iter = iter->next)
  {
    if (strcasecmp (iter->name, key) == 0)
    {
      unsigned long long tmp;
      errno = 0;
      tmp = strtoull (iter->value, NULL, 0);
      if (errno == 0 && tmp >= 0 && tmp <= UINT16_MAX)
      {
        return tmp;
      }
      else
      {
        err->code = 4, err->reason = "Configuration is invalid" ;
        iot_log_error (lc, "Unable to parse %s as uint16", iter->value);
        return 0;
      }
    }
  }
  return 0;
}

char *get_nv_config_string
  (const edgex_nvpairs *config, const char *key)
{
  for (const edgex_nvpairs *iter = config; iter; iter = iter->next)
  {
    if (strcasecmp (iter->name, key) == 0)
    {
      return strdup (iter->value);
    }
  }
  return NULL;
}
/*------------------------------------------------------------------------------------------ */
char *user_discovery_write (void)
{
    char *result;
    JSON_Value *val = json_value_init_object ();
    JSON_Object *obj = json_value_get_object (val);

    json_object_set_string (obj, "device", "");
    json_object_set_string (obj, "MAC", "");
    json_object_set_string (obj, "devtype", "");
    json_object_set_string (obj, "profile", "");

    result = json_serialize_to_string (val);
    json_value_free (val);
    return result;
}

user_discovery *user_discovery_read(const char* json_input)
{
    JSON_Value *val = json_parse_string (json_input);
    if (val == NULL)
    {
        fprintf(stderr, "Waring: Payload did not parse as JSON\n");
        return NULL;        
    }
    JSON_Object *jobj = json_value_get_object (val);
    user_discovery * result = malloc( sizeof(user_discovery) );

    result->device = get_string(jobj, "device");
    result->MAC = get_string(jobj, "MAC");
    result->devtype = get_string(jobj, "devtype");
    result->profile = get_string(jobj, "profile");

    json_value_free (val);
    return result;
}

void user_discovery_free(user_discovery *e)
{
    free(e->device);
    free(e->MAC);
    free(e->devtype);
    free(e->profile);
    free(e);
}

char *user_add_device_write(user_add_device *e)
{
    char *result;
    JSON_Value *val = json_value_init_object();
    JSON_Object *obj = json_value_get_object(val);

    json_object_set_string (obj, "MAC", e->MAC);
    json_object_set_string (obj, "device", e->device);    
    json_object_set_string (obj, "addr", e->addr);
    json_object_set_string (obj, "adminState", edgex_adminstate_tostring (e->adminState));
    json_object_set_string (obj, "opState", edgex_operatingstate_tostring(e->opState));

    result = json_serialize_to_string (val);
    json_value_free (val);
    return result;
}

char *user_protocol_write ( const char *devname, edgex_protocols *protocols)
{
    char *result;    
    JSON_Value *val = json_value_init_object ();
    JSON_Object *obj = json_value_get_object (val);

    json_object_set_string (obj, "device", devname);
    json_object_set_uint (obj, "origin", 0);    
    json_object_set_value (obj, "protocols", protocols_write (protocols));

    result = json_serialize_to_string (val);
    json_value_free (val);
    return result;
}

user_push_event *user_push_event_read(edgex_device_service *svc,char *json_input)
{
  sensorEvent *ev =takeEvent(json_input);
  user_push_event *result_event = malloc(sizeof(user_push_event)); 
  result_event->device_name= strdup(ev->name);
  uint64_t origin = ev->origin;

  edgex_device *dev = edgex_device_get_device_byname (svc, result_event->device_name);
  if (dev == NULL)
  {
    fprintf(stderr, "Post readings: no such device %s\n", result_event->device_name);
    free(result_event);
    freeEvent(ev);
    return NULL;
  }
  result_event->resource_name = strdup (ev->commandDevice);
    if( result_event->resource_name == NULL)
    {
      fprintf(stderr, "Not found resource_name\n");
      edgex_device_free_device (dev);    
      free(result_event);
      freeEvent(ev);
      return NULL;
    }    
    const edgex_cmdinfo *commandinfo = edgex_deviceprofile_findcommand (result_event->resource_name, dev->profile, true);
    if( commandinfo == NULL)
    {
      printf( "Not found command_info\n");
      free(result_event->resource_name);

      edgex_device_free_device (dev);    
      free(result_event);
      freeEvent(ev);
      return NULL;
    }
    result_event->values = calloc (commandinfo->nreqs, sizeof (edgex_device_commandresult)); // size
    result_event->nreqs = commandinfo->nreqs;  //size
    for (int i = 0; i < commandinfo->nreqs; i++)
    {
      const char *value = NULL;
      const char *resname = commandinfo->reqs[i].resname; // temp humi     

      result_event->values[i].type = commandinfo->pvals[i]->type;
      result_event->values[i].origin = origin;

      if (!populateValue (&result_event->values[i], ev->value[i]))   // chuyen str->int
      {                
        fprintf(stderr, "Cannot get Value of %s\n", resname);
        edgex_device_free_device (dev);    
        free(result_event->resource_name);
        edgex_device_commandresult_free(result_event->values,  commandinfo->nreqs );          
        free(result_event);
        freeEvent(ev);
        return NULL;
      }
    }
  // printf("vongcuoi\n");
  edgex_device_free_device (dev);
  freeEvent(ev);       
  return result_event;
}

void user_push_event_free(user_push_event *e)
{
  free(e->device_name);
  free(e->resource_name);
  edgex_device_commandresult_free(e->values, e->nreqs);
  free(e);
}

char *user_gethandler_write(const char *devname,
                            uint32_t nreadings,
                            const edgex_device_commandrequest *requests)
{
  char fullResname[100]="\0";
  for(uint32_t i = 0; i < nreadings; i++)
  {
    //requests[i].resname, "");
    strcat(fullResname,requests[i].resname);
    strcat(fullResname,"#");
  }
  int length = strlen(devname)+ strlen("100")+5+strlen(fullResname)+6; //6= 5# +1CMD_GET
  char * result = malloc(length);
  sprintf(result,"%c#%s#%s#%d#%s",CMD_GET,devname,"100",nreadings,fullResname); 
   // sprintf(result,"%s#%s#%d#%s",devname,"100",nreadings,fullResname);
  return result;
}                            
//result = user_repget_event_read(readings,devname, requests, nreadings, test);
bool user_repget_event_read( edgex_device_commandresult *readings, const char *devname, const edgex_device_commandrequest *requests, 
                      uint32_t nreadings, char *json_input)
{
  // JSON_Value *jval = json_parse_string (json_input);

  // if (jval == NULL)
  // {
  //   fprintf(stderr, "Waring: Payload did not parse as JSON\n");
  //   return false;
  // }  
  // JSON_Object *jobj = json_value_get_object (jval);
  // const char* device_name = json_object_get_string (jobj, "device");
  sensorEvent * ev=takeEvent(json_input);

  if( strcmp(ev->name, devname) )
  {
    fprintf(stderr, "Khong phai phan hoi cua thiet bi mong muon\n");
    //AddNode(&QueueGetHandler,json_input,&mutex_Gethandler);
    // printf("%s\n",ev->name);
    // printf("%s\n",ev->commandDevice);
    //printf("%s\n",ev->value[0]);
    //printf("%s\n",ev->name);
    if(ev!=NULL)
    {
      freeEvent(ev);
    }
    return false;
  }
  // printf("\n------------------------>ben trong repget\n");

  uint64_t origin = ev->origin;

  // JSON_Object *obj_data = json_object_get_object(jobj,"readings");
  // size_t count = json_object_get_count (obj_data);
  // if( count > 0)
  // {                
  //   JSON_Value *pval = json_object_get_value_at (obj_data, 0);
  //   JSON_Object *obj_readings = json_value_get_object(pval);
  
    for (int i = 0; i < nreadings; i++)
    {
      // printf("\nben trong vong lap\n");
      const char *value = NULL;
      const char *resname = requests[i].resname;   /// 1 temp  2 humi   
      // printf("\n vong lap a1\n");
      //value = json_object_get_string (obj_readings, resname);}
      value = ev->value[i];
    
      // printf("\n vong lap a2\n");
      if (value == NULL)
      {
        fprintf(stderr, "Cannot get Value of %s in JSON Readings\n", resname);        
        // json_value_free (jval);   
        freeEvent(ev);                  
        return false;
      }
      readings[i].type = requests[i].type;   // 
      readings[i].origin = origin;
      // printf("\n vong lap 1\n");
      if (!populateValue (&readings[i], value))
      {        
        fprintf(stderr, "Cannot get Value of %s\n", resname);
        //json_value_free (jval);    
        freeEvent(ev);
        return false;
      }
    }
      //json_value_free (jval);
      freeEvent(ev);
      // printf("\n vong lap 2\n");
      return true;  
  }                     

char *user_puthandler_write(const char *devname,
                            uint32_t nvalues,
                            const edgex_device_commandrequest *requests,
                            const edgex_device_commandresult *values)
{
  //CMD#Name#Origin#size#value#12#12#12
  // JSON_Value *json_result = json_value_init_object ();
  // JSON_Object *obj = json_value_get_object (json_result);
  // json_object_set_string (obj, "device", devname);
  // json_object_set_uint (obj, "origin", 0);
   char fullValue[100]="\0";
  for(uint32_t i = 0; i < nvalues; i++)
  {
    char *str_values = edgex_value_tostring (&values[i], true);
    // json_object_set_string(obj_put, requests[i].resname, str_values);
    strcat(fullValue,requests[i].resname);
    strcat(fullValue,":");
    strcat(fullValue,str_values);
    strcat(fullValue,"#");
    free(str_values);
  }
  uint32_t length = strlen(devname)+ strlen("100")+5+strlen(fullValue)+6; //6= 5# +1CMD_GET
  char * result = malloc(length);
  sprintf(result,"%c#%s#%s#%d#%s",CMD_PUT,devname,"100",nvalues,fullValue);
  // json_object_set_value (obj_readings, "put",value_put );

  // json_object_set_value (obj, "readings", json_readings);

  // result = json_serialize_to_string(json_result);
  // json_value_free(json_result);
  return result;
}

int user_serial_init(const char * _file, speed_t _speed)
{
	int fd = open(_file, O_RDWR | O_NOCTTY);
	if( fd < 0 )
	{
		fprintf(stderr, "Error %i from open: %s\n", errno, strerror(errno));
		return -1;
	}
		
	// Create new termios struc, we call it 'tty' for convention
	struct termios tty;
	memset( &tty, 0, sizeof(tty));

	// Read in existing settings, and handle any error
	if( tcgetattr(fd, &tty) != 0 )
	{
		fprintf(stderr,"Err %i from tcgetattr: %s\n", errno, strerror(errno));
		return -1;
	}
	
	tty.c_cflag	&= 	~PARENB;	// parity
	tty.c_cflag	&=	~CSTOPB;	// only one stop bit
	tty.c_cflag	|=	CS8;		// 8 bits per byte
	tty.c_cflag |= 	CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1
	// tty.c_lflag &= 	~ICANON;
	tty.c_lflag |= 	ICANON;
	tty.c_lflag &= 	~ECHO; 		// Disable echo
	tty.c_lflag &= 	~ECHOE; 	// Disable erasure
	tty.c_lflag &= 	~ECHONL; 	// Disable new-line echo
	tty.c_lflag &= 	~ISIG; // Disable interpretation of INTR, QUIT and SUSP

	tty.c_iflag &= 	~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
	tty.c_iflag &= 	~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // Disable any special handling of received bytes

	tty.c_oflag &= 	~OPOST; 	// Prevent special interpretation of output bytes (e.g. newline chars)
	tty.c_oflag &= 	~ONLCR; 	// Prevent conversion of newline to carriage return/line feed
	
	// Set in/out baud rate to be 9600
	cfsetispeed(&tty, _speed);
	cfsetospeed(&tty, _speed);
	
	// Save tty settings, also checking for error
	if( tcsetattr(fd, TCSANOW, &tty) != 0)
	{
		fprintf(stderr, "Error %i from tcsetattr: %s\n", errno, strerror(errno));
		return -1;
	}
	return fd;		
}

int user_serial_write(int _fd, const void *__buf, size_t __n)
{
    int n1 = write(_fd, __buf, __n);
    int n2 = write(_fd, "\n", 2);
    return n1 + n2;
}

int user_serial_read(int _fd, void *_buf, size_t _nbytes)
{
    int n = read(_fd, _buf, _nbytes);
    if( n < 0 )    
        fprintf(stderr, "Error %i from Read: %s\n", errno, strerror(errno));      
    else    
        ((char*)_buf)[n] = '\0';             
    return n;
}
