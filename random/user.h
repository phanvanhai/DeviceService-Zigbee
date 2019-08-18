#ifndef _USER_H_
#define _USER_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "user_typdef.h"
#include "parson.h"
#include <termios.h>	
#include <unistd.h>		

extern repDiscovery * takeDiscovery(char *str);
extern void freeDiscovery(repDiscovery *d);
extern char * takeString(char **str);
extern sensorEvent *takeEvent(char * str);
extern string *takeValue(char *str,int size,sensorEvent *ev);
extern void freeEvent(sensorEvent *ev);
extern char * takeString(char **str);
extern bool edgex_b64_decode (const char *in, void *outv, size_t *outLen);
extern size_t edgex_b64_maxdecodesize (const char *in);
extern edgex_strings *edgex_strings_dup (const edgex_strings *strs);
extern void edgex_strings_free (edgex_strings *strs);
extern edgex_nvpairs *edgex_nvpairs_dup (const edgex_nvpairs *p);
extern void edgex_nvpairs_free (edgex_nvpairs *p);
extern const char *edgex_propertytype_tostring (edgex_propertytype pt);
extern bool edgex_propertytype_fromstring (edgex_propertytype *res, const char *str);
extern edgex_protocols *edgex_protocols_dup (const edgex_protocols *e);
extern void edgex_protocols_free (edgex_protocols *e);
extern edgex_deviceservice *edgex_deviceservice_read (const char *json);
extern char *edgex_deviceservice_write (const edgex_deviceservice *e, bool create);
extern edgex_deviceservice *edgex_deviceservice_dup (const edgex_deviceservice *e);
extern edgex_device *edgex_device_read (iot_logger_t *lc, const char *json);
extern char *edgex_device_write (const edgex_device *e, bool create);
extern char *edgex_device_write_sparse (const char *name, const char *id, const char *description, const edgex_strings *labels, const char *profile_name);
extern edgex_device *edgex_device_dup (const edgex_device *e);
extern void edgex_device_free (edgex_device *e);
extern edgex_device *edgex_devices_read (iot_logger_t *lc, const char *json);

extern const edgex_cmdinfo *edgex_deviceprofile_findcommand (const char *name, edgex_deviceprofile *prof, bool forGet);
extern void edgex_device_commandresult_free (edgex_device_commandresult *res, int n);
extern char *edgex_value_tostring (const edgex_device_commandresult *value, bool binfloat);

extern edgex_nvpairs *edgex_device_getConfig (const edgex_device_service *svc);
uint16_t get_nv_config_uint16
(
  iot_logger_t *lc,
  const edgex_nvpairs *config,
  const char *key,
  edgex_error *err
);
char *get_nv_config_string
  (const edgex_nvpairs *config, const char *key);
////////////////////////////////////////////////////////////////////////////////////////
/*- tao String de gui yeu cau Discovey toi cac thiet bi.
  - NOTE: dung free() de free ket qua sau khi su dung.
 */
char *user_discovery_write (void);

/*- phan tich phan hoi Discovery tu dang JSON -> String.  
  - NOTE: dung user_discovery_free() sau khi su dung.
*/
user_discovery *user_discovery_read(const char* json_input);

/*- giai phong vung nho khi doc phan hoi Discovery.
*/
void user_discovery_free(user_discovery *e);

/*- tao String de gui toi thiet bi se duoc them vao.
  - NOTE: dung free() de free ket qua sau khi su dung.
*/
char *user_add_device_write(user_add_device *e);

/*- tao String de gui toi thiet bi khi them/ thay doi protocol cua thiet
    bi do.
  - NOTE: dung free() de free ket qua sau khi su dung.
*/
char *user_protocol_write ( const char *devname, edgex_protocols *protocols);

/*- phan tich yeu cau Push Event cua thiet bi, luu vao struct: user_push_event.
  - NOTE: dung user_push_event_free() sau khi su dung.
*/
user_push_event *user_push_event_read(edgex_device_service *svc,char *json_input);

/*- giai phong bien struct user_push_event khi su dung xong.
*/
void user_push_event_free(user_push_event *e);

/*- tao String khi Get_handler muon gui yeu cau cho cac thiet bi.
  - NOTE: dung free() de free ket qua sau khi su dung.
*/
char *user_gethandler_write(const char *devname,
                            uint32_t nreadings,
                            const edgex_device_commandrequest *requests);

/*- nhan cac Reading tu phan hoi Get_handler cua thiet bi.
  - bool cho biet phan tich co thanh cong khong.
*/
bool user_repget_event_read( edgex_device_commandresult *readings, const char *devname, 
                            const edgex_device_commandrequest *requests, 
                            uint32_t nreadings, char *json_input);

/*- tao String khi PUT_handler muon gui yeu cau cho cac thiet bi.
  - NOTE: dung free() de free ket qua sau khi su dung.
*/
char *user_puthandler_write(const char *devname,
                            uint32_t nvalues,
                            const edgex_device_commandrequest *requests,
                            const edgex_device_commandresult *values);
/*- file = "/dev/ttyACM0"
  - NOTE: use close(file) after used
 */
int user_serial_init(const char * _file, speed_t _speed);

/*- gui du lieu toi Serial
*/
int user_serial_write(int _fd, const void *__buf, size_t __n);

/*- doc du lieu tu Serial
*/
int user_serial_read(int _fd, void *_buf, size_t _nbytes);

#ifdef __cplusplus
}
#endif

#endif