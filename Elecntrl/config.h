#ifndef CONFIG_H_
#define CONFIG_H_

#define config_MAX_NAME_LENGTH			8
#define config_MAX_PARENT_GROUPS		10

#define config_MAX_TARGETS_PER_GROUP	10

#define 	endConfig	0xFF
#define 	config_GroupEnd	0xFE	// End of Group Entry

typedef struct cfg_Device_s {
	unsigned long identity;
	unsigned short nmea_Address;
	struct cfg_Device_s *next;
} config_Device_t;


typedef struct {
	unsigned char name[config_MAX_NAME_LENGTH];
	struct cfg_Device_s *devices;
} config_Group_t;

typedef struct cfg_Event_s {
	unsigned char ctrlDev;
	unsigned char ctrlFunc;
	unsigned char ctrlEvent;
	unsigned char ctrlAction;
	unsigned char function;
	struct cfg_Event_s *next;
} config_Event_t;

typedef const unsigned char *config_File_t;


extern config_Event_t *config_MyEvents;
extern unsigned char config_Invalid;

void config_AddMyControlEvents( 
	unsigned char group, 
	const unsigned char *fromCfgPtr, 
	unsigned char func
);

void config_AddControlEvent( 
	const unsigned char ctrlDev, 
	const unsigned char ctrlFunc, 
	const unsigned char ctrlEvent, 
	const unsigned char ctrlAction,
	const unsigned char function
);

void config_Initialize();
void config_Task();
void config_Update( unsigned short configBytes );
config_File_t config_FileFindGroup( unsigned char groupId );

inline unsigned char _findNextGroup( config_File_t* filePointer );
inline unsigned char _findNextTarget( config_File_t* filePointer );
inline unsigned char _findControllers( config_File_t* filePointer );

#endif /* CONFIG_H_ */
