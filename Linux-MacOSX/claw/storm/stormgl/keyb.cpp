//#ifdef _MSC_VER
#include "precompiled.h"
//#endif

#include <SDL.h>
#include <assert.h>
#include "../../system/Miscellaneous.h"
#include "keyb3.h"

#include "../../game/SimpleOptions.h"
#include "../../game/options/options_controllers.h"

#ifdef __GNUC__

#include "../../system/Logger.h"
#include "../../system/Miscellaneous.h"

#endif  //  __GNUC__

// Nappien maksimim‰‰r‰t
#define MAX_MOUSEBUTTONS 8
#define MAX_JOYBUTTONS 32
#define MAX_JOYSTICKS 4

int JoyNum=0;	// Joystikkejen m‰‰r‰

static int keylookup[] = {
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	KEYCODE_BACKSPACE, /* backspace */
	KEYCODE_TAB, /* tab */
	0, /* unknown key */
	0, /* unknown key */
	0, /* clear */
	KEYCODE_ENTER, /* return */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* pause */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	KEYCODE_ESC, /* escape */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	KEYCODE_SPACE, /* space */
	0, /* ! */
	0, /* " */
	0, /* # */
	0, /* $ */
	0, /* unknown key */
	0, /* & */
	0, /* ' */
	0, /* ( */
	0, /* ) */
	0, /* * */
	0, /* + */
	0, /* , */
	0, /* - */
	0, /* . */
	0, /* / */
	KEYCODE_0, /* 0 */
	KEYCODE_1, /* 1 */
	KEYCODE_2, /* 2 */
	KEYCODE_3, /* 3 */
	KEYCODE_4, /* 4 */
	KEYCODE_5, /* 5 */
	KEYCODE_6, /* 6 */
	KEYCODE_7, /* 7 */
	KEYCODE_8, /* 8 */
	KEYCODE_9, /* 9 */
	0, /* : */
	0, /* ; */
	0, /* < */
	0, /* = */
	0, /* > */
	0, /* ? */
	0, /* @ */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* [ */
	0, /* \ */
	0, /* ] */
	0, /* ^ */
	0, /* _ */
	0, /* ` */
	KEYCODE_A, /* a */
	KEYCODE_B, /* b */
	KEYCODE_C, /* c */
	KEYCODE_D, /* d */
	KEYCODE_E, /* e */
	KEYCODE_F, /* f */
	KEYCODE_G, /* g */
	KEYCODE_H, /* h */
	KEYCODE_I, /* i */
	KEYCODE_J, /* j */
	KEYCODE_K, /* k */
	KEYCODE_L, /* l */
	KEYCODE_M, /* m */
	KEYCODE_N, /* n */
	KEYCODE_O, /* o */
	KEYCODE_P, /* p */
	KEYCODE_Q, /* q */
	KEYCODE_R, /* r */
	KEYCODE_S, /* s */
	KEYCODE_T, /* t */
	KEYCODE_U, /* u */
	KEYCODE_V, /* v */
	KEYCODE_W, /* w */
	KEYCODE_X, /* x */
	KEYCODE_Y, /* y */
	KEYCODE_Z, /* z */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	KEYCODE_DELETE, /* delete */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	0, /* world 0 */
	0, /* world 1 */
	0, /* world 2 */
	0, /* world 3 */
	0, /* world 4 */
	0, /* world 5 */
	0, /* world 6 */
	0, /* world 7 */
	0, /* world 8 */
	0, /* world 9 */
	0, /* world 10 */
	0, /* world 11 */
	0, /* world 12 */
	0, /* world 13 */
	0, /* world 14 */
	0, /* world 15 */
	0, /* world 16 */
	0, /* world 17 */
	0, /* world 18 */
	0, /* world 19 */
	0, /* world 20 */
	0, /* world 21 */
	0, /* world 22 */
	0, /* world 23 */
	0, /* world 24 */
	0, /* world 25 */
	0, /* world 26 */
	0, /* world 27 */
	0, /* world 28 */
	0, /* world 29 */
	0, /* world 30 */
	0, /* world 31 */
	0, /* world 32 */
	0, /* world 33 */
	0, /* world 34 */
	0, /* world 35 */
	0, /* world 36 */
	0, /* world 37 */
	0, /* world 38 */
	0, /* world 39 */
	0, /* world 40 */
	0, /* world 41 */
	0, /* world 42 */
	0, /* world 43 */
	0, /* world 44 */
	0, /* world 45 */
	0, /* world 46 */
	0, /* world 47 */
	0, /* world 48 */
	0, /* world 49 */
	0, /* world 50 */
	0, /* world 51 */
	0, /* world 52 */
	0, /* world 53 */
	0, /* world 54 */
	0, /* world 55 */
	0, /* world 56 */
	0, /* world 57 */
	0, /* world 58 */
	0, /* world 59 */
	0, /* world 60 */
	0, /* world 61 */
	0, /* world 62 */
	0, /* world 63 */
	0, /* world 64 */
	0, /* world 65 */
	0, /* world 66 */
	0, /* world 67 */
	0, /* world 68 */
	0, /* world 69 */
	0, /* world 70 */
	0, /* world 71 */
	0, /* world 72 */
	0, /* world 73 */
	0, /* world 74 */
	0, /* world 75 */
	0, /* world 76 */
	0, /* world 77 */
	0, /* world 78 */
	0, /* world 79 */
	0, /* world 80 */
	0, /* world 81 */
	0, /* world 82 */
	0, /* world 83 */
	0, /* world 84 */
	0, /* world 85 */
	0, /* world 86 */
	0, /* world 87 */
	0, /* world 88 */
	0, /* world 89 */
	0, /* world 90 */
	0, /* world 91 */
	0, /* world 92 */
	0, /* world 93 */
	0, /* world 94 */
	0, /* world 95 */
	KEYCODE_KEYPAD_0, /* [0] */
	KEYCODE_KEYPAD_1, /* [1] */
	KEYCODE_KEYPAD_2, /* [2] */
	KEYCODE_KEYPAD_3, /* [3] */
	KEYCODE_KEYPAD_4, /* [4] */
	KEYCODE_KEYPAD_5, /* [5] */
	KEYCODE_KEYPAD_6, /* [6] */
	KEYCODE_KEYPAD_7, /* [7] */
	KEYCODE_KEYPAD_8, /* [8] */
	KEYCODE_KEYPAD_9, /* [9] */
	KEYCODE_KEYPAD_DOT, /* [.] */
	KEYCODE_KEYPAD_DIVIDE, /* [/] */
	KEYCODE_KEYPAD_MULTIPLY, /* [*] */
	KEYCODE_KEYPAD_MINUS, /* [-] */
	KEYCODE_KEYPAD_PLUS, /* [+] */
	KEYCODE_ENTER, /* enter */
	0, /* equals */
	KEYCODE_UP_ARROW, /* up */
	KEYCODE_DOWN_ARROW, /* down */
	KEYCODE_RIGHT_ARROW, /* right */
	KEYCODE_LEFT_ARROW, /* left */
	KEYCODE_INSERT, /* insert */
	0, /* home */
	0, /* end */
	KEYCODE_PAGE_UP, /* page up */
	KEYCODE_PAGE_DOWN, /* page down */
	KEYCODE_F1, /* f1 */
	KEYCODE_F2, /* f2 */
	KEYCODE_F3, /* f3 */
	KEYCODE_F4, /* f4 */
	KEYCODE_F5, /* f5 */
	KEYCODE_F6, /* f6 */
	KEYCODE_F7, /* f7 */
	KEYCODE_F8, /* f8 */
	KEYCODE_F9, /* f9 */
	KEYCODE_F10, /* f10 */
	KEYCODE_F11, /* f11 */
	11, /* f12 */
	0, /* f13 */
	0, /* f14 */
	0, /* f15 */
	0, /* unknown key */
	0, /* unknown key */
	0, /* unknown key */
	KEYCODE_NUMLOCK, /* numlock */
	KEYCODE_CAPSLOCK, /* caps lock */
	KEYCODE_SCROLLLOCK, /* scroll lock */
	KEYCODE_SHIFT_RIGHT, /* right shift */
	KEYCODE_SHIFT_LEFT, /* left shift */
	KEYCODE_CONTROL_RIGHT, /* right ctrl */
	KEYCODE_CONTROL_LEFT, /* left ctrl */
	0, /* right alt */
	0, /* left alt */
	0, /* right meta */
	0, /* left meta */
	0, /* left super */
	0, /* right super */
	KEYCODE_ALT_GR, /* alt gr */
	0, /* compose */
	0, /* help */
	KEYCODE_PRINTSCREEN, /* print screen */
	0, /* sys req */
	0, /* break */
	0, /* menu */
	0, /* power */
	0, /* e*-uro */
	0, /* undo */
};

#define MAX_KBS BASIC_KEYCODE_AMOUNT + ADDITIONAL_KEYBOARD_KEYS_AMOUNT //500
// FIXME: this is fucking ugly and breaks all kinds of encapsulation rules...
#define JOYSTICK_THRESHOLD game::SimpleOptions::getInt(DH_OPT_I_JOYSTICK1_DEADZONE + i)

struct nappis1
{
	unsigned char keysdown[MAX_KBS];	// mitk‰ napit pohjassa
	unsigned char exkd[MAX_KBS];		// entiset napit, viime
										// UpdateInputDevices:n k‰yttˆ‰ 
										// ennen, t‰t‰ voit k‰ytt‰‰
										// painallusten/irrotusten seuraamiseen
	unsigned char getpress[MAX_KBS];	// mink‰ nappien painallukset on jo "k‰sitelty" (GetKeypress)
	unsigned char getrelease[MAX_KBS];	// mink‰ nappien releaset on jo "k‰sitelty" (GetRelease)
};

struct hiiri1
{
	int x,y;	//kursorin koordinaatit
    int oldx, oldy;
	int dx,dy;	//kursorin deltakoordinaatit (vauhdit)

	int rulla;	//rullan delta "koordinaatti"
	
	int max_x,max_y;	// Hiiren reunat

	unsigned char nappi[MAX_MOUSEBUTTONS];	//napit
};

struct joy1
{
	SDL_Joystick *sdljoy;
	int x,y;	//tikun koordinaatit

	// Lis‰-akselit
	int rot_x,rot_y;
	int throttle,rudder;		

	unsigned char nappi[MAX_JOYBUTTONS];	//napit 
};

// Globaalit devicet
nappis1 nappis;
hiiri1 hiiri;
joy1 *joys;
int joyCount;
ui::GameController *gc;


// Inits/frees Keyb3 Control System
extern "C" int Keyb3_Init(HWND hw, uint32_t CAPS) {		// Returns: TRUE=ok, FALSE=error
	// FIXME: ignores CAPS

	SDL_InitSubSystem(SDL_INIT_JOYSTICK);
	joyCount = SDL_NumJoysticks();
	if (joyCount > 4) // handle max 4 joysticks
		joyCount = 4;
	joys = new joy1[joyCount];

	for (int i = 0; i < joyCount; i++ ){
		joys[i].sdljoy = SDL_JoystickOpen(i);
		joys[i].x = 0;
		joys[i].y = 0;
		joys[i].rot_x = 0;
		joys[i].rot_y = 0;
		joys[i].throttle = 0;
		joys[i].rudder = 0;
		memset(joys[i].nappi, 0, sizeof(unsigned char) * MAX_JOYBUTTONS);
		//memset(joys[i].pov, 0, sizeof(int) * MAX_JOYPOV);

		// Debug
#ifdef  __GNUC__
		LOG_INFO(strPrintf("Joystick info for %s", SDL_JoystickName(i)).c_str());
		LOG_INFO(strPrintf("Number of axes: %i", SDL_JoystickNumAxes(joys[i].sdljoy)).c_str());
		LOG_INFO(strPrintf("Number of balls: %i", SDL_JoystickNumBalls(joys[i].sdljoy)).c_str());
		LOG_INFO(strPrintf("Number of hats: %i", SDL_JoystickNumHats(joys[i].sdljoy)).c_str());
		LOG_INFO(strPrintf("Number of buttons: %i", SDL_JoystickNumButtons(joys[i].sdljoy)).c_str());
#endif  //  __GNUC__
	}
	SDL_JoystickEventState(SDL_ENABLE);

	JoyNum = joyCount;

	return true;
}

extern "C" void Keyb3_Free() {
	if (joys != NULL) {
		for (int i = 0; i < joyCount; i++)
			if (SDL_JoystickOpened(i))
				SDL_JoystickClose(joys[i].sdljoy);

		joyCount = 0;
		delete[] joys;
		joys = NULL;
	}
	SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
}

// Activates/Deactivates Keyb3 Control System.
// Deactivate Keyb3 when your window gets minimized.
// Activate Keyb3 when your window gets active again.
extern "C" void Keyb3_SetActive(bool on) {
	SDL_WM_GrabInput(on ? SDL_GRAB_ON : SDL_GRAB_OFF);
	// for debugging so we don't steal focus from gdb
	SDL_ShowCursor(on ? SDL_DISABLE : SDL_ENABLE);
}

#define JOY_UP      (KEYCODE_JOY_UP      - KEYCODE_JOY_UP)
#define JOY_DOWN    (KEYCODE_JOY_DOWN    - KEYCODE_JOY_UP)
#define JOY_LEFT    (KEYCODE_JOY_LEFT    - KEYCODE_JOY_UP)
#define JOY_RIGHT   (KEYCODE_JOY_RIGHT   - KEYCODE_JOY_UP)
#define JOY_BUTTON1 (KEYCODE_JOY_BUTTON1 - KEYCODE_JOY_UP)

extern "C" void Keyb3_AddController(ui::GameController *_gc) {
	gc = _gc;
	SDL_EnableUNICODE((gc != NULL) ? 1 : 0);
}

// Updates all devices
// Use this function frequently (it's best to be placed in your programs main loop)
extern "C" void Keyb3_UpdateDevices() {
	memset(nappis.getpress, 0, MAX_KBS * sizeof(unsigned char));
	memset(nappis.getrelease, 0, MAX_KBS * sizeof(unsigned char));
	memcpy(nappis.exkd, nappis.keysdown, MAX_KBS * sizeof(unsigned char));

	nappis.keysdown[KEYCODE_MOUSE_WHEEL_DOWN] = false;
	nappis.keysdown[KEYCODE_MOUSE_WHEEL_UP] = false;

	hiiri.oldx = hiiri.x; hiiri.oldy = hiiri.y;

	SDL_Event event;
	int joyNum;
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_KEYDOWN:
			if (event.key.keysym.sym == SDLK_RETURN) {
				SDLMod m = SDL_GetModState();
				if ((m & KMOD_ALT) != 0) {
					SDL_WM_ToggleFullScreen(SDL_GetVideoSurface());
					continue;
				}
			} else if (event.key.keysym.sym == SDLK_g) {
				SDLMod m = SDL_GetModState();
				if ((m & KMOD_CTRL) != 0) {
					SDL_GrabMode curr = SDL_WM_GrabInput(SDL_GRAB_QUERY);
					Keyb3_SetActive(curr == SDL_GRAB_OFF);
					continue;
				}
			} else if (event.key.keysym.sym == SDLK_c) {
				SDLMod m = SDL_GetModState();
				if ((m & KMOD_CTRL) != 0) {
					SDL_Quit();
					//gc->suicide();
				}
			}
#ifdef __APPLE__
			else if ((KMOD_LMETA & event.key.keysym.mod) && event.key.keysym.sym == SDLK_q) {
				SDL_Quit();
				//gc->suicide();
			}
#endif

			if (gc != NULL)
			{
				if (keylookup[event.key.keysym.sym] != 0)
					gc->addReadKey(0, keylookup[event.key.keysym.sym]);
				else
					gc->addReadKey(event.key.keysym.unicode, 0);
			}
			nappis.keysdown[keylookup[event.key.keysym.sym]] = 1;
			break;
		case SDL_KEYUP:
			nappis.keysdown[keylookup[event.key.keysym.sym]] = 0;
			break;
		case SDL_JOYAXISMOTION:
			joyNum = event.jaxis.which;
			if (joyNum >= 0 && joyNum < joyCount) {
				int joyValue = (int) ((float(event.jaxis.value)) / 32768.0 * 1000.0);
				if (event.jaxis.axis == 0) {
					joys[joyNum].x = joyValue;
				} else if (event.jaxis.axis == 1) {
					joys[joyNum].y = joyValue;
				} else if (event.jaxis.axis == 2) {
					joys[joyNum].rot_x = joyValue;
				} else if (event.jaxis.axis == 3) {
					joys[joyNum].rot_y = joyValue;
				} else if (event.jaxis.axis == 4) {
					joys[joyNum].throttle = joyValue;
				} else if (event.jaxis.axis == 5) {
					joys[joyNum].rudder = joyValue;
				}
			}
			break;
		case SDL_JOYHATMOTION:
			joyNum = event.jhat.which;
			if (joyNum >= 0 && joyNum < joyCount) {
				/*
				for(int ii=0; ii < MAX_JOYPOV; ii++)
					joys[joyNum].pov[ii] = 0;
				if ((event.jhat.value & SDL_HAT_UP) != 0) {
					joys[joyNum].pov[0] = 1;
				} else if ((event.jhat.value & SDL_HAT_DOWN) != 0) {
					joys[joyNum].pov[1] = 1;
				} else if ((event.jhat.value & SDL_HAT_LEFT) != 0) {
					joys[joyNum].pov[2] = 1;
				} else if ((event.jhat.value & SDL_HAT_RIGHT) != 0) {
					joys[joyNum].pov[3] = 1;
				}
				*/
			}
			break;
		case SDL_JOYBUTTONDOWN: // fallthrough
		case SDL_JOYBUTTONUP:
      {
        joyNum = event.jbutton.which;
        int buttonNum = event.jbutton.button;
        if (joyNum >= 0 && joyNum < joyCount && buttonNum >= 0 && buttonNum < MAX_JOYBUTTONS)
          joys[joyNum].nappi[buttonNum] = (event.jbutton.state == SDL_PRESSED) ? 1 : 0;
        break;
      }
		case SDL_MOUSEMOTION:
			{
				hiiri.x = event.motion.x; hiiri.y = event.motion.y;
				//hiiri.dx = event.motion.xrel; hiiri.dy = event.motion.yrel;

				break;
			}

		case SDL_MOUSEBUTTONDOWN: // fallthrough
		case SDL_MOUSEBUTTONUP:
			{
				static unsigned int call = 0;
				++call;
				unsigned int b;
				switch (event.button.button) {
				case SDL_BUTTON_MIDDLE:
					b = KEYCODE_MOUSE_BUTTON3;
					break;

				case SDL_BUTTON_RIGHT:
					b = KEYCODE_MOUSE_BUTTON2;
					break;

				case SDL_BUTTON_WHEELDOWN:
					b = KEYCODE_MOUSE_WHEEL_DOWN;
					event.button.state = SDL_PRESSED;
					break;

				case SDL_BUTTON_WHEELUP:
					b = KEYCODE_MOUSE_WHEEL_UP;
					event.button.state = SDL_PRESSED;
					break;

				case SDL_BUTTON_LEFT: // fallthrough
				default:
					b = KEYCODE_MOUSE_BUTTON1;
					break;
				}
				nappis.keysdown[b] = (event.button.state == SDL_PRESSED);
				break;
			}
    case SDL_QUIT:
      exit(0);
		}
	}

	// comment this out to break shit
	hiiri.dx = hiiri.x - hiiri.oldx; hiiri.dy = hiiri.y - hiiri.oldy;

	nappis.keysdown[KEYCODE_MOUSE_UP]         = (hiiri.dy < 0);
	nappis.keysdown[KEYCODE_MOUSE_DOWN]       = (hiiri.dy > 0);
	nappis.keysdown[KEYCODE_MOUSE_LEFT]       = (hiiri.dx < 0);
	nappis.keysdown[KEYCODE_MOUSE_RIGHT]      = (hiiri.dx > 0);

	for (int i = 0; i < joyCount; i++) {
		int baseKeyCode = KEYCODE_JOY_UP;

		switch (i) {
			case 0:
				baseKeyCode = KEYCODE_JOY_UP;
				break;
			case 1:
				baseKeyCode = KEYCODE_JOY2_UP;
				break;
			case 2:
				baseKeyCode = KEYCODE_JOY3_UP;
				break;
			case 3:
				baseKeyCode = KEYCODE_JOY4_UP;
				break;
		}

		nappis.keysdown[baseKeyCode + JOY_UP] = (joys[i].y < -JOYSTICK_THRESHOLD);
		nappis.keysdown[baseKeyCode + JOY_DOWN] = (joys[i].y > JOYSTICK_THRESHOLD);
		nappis.keysdown[baseKeyCode + JOY_LEFT] = (joys[i].x < -JOYSTICK_THRESHOLD);
		nappis.keysdown[baseKeyCode + JOY_RIGHT] = (joys[i].x > JOYSTICK_THRESHOLD);

        /*
		LOG_DEBUG(strPrintf("joy %d x y %d %d buttons %d %d %d %d"
							, i, joys[i].x, joys[i].y
							, nappis.keysdown[baseKeyCode + JOY_UP]
							, nappis.keysdown[baseKeyCode + JOY_DOWN]
							, nappis.keysdown[baseKeyCode + JOY_LEFT]
							, nappis.keysdown[baseKeyCode + JOY_RIGHT]
						   ).c_str()
				 );
        */
		for (int j = 0; j < 16; j++)
			nappis.keysdown[baseKeyCode + JOY_BUTTON1 + j] = joys[i].nappi[j];
		nappis.keysdown[KEYCODE_JOY_ROT_UP] = joys[i].rot_y < -JOYSTICK_THRESHOLD;
		nappis.keysdown[KEYCODE_JOY_ROT_DOWN] = joys[i].rot_y > JOYSTICK_THRESHOLD;
		nappis.keysdown[KEYCODE_JOY_ROT_LEFT] = joys[i].rot_x < -JOYSTICK_THRESHOLD;
		nappis.keysdown[KEYCODE_JOY_ROT_RIGHT] = joys[i].rot_x > JOYSTICK_THRESHOLD;

		// POV
		/*
		nappis.keysdown[KEYCODE_JOY_POV_UP] = joys[i].pov[0];
		nappis.keysdown[KEYCODE_JOY_POV_DOWN] = joys[i].pov[1];
		nappis.keysdown[KEYCODE_JOY_POV_LEFT] = joys[i].pov[2];
		nappis.keysdown[KEYCODE_JOY_POV_RIGHT] = joys[i].pov[3];
		*/
	}

}

// Updates all devices. Updating devices uses a lot of processor time, so this
// function updates devices only if "int time" amount of time is elapsed.
extern "C" void Keyb3_UpdateDevices_Optimized(int time) {	// time in 1/1000 secs (1000=1 second)
	static int last = 0;
	int now = SDL_GetTicks();

	if ((now - last) > time) {
		Keyb3_UpdateDevices();
		last = now;
	}
}

// Waits for a keypress, and returns keycode 
extern "C" int Keyb3_WaitKeypress(bool returnIndividualMouse) {
	int OK = -1;
	Keyb3_UpdateDevices();

	do {
		Keyb3_UpdateDevices();

		for (int i = 0; i < MAX_KBS; i++)
			if (nappis.keysdown[i] > nappis.exkd[i]) OK = i;
	} while (OK < 0);

	return OK;
}

// Checks if key is down (returns TRUE if down, FALSE is not)
extern "C" int Keyb3_IsKeyDown(int keycode) {
	assert(keycode < MAX_KBS);
	if (nappis.keysdown[keycode]) return 1;

	return 0;
}

// Gets keypress(/release), and clears the keypress(/release) mark. (returns TRUE if pressed, FALSE is not)
extern "C" int Keyb3_GetKeyPress(int keynum) {
	int ok = 0;

	if (nappis.exkd[keynum]) return 0;
	if (nappis.keysdown[keynum]) ok = 1;
	if (nappis.getpress[keynum]) ok = 0;
	nappis.getpress[keynum] = 1;

	return ok;
}

extern "C" int Keyb3_GetKeyRelease(int keynum) {
	int ok = 0;

	if (nappis.keysdown[keynum]) return 0;
	if (nappis.exkd[keynum]) ok = 1;
	if (nappis.getrelease[keynum]) ok = 0;
	nappis.getrelease[keynum] = 1;

	return ok;
}

// Checks is key state is changed from last update (returns TRUE if changed, FALSE is not)
extern "C" int Keyb3_IsKeyPressed(int keynum) {
	if (nappis.exkd[keynum]) return 0;
	if (nappis.keysdown[keynum]) return 1;

	return 0;
}

extern "C" int Keyb3_IsKeyReleased(int keynum) {
	if (nappis.keysdown[keynum]) return 0;
	if (nappis.exkd[keynum]) return 1;

	return 0;
}

// Mouse setup
extern "C" void Keyb3_SetMouseBorders(int max_x, int max_y, int mouseID) {
	hiiri.max_x = max_x; hiiri.max_y = max_y;
}

extern "C" void Keyb3_SetMousePos(int x, int y, int mouseID) {
	SDL_GrabMode curr = SDL_WM_GrabInput(SDL_GRAB_QUERY);
	if (curr == SDL_GRAB_ON) {
		hiiri.x = x; hiiri.y = y;
		hiiri.oldx = x; hiiri.oldy = y;
		hiiri.dx = 0; hiiri.dy = 0;
		SDL_WarpMouse(x, y);
	}
}

extern "C" void Keyb3_ReleaseMouseBorders() {
}

// Direct mouse read (give a NULL-pointer if you don't want some information) 
extern "C" void Keyb3_ReadMouse(int *x, int *y, int *dx, int *dy, int mouseID) {
	if (x != NULL) *x = hiiri.x;
	if (y != NULL) *y = hiiri.y;
	if (dx != NULL) *dx = hiiri.dx;
	if (dy != NULL) *dy = hiiri.dy;
}

// Direct joystick read. (Axis: [-1000,1000], 0=center)
// (joynum: 0=first joystick, 1=second joystick)
// (give a NULL-pointer if you don't want some information) 
extern "C" void Keyb3_ReadJoystick(int joynum, int *x, int *y, int *rx, int *ry, int *throttle, int *rudder) {
	if (joynum >= 0 && joynum < joyCount) {
		if (x != NULL) (*x) = joys[joynum].x;
		if (y != NULL) (*y) = joys[joynum].y;
		if (rx != NULL) (*rx) = joys[joynum].rot_x;
		if (ry != NULL) (*ry) = joys[joynum].rot_y;
		if (throttle != NULL) (*throttle) = joys[joynum].throttle;
		if (rudder != NULL) (*rudder) = joys[joynum].rudder;
	}
}

extern "C" int Keyb3_GetNumberOfMouseDevices() {
	return 1;
    // NOTICE: counts the "every mouse" -handle as a device as well,
    // which is the last mouseID. see MouseHandler.h for more info.
}


extern "C" int Keyb3_DefaultMouseID() {
	// Mouse ID that is controlled by every mouse for RawInput. 0 for DirectInput.
	return 0;
}


extern "C" bool RawInputDeviceHandler::isInitialized() {
	// FIXME
	return false;
}

extern "C" std::string RawInputDeviceHandler::getError() {
	// FIXME
	return "";
}

extern "C" int RawInputDeviceHandler::getNumOfMouses() {
	// FIXME
	return 1;
}

extern "C" int RawInputDeviceHandler::getNumOfKeyboards() {
	// FIXME
	return 1;
}
