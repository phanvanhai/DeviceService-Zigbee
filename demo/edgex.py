import paho.mqtt.client as mqtt
import time
from threading import Thread
import serial

ser = None
client = None
device_to_edgex_buf = []
edgex_to_device_buf = []
flag_main_continous = True


USERNAME = "ttdqymlc"
PASSWORD = "x8cN-GqZBJPK"
SERVER = 	"m16.cloudmqtt.com"
PORT = 	14023
QOS = 0
topic_pub = "edgex2device"
topic_sub = "device2edgex"

test_get            = "2#RandomDevice1#100#1#SensorOne#\n" 
# ------- Define Classes -------
class ClassThread (Thread):
    def __init__(self, func):
        Thread.__init__(self)  # , daemon = True)
        self.func = func
        print(func.__name__)

    def run(self):
        self.func()

# -------------------------------- Define Functions ----------------------------------
# -------------------------------------------- MQTT
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to broker")                         
    else:
        print("Connection failed")
    client.subscribe(topic_sub, qos=QOS)    

def on_message(client, userdata, message):    
    #print("MQTT: received message :", message.payload)
    device_to_edgex_buf.append(message.payload.decode('utf-8'))        

def on_disconnect(client, userdata, rc):
    if rc == 0:
        global flag_main_continous
        flag_main_continous = False
        print("Disconnected")
    else:
        print('Network error has interrupted')

def initMQTT():      
    client = mqtt.Client()                     
    client.on_connect= on_connect                      
    client.on_message= on_message  
    client.on_disconnect = on_disconnect                                   
    return client

def th_MQTT_pub():
    x = True
    while x:
        try:
            client.username_pw_set(username=USERNAME, password=PASSWORD)                 
            client.connect(SERVER, PORT, keepalive=30)
            # client.connect("localhost", 1883) 
            x = False
            edgex_to_device_buf.clear()
        except BaseException:
            time.sleep(5)
    client.loop_start()
    while flag_main_continous:        
        try:
            if len(edgex_to_device_buf) > 0:
                data = edgex_to_device_buf.pop(0)
                client.publish(topic_pub, data, qos=QOS)
                print("EdgeX send to Device:" ,data)
        except IndexError:
            pass
    client.loop_stop()

# ------------------------------------------- UART
def th_UART_read():
    global ser, edgex_to_device_buf    
    ser.flush()
    while flag_main_continous: 
        line = ser.readline().decode('utf-8')
        ser.read(1)
        edgex_to_device_buf.append(line)                
    ser.close()

# ------- main ------------------------------------------------------------------

# ------- multithread
ser = serial.Serial(
    port='/dev/ttyUSB0',
    baudrate=9600,
    parity=serial.PARITY_NONE,
    stopbits=serial.STOPBITS_ONE,
    bytesize=serial.EIGHTBITS,
    timeout=None)

client = initMQTT()

thread_mqtt = ClassThread(th_MQTT_pub)
thread_mqtt.setDaemon(True)
thread_mqtt.start()

thread_ser = ClassThread(th_UART_read)
thread_ser.setDaemon(True)
thread_ser.start()

while flag_main_continous:                
    try:
        time.sleep(2)
        if len(device_to_edgex_buf) > 0:            
            data = device_to_edgex_buf.pop(0)                                    
            print("Device send to EdgeX:" ,data)
            ser.write(data.encode('utf-8'))
    except IndexError as e:
        print("loi")
    except KeyboardInterrupt:
        flag_main_continous = False
        time.sleep(3)
