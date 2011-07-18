extern unsigned char ballast_State;
extern unsigned char ballast_Transitioning;

void ballast_KeyClick( unsigned char key, unsigned char event );
void ballast_GoToState( unsigned char state );
void ballast_ShutOffTask();
