#ifndef HEADER_EVENTKEYBOARD
#define HEADER_EVENTKEYBOARD


#define RIGHT_STICK_LEFT	0	//	Keyboard : a		Xbox Controller : Joystick 2 (position left)   
#define RIGHT_STICK_RIGHT	1	//	Keyboard : d		Xbox Controller : Joystick 2 (position right)
#define BACK_BUTTON		2	//	Keyboard : l		Xbox Controller : Button back
#define XBOX_BUTTON		3	//	Keyboard : r		Xbox Controller : Button Xbox
#define RIGHT_STICK_DOWN	4	//	Keyboard : s		Xbox Controller : Joystick 2 (position down)
#define START_BUTTON		5	//	Keyboard : t		Xbox Controller : Button start
#define RIGHT_STICK_UP		6	//	Keyboard : w		Xbox Controller : Joystick 2 (position up)
#define LEFT_STICK_UP		7	//	Keyboard : top arrow	Xbox Controller : Joystick 1 (position up)
#define LEFT_STICK_DOWN		8	//	Keyboard : down arrow	Xbox Controller : Joystick 1 (position down)
#define LEFT_STICK_RIGHT	9	//	Keyboard : rigth arrow	Xbox Controller : Joystick 1 (position right)
#define LEFT_STICK_LEFT		10	//	Keyboard : left arrow	Xbox Controller : Joystick 1 (position left)
#define A_BUTTON		11	//	Keyboard : m		Xbox Controller : Button A
#define Y_BUTTON		13	//	Keyboard : escape	Xbox Controller : Button Y
#define DIRECTIONNAL_PAD_UP	14	//	Keyboard : 1		Xbox Controller : Hat (position up)
#define DIRECTIONNAL_PAD_RIGHT	15	//	Keyboard : 2		Xbox Controller : Hat (position right)
#define DIRECTIONNAL_PAD_DOWN	16	//	Keyboard : 3		Xbox Controller : Hat (position down)
#define DIRECTIONNAL_PAD_LEFT	17	//	Keyboard : 4		Xbox Controller : Hat (position left)
#define X_BUTTON		18	//	Keyboard : none		Xbox Controller : Button X
#define RIGHT_TRIGGER		19	//	Keyboard : none		Xbox Controller : Trigger RT
#define LEFT_TRIGGER		20	//	Keyboard : none		Xbox Controller : Trigger LT
#define RIGHT_BUMPER		21	//	Keyboard : none		Xbox Controller : Button RB

#include <SDL/SDL.h>

int threadEventKeyboard(void *data);

class EventKeyboard
{
	public:

	EventKeyboard();

	void updateEvent();
	bool getKey(int i);
	void setKey(bool val,int i);
	SDL_Surface* getSDLSurface();

	void mutexP();
	void mutexV();
	void destroyMutex();
	bool getQuit();
	void shutDown();

	private:

	bool key[23]; // true == pressed, false == released
	bool quit;
	
	SDL_mutex* mutexKeyData;
	SDL_Event event;
	SDL_Surface* surface;
	SDL_Joystick *joystick;

};


#endif

