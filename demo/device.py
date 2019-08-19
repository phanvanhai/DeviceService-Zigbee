import paho.mqtt.client as mqtt
import os, time
import random
from threading import Thread
import sys

USERNAME = "ttdqymlc"
PASSWORD = "x8cN-GqZBJPK"
SERVER = 	"m16.cloudmqtt.com"
PORT = 	14023
QOS = 0

topic_sub = "edgex2device"
topic_pub = "device2edgex"

# -------------------ham cho xu ly du lieu-------------------------------

DEFAULT_NAME        = "newdevice"
MAC                 = "29081997"
PROFILE             = "ExampleSensor"
MODEL               = "BKHN"
CMD_PUSH            = "a"
CMD_GET             = "b"
CMD_PUT             = "c"
CMD_DISCOVERY       = "d"
CMD_ADD_DEVICE      = "e"

DEVICE_COMMAND_1    = "SensorOne"
LIST_RES_PUSH_1     = ["SensorOne",]
DEVICE_COMMAND_2    = "SensorTwo"
LIST_RES_PUSH_2     = ["SensorTwo",]
DEVICE_COMMAND_3    = "Switch"
LIST_RES_PUSH_3     = ["Switch",]

template_push       = "a#{}#{}#{}#{}#value#{}\n"          # name#origin#device_command#size#value#30#40#
template_repget     = "b#{}#{}#repget#{}#value#{}\n"          # name#origin#device_command#size#value#30#40#
template_discovery  = "d#{}#{}#{}#{}#\n"                  # name#mac#profile#model#

INDEX_SIZE_GET      = 3
INDEX_VALUE_GET     = 4
INDEX_SIZE_PUT      = 3
INDEX_VALUE_PUT     = 4

my_name             = DEFAULT_NAME
addr_zigbee         = None
Switch_value        = "false"

flag_main_continous = True
device_to_edgex_buf = []

# test_get            = "b#RandomDeviceH#60#3#SensorOne#SensorTwo#Switch#\n"           

# ------- Define Classes -------
class ClassThread (Thread):
    def __init__(self, func):
        Thread.__init__(self)  # , daemon = True)
        self.func = func
        print(func.__name__)

    def run(self):
        self.func()

# -------------------------------- Define Functions ----------------------------------

def set_value(str_):
    index = str_.find(':')
    resname = str_[:index]
    value   = str_[index+1:]
    if str_ == "Switch":
        Switch_value = value
    else:
        pass
    print("\t" + resname + "=" + value)

def set_arr_values(arr):
    for x in arr:
        set_value(x)

def get_value(str_res):
    if str_res == "SensorOne":
        return str(random.randint(30, 40))
    elif str_res == "SensorTwo":
        return str(random.randint(70, 100))
    elif str_res == "Switch":
        return Switch_value
    else:
        return ""

def get_arr_value(arr):
    str_values = ""
    for x in arr:        
        value = get_value(x)
        if value != "":
            str_values = str_values  + str(value) + "#"
        else:
            return ""
    return str_values

def device_repdiscovery():
    str_ = template_discovery.format(my_name, MAC, PROFILE, MODEL)
    client.publish(topic_pub, str_) 
    print("\t" + str_)

def device_repadd_device(arr_input):
    global my_name, addr_zigbee
    if( arr_input[1] == my_name and arr_input[2] == MAC):
        my_name     = arr_input[3]
        addr_zigbee = arr_input[4]
        print("\t" + my_name + "-" + addr_zigbee)

def device_push_edgex(device_command, arr_resname):
    print("Device thuc hien PUSH:")
    origin = random.randint(0, 100)    
    str_values  = get_arr_value(arr_resname)
    if str_values == "":
        return ""
    else:
        pass
    str_result = template_push.format(my_name, origin ,device_command, len(arr_resname), str_values)
    print("\t" + str_result)
    return str_result

def device_repput_edgex(arr):
    print("Thuc hien lenh PUT:")
    size = arr[INDEX_SIZE_PUT]
    arr_values = arr[INDEX_VALUE_PUT: (INDEX_VALUE_PUT + int(size))]
    set_arr_values(arr_values)

def device_repget_edgex(arr):    
    str_result = ""
    origin = random.randint(0, 100)

    size = arr[INDEX_SIZE_GET]    
    arr_values = arr[INDEX_VALUE_GET: (INDEX_VALUE_GET + int(size)) ]
    str_values = get_arr_value(arr_values)
    if str_values == "":
        return ""
    else:
        pass
    str_result = template_repget.format(my_name, origin,size ,str_values)    
    print("Phan hoi lenh GET:\n\t", str_result)
    client.publish(topic_pub, str_result)         
    return str_result

def receive_proccess_edgex(input):
    global my_name
    input.strip()
    arr = input.split('#') 
    if str(arr[1]) == my_name: 
        print("Device nhan duoc yeu cau:\n\t",input)       
        if (arr[0]  == CMD_GET ):            
            device_repget_edgex(arr)
        elif arr[0] == CMD_PUT:
            device_repput_edgex(arr)
        elif arr[0] == CMD_DISCOVERY:
            device_repdiscovery()
        elif arr[0] == CMD_ADD_DEVICE:
            device_repadd_device(arr)
        else:
            pass
def th_process():
    while flag_main_continous: 
        try:
            if len(device_to_edgex_buf) > 0:            
                    data = device_to_edgex_buf.pop(0)                       
                    receive_proccess_edgex(data)
        except IndexError:
            pass
# -----------------------------------------------------------------------
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to broker")
        global Connected                            
        Connected = True                            
    else:
        print("Connection failed")
 
def on_message(client, userdata, message):
    #pass
    #print("Device nhan duoc yeu cau:\n\t",message.payload)
    # receive_proccess_edgex(message.payload.decode("utf-8"))
    device_to_edgex_buf.append(message.payload.decode('utf-8'))        

if __name__ == "__main__":
    if (len(sys.argv) >= 2):
        my_name = sys.argv[1]
    print(my_name)
    Connected = False   

    client = mqtt.Client()                     
    client.on_connect= on_connect                      
    client.on_message= on_message     

    client.username_pw_set(username=USERNAME, password=PASSWORD)                 
    client.connect(SERVER, PORT, keepalive=30)
    # client.connect("localhost", 1883)                  
    client.loop_start()                                
    
    while Connected != True:                           
        time.sleep(0.1)
 
    client.subscribe(topic_sub)

    thread_mqtt = ClassThread(th_process)
    thread_mqtt.setDaemon(True)
    thread_mqtt.start()

    try:
        while True:
            time.sleep(5)        
            str_ = device_push_edgex( DEVICE_COMMAND_1 ,LIST_RES_PUSH_1)
            str_ = device_push_edgex( DEVICE_COMMAND_1 ,LIST_RES_PUSH_1)
            client.publish(topic_pub, str_) 
            #print(repget)         
            # time.sleep(5)
            # str_ = device_push_edgex( DEVICE_COMMAND_2 ,LIST_RES_PUSH_2)
            # client.publish(topic_pub, str_)   
            # time.sleep(5)
            # str_ = device_push_edgex( DEVICE_COMMAND_3 ,LIST_RES_PUSH_3)
            # client.publish(topic_pub, str_) 
    
    except KeyboardInterrupt:
        print("exiting")
        flag_main_continous = False
        client.disconnect()
        client.loop_stop()