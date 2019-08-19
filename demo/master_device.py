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

DEFAULT_NAME        = "MasterDevice"

CMD_PUSH            = "a"
CMD_PUT             = "c"

URL_GET_DEVICE_BY_LABEL = "http://localhost:48081/api/v1/device/label/{}"
URL_POST_DISCOVERY  = "http://localhost:49990/api/v1/discovery"
URL_PUT_COMMAND     = "http://localhost:48082/api/v1/device/name/{}/command/{}"

URL_ADD_DEVICE  = "http://localhost:48081/api/v1/device"
BODY_ADD_DEVICE = "{\"name\":\"%s\",\"adminState\":\"unlocked\",\"operatingState\":\"enabled\",\"protocols\":{\"zigbee\":{\"address\":\"%s\"}},\"service\":{\"name\":\"device-random\"},\"profile\":{\"name\":\"%s\"}}"
template_push       = "a#{}#0#MasterRequest#3#value#{}#{}#{}#\n"          # name#origin#MasterRequest#size#value#30#40#

INDEX_SIZE_PUT      = 3
INDEX_VALUE_PUT     = 4

my_name             = DEFAULT_NAME

flag_main_continous = True
device_to_edgex_buf = []

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
    print("sorry, I haven't code fo this part")
    # index = str_.find(':')
    # resname = str_[:index]
    # value   = str_[index+1:]
    # if str_ == "Switch":
    #     Switch_value = value
    # else:
    #     pass
    # print("\t" + resname + "=" + value)

def set_arr_values(arr):
    for x in arr:
        set_value(x)

def device_repput_edgex(arr):
    print("Thuc hien lenh PUT:")
    size = arr[INDEX_SIZE_PUT]
    arr_values = arr[INDEX_VALUE_PUT: (INDEX_VALUE_PUT + int(size))]
    set_arr_values(arr_values)


def receive_proccess_edgex(input):
    global my_name
    input.strip()
    arr = input.split('#') 
    if str(arr[1]) == my_name: 
        print("Device nhan duoc yeu cau:\n\t",input)       
        if (arr[0]  == CMD_PUT ):            
            device_repput_edgex(arr)
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
    device_to_edgex_buf.append(message.payload.decode('utf-8'))        


# --------------------------Console Menu---------------------------------------------
def menu():
    os.system("clear")
    print("---------------------> Tin hoc cong nghiep - DHBKHN <---------------------")
    print("1: Trigger Discovery")
    print("2: Add new device")
    print("3: Get new device by label")
    print("4: Send PUT Command")
    print("5: Quit")

    choice = input(">>> ")

    if choice == "1":
        url = URL_POST_DISCOVERY
        body = "body"
        request = template_push.format(my_name, "POST", url, body)
        client.publish(topic_pub, request)        
        os.system("clear")
        print(request)
        print("Send Request: Discovery")
        input("Press to Return Menu")
        menu()
    elif choice == "2":        
        url = URL_ADD_DEVICE
        devname = input("name new device >>> ")
        address = input("protocols: zigbee\n\taddress >>> ")
        profile = input("name profile    >>> ")
        body = BODY_ADD_DEVICE % (devname, address, profile)
        request = template_push.format(my_name, "POST", url, body)
        client.publish(topic_pub, request)        
        os.system("clear")
        print(request)
        print("Add new device :", devname)
        input("Press to Return Menu")
        menu()

    elif choice == "3":
        label = input("label >>> ")
        url   = URL_GET_DEVICE_BY_LABEL.format(label)
        body  = "body"
        request = template_push.format(my_name, "GET", url, body)
        client.publish(topic_pub, request)        
        os.system("clear")
        print(request)
        print("GET devices by label:", label)
        input("Press to Return Menu")
        menu()

    elif choice == "4":
        print          ("-------------> Send PUT command <-------------")
        device  = input("name Device        >>> ")
        command = input("command            >>> ")
        url = URL_PUT_COMMAND.format(device, command)
        body    = input("body {\"a\":\"b\"} >>> ")
        request = template_push.format(my_name, "PUT", url, body)
        client.publish(topic_pub, request)        
        os.system("clear")
        print(request)
        print("Send PUT to device:", device, "command:", command)
        input("Press to Return Menu")
        menu()
    elif choice == "5":
        pass
    os.system("clear")
    

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
        menu()
    except KeyboardInterrupt:
        print("exiting")
        flag_main_continous = False
        client.disconnect()
        client.loop_stop()
