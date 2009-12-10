#ifndef CONFIG_H_
#define CONFIG_H_

#define cfg_MAX_NAME_LENGTH		8
#define cfg_MAX_PARENT_GROUPS	10

typedef struct cfg_Device_s {
	unsigned long identity;
	unsigned short nmea_Address;
	struct cfg_Device_s *next;
} cfg_Device_t;


typedef struct {
	unsigned char name[cfg_MAX_NAME_LENGTH];
	struct cfg_Device_s *devices;
} cfg_Group_t;

typedef struct cfg_Event_s {
	unsigned char group;
	unsigned char ctrlDev;
	unsigned char ctrlFunc;
	unsigned char ctrlEvent;
	unsigned char function;
	struct cfg_Event_s *next;
} config_Event_t;

extern config_Event_t *config_MyEvents;
extern unsigned char config_Valid;

void config_AddControlEvents( 
	unsigned char group, 
	unsigned char *fromCfgPtr, 
	unsigned char func
);

void config_AddControlEvent( 
	unsigned char group, 
	unsigned char ctrlDev, 
	unsigned char ctrlFunc, 
	unsigned char ctrlEvent, 
	unsigned char function
);

void config_Initialize();
void config_Task();

#endif /* CONFIG_H_ */
