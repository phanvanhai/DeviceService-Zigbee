/*
 * Copyright (c) 2018-2019
 * IoTech Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include "edgex/devsdk.h"
#include "edgex/eventgen.h"

#include <unistd.h>
#include <signal.h>

#define ERR_CHECK(x) if (x.code) { fprintf (stderr, "Error: %d: %s\n", x.code, x.reason); edgex_device_service_free (service); free (impl); return x.code; }

static volatile sig_atomic_t running = true;
static void inthandler (int i)
{
  running = (i != SIGINT);
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
  return true;
}

static void random_discover (void *impl) {}

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
  random_driver *driver = (random_driver *) impl;
  bool result = true;

  for (uint32_t i = 0; i < nreadings; i++)
  {
    result = false;
    /* Drill into the attributes to differentiate between resources via the SensorType NVP */
    const edgex_nvpairs * current = requests[i].attributes;
    while (current!=NULL)
    {
      if (strcmp (current->name, "SensorType") ==0 )
      {
        /* Set the resulting reading type as Uint64 */
        readings[i].type = Uint64;

        if (strcmp (current->value, "1") ==0 )
        {
          /* Set the reading as a random value between 0 and 100 */
          readings[i].value.ui64_result = rand() % 100;
          result = true;
        }
        else if (strcmp (current->value, "2") ==0 )
        {
          /* Set the reading as a random value between 0 and 1000 */
          readings[i].value.ui64_result = rand() % 1000;
          result = true;
        }
        else
        {
          iot_log_error (driver->lc, "%s is not a valid SensorType", current->value);
        }
      }

      if (strcmp (current->name, "SwitchID") ==0 )
      {
        readings[i].type = Bool;
        pthread_mutex_lock (&driver->mutex);
        readings[i].value.bool_result=driver->state_flag;
        pthread_mutex_unlock (&driver->mutex);
        result = true;
      }
      current = current->next;
    }
    if (!result)
    {
      break;
    }
  }
  return result;
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

  for (uint32_t i = 0; i < nvalues; i++)
  {
    /* A Device Service again makes use of the data provided to perform a PUT */

    /* In this case we set a boolean flag */
    if (strcmp (requests[i].resname,"Switch") ==0 )
    {
      pthread_mutex_lock (&driver->mutex);
      driver->state_flag=values[i].value.bool_result;
      pthread_mutex_unlock (&driver->mutex);
    }
    else
    {
      iot_log_error (driver->lc, "PUT not valid for resource %s", requests[i].resname);
      result = false;
    }
  }
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
  char *profile = "";
  char *confdir = "";
  char *svcname = "device-random";
  char *regURL = getenv ("EDGEX_REGISTRY");

  random_driver * impl = malloc (sizeof (random_driver));
  memset (impl, 0, sizeof (random_driver));

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
  edgex_device_service *service = edgex_device_service_new
    (svcname, "1.0", impl, randomImpls, &e);
  ERR_CHECK (e);

  /* Start the device service*/
  edgex_device_service_start (service, regURL, profile, confdir, &e);
  ERR_CHECK (e);

  signal (SIGINT, inthandler);
  running = true;

  edgex_device_commandresult values1;
  values1.origin = 0;
  values1.type = Int64;
  values1.value.i64_result = 50;

  printf("Phan Van Hai");
  while (running)
  {
    sleep(3);
    printf("RandomDevice1 sent %ld\n", values1.value.i64_result);
    edgex_device_post_readings(service, "RandomDevice1", "SensorTwo", &values1);
  }

  /* Stop the device service */
  edgex_device_service_stop (service, true, &e);
  ERR_CHECK (e);

  edgex_device_service_free (service);
  free (impl);
  return 0;
}
