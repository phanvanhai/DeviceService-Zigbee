#include "user.h"
#include <signal.h>

const char *json_read_event1 = "{\"device\":\"random\",\"origin\":60,\"data\":{\"resource\":{\"temp\":\"30\",\"hum\":\"80\"}, \"resource1\":{\"temp\":\"40\",\"hum\":\"75%\"}}}\n";
const char *json_read_discovery = "{\"device\":\"newDev\",\"MAC\":\"MAC1234\",\"devtype\":\"SenSor\",\"profile\":\"Profile_Sensor\"}\n";
const char *json_read_event = "{\"device\":\"RandomDevice1\",\"origin\":60,\"data\":{\"SensorOne\":{\"SensorOne\":\"100\"}}}\n";
static volatile sig_atomic_t running = 1;
static void inthandler (int i)
{
  running = 0;
}

void test_user_discovery(const char* json_discovery)
{
    printf("\n---> Test user_discovery_write: <---\n");
    char *write = user_discovery_write ();
    printf("%s\n", write);
    free(write);

    printf("\n---> Test user_discovery_read: <---\n");
    user_discovery *e = user_discovery_read(json_discovery);
    user_discovery *e_dup = user_discovery_dup(e);
    user_discovery_free(e);

    printf("device:%s\n", e_dup->device);
    printf("MAC:%s\n", e_dup->MAC);
    printf("devtype:%s\n", e_dup->devtype);
    printf("profile:%s\n", e_dup->profile);
    user_discovery_free(e_dup);
}
void test_user_add_device(void)
{
    printf("\n---> Test user_add_device_write: <---\n");
    user_add_device e;
    e.device = "testAdd";
    e.MAC = "MAC012345";
    e.addr = "ADC";
    e.adminState = UNLOCKED;
    e.opState = ENABLED;

    char *write = user_add_device_write(&e);
    printf("%s\n", write);
    free(write);
}
void test_print_event(user_event_protocol *event)
{   
    printf("\n---> Test user_event_protocol_read: <---\n"); 
    if( event != NULL)
    {
        printf("device:%s\norigin:%ld\ndata:\n", event->device,event->origin);
        for (const edgex_protocols *prot = event->data; prot; prot = prot->next)
        {
            printf("\t%s:\n", prot->name);
            for (const edgex_nvpairs *nv = prot->properties; nv; nv = nv->next)
            {
                printf ("\t\t%s:%s\n", nv->name, nv->value);
            }
        }
    }
    else
    {
        printf("Event Null\n");
    }    
}
void test_user_event_protocol(const char * test_json)
{
    user_event_protocol * event;
    event = user_event_protocol_read(test_json);
    user_event_protocol *event1 = user_event_protocol_dup(event);
    user_event_protocol_free(event);
    test_print_event(event1) ;
    char * write = user_event_protocol_write(event1);
    printf("\n---> Test user_event_protocol_write: <---\n%s\n", write);
    free(write);
    user_event_protocol_free(event1);
}
int main()
{
    int serial_port = user_serial_init("/dev/ttyUSB0", (speed_t) 9600);
    if( serial_port < 0 )
        exit(0);
	
	signal (SIGINT, inthandler);
	running = 1;
    char buf[255];
    memset(&buf, '\0', sizeof(buf));
    int n = 0;

    test_user_event_protocol(json_read_event);    
    test_user_add_device();
    test_user_discovery(json_read_discovery);

	while (running)
	{
        //  check Emty(queue) -> takeNode(queue) -> write
		// fprintf( stdout, "write = %d\n",user_serial_write(serial_port, buf, strlen(buf)) );
        // sleep(1);
        n = user_serial_read(serial_port, &buf, sizeof(buf));        
        if( n > 0 )
        {
            // event = user_event_protocol_read(buf);   
            // test_print_event(event);
            // check -> addNode(queue)
        }
	}

	close(serial_port);
    return 0;
}