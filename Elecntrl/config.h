#ifndef CONFIG_H_
#define CONFIG_H_

#define 	END_OF_FILE		0xFF
#define 	DELIMITER		0xFE	// End of Group Entry
#define 	hw_DEVICE_ANY	0xFF
#define 	config_GROUP_UNDEFINED	0xFF

extern unsigned char config_Invalid;
extern unsigned char config_CurrentGroup;

void config_AddControlEvent( 
	const unsigned char ctrlGroup,
	const unsigned char ctrlEvent, 
	const unsigned char ctrlAction
);

void config_Initialize();
void config_UninitializedTask();
void config_Update( unsigned short configBytes );
unsigned char config_GetGroupIdForPort( unsigned char port );
unsigned char config_GetPortActionFromEvent( unsigned char port, event_t* event );

#endif /* CONFIG_H_ */
