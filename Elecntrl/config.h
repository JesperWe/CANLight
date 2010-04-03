#ifndef CONFIG_H_
#define CONFIG_H_

#define 	END_OF_FILE	0xFF
#define 	DELIMITER	0xFE	// End of Group Entry

typedef struct cfg_Event_s {
	unsigned char ctrlGroup;
	unsigned char ctrlEvent;
	unsigned char ctrlAction;
	struct cfg_Event_s *next;
} config_Event_t;


extern config_Event_t *config_MyEvents;
extern unsigned char config_Invalid;
extern unsigned char functionInGroup[];
extern unsigned char functionListenGroup[];

void config_AddControlEvent( 
	const unsigned char ctrlGroup,
	const unsigned char ctrlEvent, 
	const unsigned char ctrlAction
);

void config_Initialize();
void config_Task();
void config_Update( unsigned short configBytes );

#endif /* CONFIG_H_ */
