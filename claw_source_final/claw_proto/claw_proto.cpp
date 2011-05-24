
//
// Dha super hyper Storm3D programming test
//
// (Now known as the gpdemo dev version ;)
//

#include "precompiled.h"

#ifdef CHANGE_TO_PARENT_DIR
#include <direct.h>
#endif

#include <string.h>
#include <time.h>
#include <stdio.h>
//#include <vector> // include std
#include <Storm3D_UI.h>
#include <keyb3.h>

#include "version.h"
#include "configuration.h"

#include "../system/Logger.h"
//#include "../util/Parser.h"
#include "../editor/Parser.h"
#include "../util/Checksummer.h"
#include "../sound/SoundLib.h"
#include "../sound/SoundMixer.h"
#include "../sound/MusicPlaylist.h"

#include "../ogui/Ogui.h"
#include "../ogui/OguiStormDriver.h"

#include "../ui/uidefaults.h"
#include "../ui/Visual2D.h"
#include "../ui/VisualObjectModel.h"
#include "../ui/Animator.h"
#include "../ui/ErrorWindow.h"
#include "../ui/SelectionBox.h"
#include "../ui/cursordefs.h"
#include "../ui/GameVideoPlayer.h"
#include "../ui/LoadingMessage.h"
#include "../ui/DebugDataView.h"

#include "../convert/str2int.h"

#include "../game/Game.h"
#include "../game/createparts.h"
#include "../game/unittypes.h"
#include "../game/GameOptionManager.h"
#include "../game/SimpleOptions.h"
#include "../game/options/options_all.h"
#include "../game/PlayerWeaponry.h"

// TEMP
//#include "../game/CoverMap.h"
#include "../game/DHLocaleManager.h"

#include "../game/GameRandom.h"
#include "../game/GameUI.h"
#include "../ui/GameController.h"
#include "../ui/GameCamera.h"
#include "../ui/UIEffects.h"
#include "../util/ScriptManager.h"
#include "../game/OptionApplier.h"

#include "../system/Timer.h"
#include "../system/SystemRandom.h"
#include <istorm3d_logger.h>

#ifdef SCRIPT_DEBUG
#include "../game/ScriptDebugger.h"
#endif

#include "../util/procedural_properties.h"
#include "../util/procedural_applier.h"
#include "../util/assert.h"
#include "../util/Debug_MemoryManager.h"

#ifndef USE_DSOUND
#pragma comment(lib, "fmodvc.lib")
#endif

//#include "../filesystem/zip_package.h"
//#include "../filesystem/input_stream.h"
#include "../filesystem/ifile_package.h"
#include "../filesystem/standard_package.h"
#include "../filesystem/zip_package.h"
#include "../filesystem/file_package_manager.h"
#include "../filesystem/file_list.h"

#include "../util/mod_selector.h"

#include "../project_common/configuration_post_check.h"

using namespace game;
using namespace ui;
using namespace sfx;
using namespace frozenbyte;

// TEMP!!!
/*
namespace ui
{
extern int visual_object_allocations;
extern int visual_object_model_allocations;
}
*/

int interface_generation_counter = 0;
bool next_interface_generation_request = false;
bool apply_options_request = false;
IStorm3D_Scene *disposable_scene = NULL;

// for perf stats..
int disposable_frames_since_last_clear = 0;
int disposable_ticks_since_last_clear = 0;

GameUI *msgproc_gameUI = NULL;

extern const char *version_branch_name;


int scr_width = 0;
int scr_height = 0;


bool lostFocusPause = false;

// TEMP
//IStorm3D *disposable_s3d = NULL;


int renderfunc(IStorm3D_Scene *scene)
{
	return scene->RenderScene();
}

LRESULT WINAPI customMessageProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	if(msg == WM_ACTIVATE)
	{
		int active = LOWORD(wParam);
		int minimized = HIWORD(wParam);

		if(active == WA_INACTIVE)
		{
			//ShowWindow(hWnd, SW_MINIMIZE);
		}
		else
		{
			//if(minimized)
			//	ShowWindow(hWnd, SW_RESTORE);
		}

		if(active == WA_INACTIVE || minimized)
			lostFocusPause = true;
		else 
			lostFocusPause = false;
	}

	if(msg == WM_KEYDOWN)
	{
		int key = int(wParam);

		if (msgproc_gameUI != NULL
			&& msgproc_gameUI->getController(0) != NULL)
		{
			if(key == VK_LEFT)
				msgproc_gameUI->getController(0)->addReadKey(0, KEYCODE_LEFT_ARROW);
			else if(key == VK_RIGHT)
				msgproc_gameUI->getController(0)->addReadKey(0, KEYCODE_RIGHT_ARROW);
			else if(key == VK_UP)
				msgproc_gameUI->getController(0)->addReadKey(0, KEYCODE_UP_ARROW);
			else if(key == VK_DOWN)
				msgproc_gameUI->getController(0)->addReadKey(0, KEYCODE_DOWN_ARROW);
			else if(key == VK_DELETE)
				msgproc_gameUI->getController(0)->addReadKey(0, KEYCODE_DELETE);
		}
	}
	else if(msg == WM_CHAR)
	{
		char c = (TCHAR) wParam;
		if (msgproc_gameUI != NULL
			&& msgproc_gameUI->getController(0) != NULL)
		{
			msgproc_gameUI->getController(0)->addReadKey(c, 0);
		}
		//std::string str;
		//str += c;
		//str += "\r\n";
		//OutputDebugString(str.c_str());
	}

	return 0;
}

namespace {
	class StormLogger: public IStorm3D_Logger
	{
		Logger &logger;

	public:
		StormLogger(Logger &logger_)
		: logger(logger_)

		{
		}
		
		void debug(const char *msg)
		{
			logger.debug(msg);
		}

		void info(const char *msg)
		{
			logger.info(msg);
		}

		void warning(const char *msg)
		{
			logger.warning(msg);
		}

		void error(const char *msg)
		{
			logger.error(msg);
		}
	};

} // unnamed

/* --------------------------------------------------------- */

void parse_commandline(const char *cmdline, bool *windowed, bool *compile)
{
	if (cmdline != NULL)
	{
		// TODO: proper command line parsing 
		// (parse -option=value pairs?)
		int cmdlineLen = strlen(cmdline);
		char *parseBuf = new char[cmdlineLen + 1];
		strcpy(parseBuf, cmdline);
		int i;

		for (i = 0; i < cmdlineLen; i++)
		{
			if (parseBuf[i] == '=')
				parseBuf[i] = '\0';
			if (parseBuf[i] == ' ')
				parseBuf[i] = '\0';
		}

		for (i = 0; i < cmdlineLen; i++)
		{
			if (parseBuf[i] == '-')
			{
				i++;
				if (strcmp(&parseBuf[i], "windowed") == 0)
				{
					Logger::getInstance()->info("Windowed mode command line parameter given.");
					*windowed = true;
				}
				if (strcmp(&parseBuf[i], "compileonly") == 0)
				{
					Logger::getInstance()->info("Compileonly command line parameter given.");
					*windowed = true;
					*compile = true;
				}
				GameOption *opt = GameOptionManager::getInstance()->getOptionByName(&parseBuf[i]);
				if (opt != NULL)
				{
					int j = i + strlen(&parseBuf[i]);
					j += 1;
					if (j > cmdlineLen)
						j = cmdlineLen;

					Logger::getInstance()->debug("Option value given at command line.");
					Logger::getInstance()->debug(&parseBuf[i]);
					Logger::getInstance()->debug(&parseBuf[j]);

					if (opt->getVariableType() == IScriptVariable::VARTYPE_STRING)
					{
						opt->setStringValue(&parseBuf[j]);
					}
					else if (opt->getVariableType() == IScriptVariable::VARTYPE_INT)
					{
						opt->setIntValue(str2int(&parseBuf[j]));
					}
					else if (opt->getVariableType() == IScriptVariable::VARTYPE_FLOAT)
					{
						opt->setFloatValue((float)atof(&parseBuf[j]));
					}
					else if (opt->getVariableType() == IScriptVariable::VARTYPE_BOOLEAN)
					{
						if (parseBuf[j] == '\0'
							|| str2int(&parseBuf[j]) != 0)
							opt->setBooleanValue(true);
						else
							opt->setBooleanValue(false);
					}
				}
			}
		}

		delete [] parseBuf;
	}
}


void error_whine()
{
	bool foundErrors = false;
	FILE *fo = NULL;
#ifdef LEGACY_FILES
	FILE *f = fopen("log.txt", "rb");
#else
	FILE *f = fopen("logs/log.txt", "rb");
#endif
	if (f != NULL)
	{
		fseek(f, 0, SEEK_END);
		int flen = ftell(f);
		fseek(f, 0, SEEK_SET);

		char *buf = new char[flen + 1];

		fread(buf, flen, 1, f);
		buf[flen] = '\0';

		fclose(f);

		bool stillInError = false;

		for (int i = 0; i < flen; i++)
		{
			if (buf[i] == '\r')
			{
				buf[i] = ' ';
			}
			if (buf[i] == '\n' || buf[i] == '\0')
			{
				buf[i] = '\0';
				int skipErr = 0;
				if (strncmp(&buf[i + 1], "INFO: ", 6) == 0
					|| strncmp(&buf[i + 1], "DEBUG: ", 7) == 0)
				{
					stillInError = false;
				}
				if (strncmp(&buf[i + 1], "ERROR: ", 7) == 0
					|| strncmp(&buf[i + 1], "WARNING: ", 9) == 0
					|| stillInError)
				{
					if (strncmp(&buf[i + 1], "ERROR: ", 7) == 0) 
						skipErr = 7;
					if (strncmp(&buf[i + 1], "WARNING: ", 9) == 0) 
						skipErr = 9;
					stillInError = true;

					bool hadFileOrLine = false;
					foundErrors = true;
					if (fo == NULL)
					{
#ifdef LEGACY_FILES
						fo = fopen("log_errors.txt", "wb");
#else
						fo = fopen("logs/log_errors.txt", "wb");
#endif
						fprintf(fo, "\r\n*** Errors/warnings found - See \"log.txt\" for details. ***\r\n\r\n");
					}
					for (int j = i+1; j < flen+1; j++)
					{
						if (strncmp(&buf[j], "(file ", 6) == 0)
						{
							//buf[j - 1] = '\0';
							buf[j] = '\0';
							for (int k = j; k < flen; k++)
							{
								if (buf[k] == ',' || buf[k] == ')')
								{
									buf[k] = '\0';
									char *filename = &buf[j + 6];
									char *dhpsext = strstr(filename, ".dhps");
									if (dhpsext != NULL)
									{
										dhpsext[3] = 's';
										dhpsext[4] = '\0';
										if (fo != NULL)
											fprintf(fo, "%s:", filename);
										dhpsext[3] = 'p';
										dhpsext[4] = 's';
									} else {
										if (fo != NULL)
											fprintf(fo, "%s:", filename);
									}
									j = k;
									hadFileOrLine = true;
									break;
								}
							}
						}
						if (strncmp(&buf[j], "line ", 5) == 0)
						{
							for (int k = j; k < flen; k++)
							{
								if (buf[k] == ',' || buf[k] == ')')
								{
									buf[k] = '\0';
									if (str2int(&buf[j + 5]) == 0)
									{
										if (fo != NULL)
											fprintf(fo, "1: ", &buf[j + 5]);
									} else {
										if (fo != NULL)
											fprintf(fo, "%s: ", &buf[j + 5]);
									}
									j = k;
									hadFileOrLine = true;
									break;
								}
							}
						}
						if (buf[j] == '\r')
						{
							buf[j] = ' ';
						}
						if (buf[j] == '\n' || j == flen)
						{
							buf[j] = '\0';
							//if (hadFileOrLine)
							//{
							//  if (fo != NULL)
							//	  fprintf(fo, "%s\n", &buf[i+1+skipErr]);
							//} else {
							//}
							if (fo != NULL)
								fprintf(fo, "%s\r\n", &buf[i+1+skipErr]);
							if (i < j-1)
								i = j-1;
							break;
						}
					}
				}
			}
		}
	}

	if (fo != NULL)
	{
		fclose(fo);
	}

	if (foundErrors)
	{
		system("start devhtml\\htmllogview.bat");
	}
}


/* --------------------------------------------------------- */

int WINAPI WinMain(HINSTANCE hInstance, 
	HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	if(FindWindow(get_application_classname_string(), get_application_name_string()))
	{
		Sleep(3000);

		if(FindWindow(get_application_classname_string(), get_application_name_string()))
			return 0;
	}
	
	{
		using namespace frozenbyte::filesystem;
		boost::shared_ptr<IFilePackage> standardPackage(new StandardPackage());
		
		boost::shared_ptr<IFilePackage> zipPackage1(new ZipPackage("data1.fbz"));
		boost::shared_ptr<IFilePackage> zipPackage2(new ZipPackage("data2.fbz"));
		boost::shared_ptr<IFilePackage> zipPackage3(new ZipPackage("data3.fbz"));
		boost::shared_ptr<IFilePackage> zipPackage4(new ZipPackage("data4.fbz"));

		FilePackageManager &manager = FilePackageManager::getInstance();
		manager.addPackage(standardPackage, 999);

		manager.addPackage(zipPackage1, 1);
		manager.addPackage(zipPackage2, 2);
		manager.addPackage(zipPackage3, 3);
		manager.addPackage(zipPackage4, 4);
	}

	// initialize...

	frozenbyte::util::ModSelector modSelector;
	modSelector.changeDir();

#ifdef CHANGE_TO_PARENT_DIR
	// for profiling
//	int chdirfail = _chdir("..\\..\\..\\Snapshot");
//	if (chdirfail != 0) abort();
	//char curdir[256 + 1];
	//char *foo = _getcwd(curdir, 256);
	//if (foo == NULL) abort();
	//printf(curdir);
#endif

	bool checksumfailure = false;
	bool version_branch_failure = false;

	bool show_fps = false;
	bool show_polys = false;
	bool show_terrain_mem_info = false;
	int loglevel = 1;
	int showloglevel = 1;

	Timer::init();

	/*
	{
#ifdef LEGACY_FILES
		FILE *vbf = fopen("Data/Version/version_branch.txt", "rb");
#else
		FILE *vbf = fopen("data/version/version_branch.txt", "rb");
#endif
		if (vbf != NULL)
		{
			fseek(vbf, 0, SEEK_END);
			int vbf_size = ftell(vbf);
			fseek(vbf, 0, SEEK_SET);

			char *vbf_buf = new char[vbf_size + 1];
			int vbf_got = fread(vbf_buf, vbf_size, 1, vbf);
			if (vbf_got == 1)
			{
				vbf_buf[vbf_size] = '\0';
				if (strncmp(vbf_buf, version_branch_name, strlen(version_branch_name)) != 0)
				{
					version_branch_failure = true;					
				}
			} else {
				version_branch_failure = true;
			}
			delete [] vbf_buf;
			fclose(vbf);
		} else {
			version_branch_failure = true;
		}
	}

	{
		char fname_buf[256];
#ifdef LEGACY_FILES
		strcpy(fname_buf, "Data/Version/");
		strcat(fname_buf, version_branch_name);
		strcat(fname_buf, ".txt");
#else
		strcpy(fname_buf, "data/version/");
		strcat(fname_buf, version_branch_name);
		strcat(fname_buf, ".txt");
#endif
		FILE *vbf = fopen(fname_buf, "rb");
		if (vbf != NULL)
		{
			fclose(vbf);
		} else {
			version_branch_failure = true;
		}
	}
	*/

	/*
	int chksum1 = 3019411528;
	int chksize1 = 22357374;
	chksum1 ^= 1300166741;
	chksize1 ^= 21000334;
	if (!util::Checksummer::doesChecksumAndSizeMatchFile(chksum1, chksize1, "Data/GUI/Windows/command.dds"))
	{
		checksumfailure = true;
	}
	*/

	editor::Parser main_config;
#ifdef LEGACY_FILES
	filesystem::FilePackageManager::getInstance().getFile("Config/main.txt") >> main_config;
#else
	filesystem::FilePackageManager::getInstance().getFile("config/startup.txt") >> main_config;
#endif

	GameOptionManager::getInstance()->load();

	/*
	if (checksumfailure)
	{
		Logger::getInstance()->error("Checksum mismatch.");
		MessageBox(0,"Checksum mismatch or required data missing.\nMake sure you have all the application files properly installed.\n\nContact Frozenbyte for more info.","Error",MB_OK); 
		assert(!"Checksum mismatch");
		return 0;
	}
	*/

	if (version_branch_failure)
	{
		Logger::getInstance()->error("Version data incorrect.");
		MessageBox(0,"Required data missing.\nMake sure you have all the application files properly installed.\n\nSee game website for more info.","Error",MB_OK); 
		assert(!"Version data incorrect");
		abort();
		return 0;
	}


	bool windowedMode = false;
	bool compileOnly = false;

	char *cmdline = lpCmdLine;
	parse_commandline(cmdline, &windowedMode, &compileOnly);

	if (SimpleOptions::getBool(DH_OPT_B_WINDOWED))
		windowedMode = true;

	//if (!windowedMode)
	//{
		// new behaviour: disable script preprocess if fullscreen
	//	game::SimpleOptions::setBool(DH_OPT_B_AUTO_SCRIPT_PREPROCESS, false);
	//}

	StormLogger logger(*Logger::getInstance());
	IStorm3D *s3d = IStorm3D::Create_Storm3D_Interface(true, &filesystem::FilePackageManager::getInstance(), &logger);

//disposable_s3d = s3d;

	s3d->SetUserWindowMessageProc(&customMessageProc);

	s3d->SetApplicationName(get_application_classname_string(), get_application_name_string());

	// set lighting / shadow texture qualities now (must be set
	// before storm3d creation)
	if (SimpleOptions::getInt(DH_OPT_I_SHADOWS_LEVEL) >= 50)
	{
		s3d->SetFakeShadowQuality(SimpleOptions::getInt(DH_OPT_I_FAKESHADOWS_TEXTURE_QUALITY));
	} else {
		s3d->SetFakeShadowQuality(-1);
	}

	s3d->SetShadowQuality(SimpleOptions::getInt(DH_OPT_I_SHADOWS_TEXTURE_QUALITY));
	s3d->SetLightingQuality(SimpleOptions::getInt(DH_OPT_I_LIGHTING_TEXTURE_QUALITY));
	s3d->EnableGlow(SimpleOptions::getBool(DH_OPT_B_RENDER_GLOW));

	if(SimpleOptions::getBool(DH_OPT_B_RENDER_REFLECTION))
		s3d->SetReflectionQuality(50);
	else
		s3d->SetReflectionQuality(0);

	if(!SimpleOptions::getBool(DH_OPT_B_HIGH_QUALITY_VIDEO))
	{
		s3d->DownscaleVideos(true);
		//s3d->HigherColorRangeVideos(false);
	}
	/*
	// lipsync targets
	{
		std::string icon_xsize_conf("");
		std::string icon_ysize_conf("");

		icon_xsize_conf = std::string("gui_message_face1_size_x");
		icon_ysize_conf = std::string("gui_message_face1_size_y");

		int iconXSize = getLocaleGuiInt(icon_xsize_conf.c_str(), 0);
		int iconYSize = getLocaleGuiInt(icon_ysize_conf.c_str(), 0);

		float relativeSizeX = (float)iconXSize / 1024.0f;
		float relativeSizeY = (float)iconYSize / 768.0f;

		// NOTE: antialiased, thus size * 2
		s3d->addAdditionalRenderTargets(VC2(relativeSizeX * 2.0f, relativeSizeY * 2.0f), 2);
	}
	*/

	::util::ProceduralProperties proceduralProperties;
	::util::applyStorm(*s3d, proceduralProperties);

	// some cursor configs
	bool no_mouse = false;
	bool no_keyboard = false;
	bool no_joystick = false;
	if (!SimpleOptions::getBool(DH_OPT_B_MOUSE_ENABLED))
	{
		no_mouse = true;
	}
	if (!SimpleOptions::getBool(DH_OPT_B_KEYBOARD_ENABLED))
	{
		no_keyboard = true;
	}
	if (!SimpleOptions::getBool(DH_OPT_B_JOYSTICK_ENABLED))
	{
		no_joystick = true;
	}
	bool force_given_boundary = false;
	if (SimpleOptions::getBool(DH_OPT_B_MOUSE_FORCE_GIVEN_BOUNDARY))
	{
		force_given_boundary = true;
	}
	float mouse_sensitivity = 1.0f;
	mouse_sensitivity = SimpleOptions::getFloat(DH_OPT_F_MOUSE_SENSITIVITY);
	if (mouse_sensitivity < 0.01f) 
		mouse_sensitivity = 0.01f;

	// camera stuff
	bool disable_camera_timing = false;
	if (SimpleOptions::getBool(DH_OPT_B_CAMERA_DISABLE_TIMING))
	{
		disable_camera_timing = true;
	}
	bool no_delta_time_limit = false;
	if (SimpleOptions::getBool(DH_OPT_B_CAMERA_NO_DELTA_TIME_LIMIT))
	{
		no_delta_time_limit = true;
	}
	float camera_time_factor = 1.0f;
	if (SimpleOptions::getFloat(DH_OPT_F_CAMERA_TIME_FACTOR) > 0.0f)
	{
		camera_time_factor = SimpleOptions::getFloat(DH_OPT_F_CAMERA_TIME_FACTOR);
	}
	int camera_mode = 1;
	if(SimpleOptions::getInt(DH_OPT_I_CAMERA_MODE) > 0)
		camera_mode = SimpleOptions::getInt(DH_OPT_I_CAMERA_MODE);
	if (camera_mode < 1) camera_mode = 1;
	if (camera_mode > 4) camera_mode = 4;

	scr_width = SimpleOptions::getInt(DH_OPT_I_SCREEN_WIDTH);
	scr_height = SimpleOptions::getInt(DH_OPT_I_SCREEN_HEIGHT);

	bool vsync = SimpleOptions::getBool(DH_OPT_B_RENDER_USE_VSYNC);
	s3d->UseVSync(vsync);
	bool stormInitSucces = true;

	s3d->SetAntiAliasing(SimpleOptions::getInt(DH_OPT_I_ANTIALIAS_SAMPLES));

	// windowed mode?
	if (windowedMode)
	{
		// screen resolution
		if (compileOnly)
		{
			s3d->SetWindowedMode(32, 32);
		} else {
			if(!s3d->SetWindowedMode(scr_width, scr_height, SimpleOptions::getBool(DH_OPT_B_WINDOW_TITLEBAR)))
				stormInitSucces = false;
			if(SimpleOptions::getBool(DH_OPT_B_MAXIMIZE_WINDOW))
			{
				ShowWindow(s3d->GetRenderWindow(), SW_MAXIMIZE);
			}
		}
	} else {
		int bpp = SimpleOptions::getInt(DH_OPT_I_SCREEN_BPP);

		if(!s3d->SetFullScreenMode(scr_width, scr_height, bpp))
			stormInitSucces = false;

		Storm3D_SurfaceInfo surfinfo = s3d->GetCurrentDisplayMode();
		scr_width = surfinfo.width;
		scr_height = surfinfo.height;
	}

	if(!stormInitSucces)
	{
		std::string error = s3d->GetErrorString();
		delete s3d;
		s3d = 0;

		MSG msg = { 0 };
		while(PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		MessageBox(0, error.c_str(), "Renderer initialization failure.", MB_OK);
		Logger::getInstance()->error("Failed to initialize renderer");
		Logger::getInstance()->error(error.c_str());
		return 0;
	}

	OptionApplier::applyGammaOptions(s3d);
	::util::applyRenderer(*s3d, proceduralProperties);

	// keyb3 controller devices
	int ctrlinit = 0;
	if (!no_mouse) ctrlinit |= KEYB3_CAPS_MOUSE;
	if (!no_keyboard) ctrlinit |= KEYB3_CAPS_KEYBOARD;
	if (!no_joystick) ctrlinit |= (KEYB3_CAPS_JOYSTICK | KEYB3_CAPS_JOYSTICK2 | KEYB3_CAPS_JOYSTICK3 | KEYB3_CAPS_JOYSTICK4 );
	if (ctrlinit == 0)
	{
		Logger::getInstance()->warning("No control devices enabled, forcing mouse enable.");
		ctrlinit = KEYB3_CAPS_MOUSE;
	}
	Keyb3_Init(s3d->GetRenderWindow(), ctrlinit);
	Keyb3_SetActive(1);
	
	if (force_given_boundary)
	{
		Keyb3_SetMouseBorders((int)(scr_width / mouse_sensitivity), 
			(int)(scr_height / mouse_sensitivity));
		Keyb3_SetMousePos((int)(scr_width / mouse_sensitivity) / 2, 
			(int)(scr_height / mouse_sensitivity) / 2);
	} else {
		Storm3D_SurfaceInfo screenInfo = s3d->GetScreenSize();
		Keyb3_SetMouseBorders((int)(screenInfo.width / mouse_sensitivity), 
			(int)(screenInfo.height / mouse_sensitivity));
		Keyb3_SetMousePos((int)(screenInfo.width / mouse_sensitivity) / 2, 
			(int)(screenInfo.height / mouse_sensitivity) / 2);
	}
	Keyb3_UpdateDevices();

	if (SimpleOptions::getBool(DH_OPT_B_SHOW_TERRAIN_MEMORY_INFO))
		show_terrain_mem_info = true;

	int tex_detail_level = SimpleOptions::getInt(DH_OPT_I_TEXTURE_DETAIL_LEVEL);
	if (tex_detail_level < 0) tex_detail_level = 0;
	if (tex_detail_level > 100) tex_detail_level = 100;
	// convert level to 0 (high) - 1 (medium) - 2 (low)
	s3d->SetTextureLODLevel(2 - (tex_detail_level / 50));

	if (SimpleOptions::getBool(DH_OPT_B_HIGH_QUALITY_LIGHTMAP))
		s3d->EnableHighQualityTextures(true);
	else
		s3d->EnableHighQualityTextures(false);

	// make a scene
	COL bgCol = COL(0.0f, 0.0f, 0.0f);
	COL ambLight = COL(1.0f, 1.0f, 1.0f);

	disposable_scene = s3d->CreateNewScene();
	disposable_scene->SetBackgroundColor(bgCol);
	//scene->SetAmbientLight(ambLight);

	// show loadscreen...
	//std::string loadscr_str = Parser::GetString(main_config.GetProperties(), "load_screen");
	std::string loadscr_str = main_config.getGlobals().getValue("load_screen");
	char *load_fname = const_cast<char *> (loadscr_str.c_str());
	if(strlen(load_fname) > 3)
	{
		IStorm3D_Texture *t = s3d->CreateNewTexture(load_fname, TEXLOADFLAGS_NOCOMPRESS|TEXLOADFLAGS_NOLOD);
		IStorm3D_Material *m = s3d->CreateNewMaterial("Load screen");
		m->SetBaseTexture(t);

		RECT rc = { 0 };
		GetClientRect(s3d->GetRenderWindow(), &rc);
		
		disposable_scene->Render2D_Picture(m, Vector2D((float)rc.left,(float)rc.top), Vector2D((float)rc.right-1,(float)rc.bottom-1));
		disposable_scene->RenderScene();

		delete m;
	}


	// camera visibility
	int visRange = SimpleOptions::getInt(DH_OPT_I_CAMERA_RANGE);
	disposable_scene->GetCamera()->SetVisibilityRange((float)visRange);

	if (SimpleOptions::getBool(DH_OPT_B_GAME_SIDEWAYS))
	{
		disposable_scene->GetCamera()->SetUpVec(VC3(0,0,1));
	} else {
		disposable_scene->GetCamera()->SetUpVec(VC3(0,1,0));
	}

	// create and initialize ogui
	Ogui *ogui = new Ogui();
	OguiStormDriver *ogdrv = new OguiStormDriver(s3d, disposable_scene);
	ogui->SetDriver(ogdrv);
	ogui->SetScale(OGUI_SCALE_MULTIPLIER * scr_width / 1024, 
		OGUI_SCALE_MULTIPLIER * scr_height / 768); 
	ogui->SetMouseSensitivity(mouse_sensitivity, mouse_sensitivity);
	ogui->Init();

	// set default font
#ifdef LEGACY_FILES
	ogui->LoadDefaultFont("Data/Fonts/default.ogf");
#else
	ogui->LoadDefaultFont("data/gui/font/common/default.ogf");
#endif

	// set default ui (ogui and storm) for visual objects (pictures and models)
	ui::createUIDefaults(ogui);
	ui::Visual2D::setVisualOgui(ogui);
	ui::VisualObjectModel::setVisualStorm(s3d, disposable_scene);

	Animator::init(s3d);

	LoadingMessage::setManagers(s3d, disposable_scene, ogui);

	// error handling (forward logger messages to error window)
	ui::ErrorWindow *errorWin = new ui::ErrorWindow(ogui);
	(Logger::getInstance())->setListener(errorWin);

	// apply logger options
	OptionApplier::applyLoggerOptions();

	log_version();

	// font for fps and polycount
	/*
	IStorm3D_Font *fpsFont = NULL;
	if (show_fps || show_polys)
	{
		IStorm3D_Texture *texf = s3d->CreateNewTexture("Data/Fonts/fpsfont.dds", TEXLOADFLAGS_NOCOMPRESS|TEXLOADFLAGS_NOLOD);
		fpsFont = s3d->CreateNewFont();
		fpsFont->AddTexture(texf);
		fpsFont->SetTextureRowsAndColums(8, 8);
		fpsFont->SetColor(COL(1,1,1));
		BYTE chrsize[42];
		for (int i = 0; i < 41; i++) chrsize[i] = 64;
		chrsize[41] = '\0';
		fpsFont->SetCharacters("1234567890(),:ABCDEFGHIJKLMNOPQRSTUVWXYZ ", chrsize);
	}
	*/

	// create cursors
	if (no_mouse && no_keyboard && no_joystick)
	{
		Logger::getInstance()->error("Mouse, keyboard and joystick disabled in config - forced keyboard.");
	}
	if (no_mouse)
	{
		if (!no_joystick)
		{
			ogui->SetCursorController(0, OGUI_CURSOR_CTRL_JOYSTICK1);
		} else {
			ogui->SetCursorController(0, OGUI_CURSOR_CTRL_KEYBOARD1);
		}
	} else {
		 ogui->SetCursorController(0, OGUI_CURSOR_CTRL_MOUSE);
	}

	// cursors images for controller 0,1,2,3
	loadDHCursors(ogui, 0); 
	loadDHCursors(ogui, 1); 
	loadDHCursors(ogui, 2); 
	loadDHCursors(ogui, 3); 

	ogui->SetCursorImageState(0, DH_CURSOR_ARROW);

	// sounds
	SoundLib *soundLib = NULL;
	SoundMixer *soundMixer = NULL;
	if(SimpleOptions::getBool(DH_OPT_B_SOUNDS_ENABLED))
	{
		soundLib = new SoundLib();

		bool s_use_hardware = SimpleOptions::getBool(DH_OPT_B_SOUND_USE_HARDWARE);
		bool s_use_eax = SimpleOptions::getBool(DH_OPT_B_SOUND_USE_EAX);
		int s_mixrate = SimpleOptions::getInt(DH_OPT_I_SOUND_MIXRATE);
		int s_softchan = SimpleOptions::getInt(DH_OPT_I_SOUND_SOFTWARE_CHANNELS);
		int s_req_hardchan = SimpleOptions::getInt(DH_OPT_I_SOUND_REQUIRED_HARDWARE_CHANNELS);
		int s_max_hardchan = SimpleOptions::getInt(DH_OPT_I_SOUND_MAX_HARDWARE_CHANNELS);

		// TODO: speaker_type

		SoundLib::SpeakerType speakerType = SoundLib::StereoSpeakers;
		const char *speakerString = SimpleOptions::getString(DH_OPT_S_SOUND_SPEAKER_TYPE);
		if(speakerString)
		{
			if(strcmp(speakerString, "headphones") == 0)
				speakerType = SoundLib::HeadPhones;
			else if(strcmp(speakerString, "stereo") == 0)
				speakerType = SoundLib::StereoSpeakers;
			else if(strcmp(speakerString, "mono") == 0)
				speakerType = SoundLib::MonoSpeakers;
			else if(strcmp(speakerString, "quad") == 0)
				speakerType = SoundLib::QuadSpeakers;
			else if(strcmp(speakerString, "surround") == 0)
				speakerType = SoundLib::SurroundSpeakers;
			else if(strcmp(speakerString, "dolby") == 0)
				speakerType = SoundLib::DolbyDigital;
		}

		soundLib->setProperties(s_mixrate, s_softchan);
		//soundLib->setSpeakers(speakerType);
		soundLib->setAcceleration(s_use_hardware, s_use_eax, s_req_hardchan, s_max_hardchan);

		if(soundLib->initialize(s3d->GetRenderWindow()))
		{
			Logger::getInstance()->debug("Sound system initialized succesfully");
			soundMixer = new SoundMixer(soundLib);
		}
		else
		{
			Logger::getInstance()->warning("Failed to sound system - sounds disabled");

			delete soundLib;
			soundLib = 0;
		}
	}

	if (soundMixer != NULL)
	{
		bool fxMute = false;
		bool musicMute = false;
		bool speechMute = false;
		if (!SimpleOptions::getBool(DH_OPT_B_SPEECH_ENABLED))
			speechMute = true;
		if (!SimpleOptions::getBool(DH_OPT_B_MUSIC_ENABLED))
			musicMute = true;
		if (!SimpleOptions::getBool(DH_OPT_B_FX_ENABLED))
			fxMute = true;
		int masterVolume = SimpleOptions::getInt(DH_OPT_I_MASTER_VOLUME);
		int fxVolume = SimpleOptions::getInt(DH_OPT_I_FX_VOLUME);
		int musicVolume = SimpleOptions::getInt(DH_OPT_I_MUSIC_VOLUME);
		int speechVolume = SimpleOptions::getInt(DH_OPT_I_SPEECH_VOLUME);

		soundMixer->setVolume(masterVolume, fxVolume, speechVolume, musicVolume);
		soundMixer->setMute(fxMute, speechMute, musicMute);
	}

	// create part types (create base types and read data files)
	createPartTypes();

	// player weaponry init
	PlayerWeaponry::initWeaponry();

	// create game and game UI
	Game *game = new Game();
	GameUI *gameUI = new GameUI(ogui, game, s3d, disposable_scene, soundMixer);
	gameUI->setOguiStormDriver(ogdrv);
	game->setUI(gameUI);
	gameUI->setErrorWindow(errorWin);

	msgproc_gameUI = gameUI;

	MusicPlaylist *musicPlaylist = gameUI->getMusicPlaylist(game->singlePlayerNumber);
	
	if (SimpleOptions::getBool(DH_OPT_B_MUSIC_SHUFFLE))
	{
		musicPlaylist->setSuffle(true);
	}

	// init unit actors for unit types
	createUnitActors(game);
	createUnitTypes();

	/*
	GameCamera::CAMERA_MODE cammod = GameCamera::CAMERA_MODE_ZOOM_CENTRIC;
	if (camera_mode == 2) cammod = GameCamera::CAMERA_MODE_CAMERA_CENTRIC;
	if (camera_mode == 3) cammod = GameCamera::CAMERA_MODE_TARGET_CENTRIC;
	if (camera_mode == 4) cammod = GameCamera::CAMERA_MODE_FLYING;
	gameUI->selectCamera(GAMEUI_CAMERA_TACTICAL);
	gameUI->getGameCamera()->setMode(cammod);
	gameUI->selectCamera(GAMEUI_CAMERA_NORMAL);
	gameUI->getGameCamera()->setMode(GameCamera::CAMERA_MODE_TARGET_CENTRIC);
	*/
	gameUI->selectCamera(GAMEUI_CAMERA_NORMAL);
	gameUI->getGameCamera()->setMode(GameCamera::CAMERA_MODE_TARGET_CENTRIC);

	// set the initial camera time factor... may be changed by scripts
	// (however, should always be restored to original)
	gameUI->setCameraTimeFactor(camera_time_factor);

	// start a new single player game
	game->newGame(false);

	if (!compileOnly)
	{
		float masterVolume = SimpleOptions::getInt(DH_OPT_I_MASTER_VOLUME) / 100.f;
		float fxVolume = SimpleOptions::getInt(DH_OPT_I_FX_VOLUME)  / 100.f;
		float volume = masterVolume * fxVolume;
		if(!soundMixer || !SimpleOptions::getBool(DH_OPT_B_FX_ENABLED))
			volume = 0;

		//ui::GameVideoPlayer::playVideo(disposable_scene, "Data\\Videos\\frozenbyte_logo.mpg", volume);
		//ui::GameVideoPlayer::playVideo(disposable_scene, "Data\\Videos\\frozenbyte_logo.wmv", volume);
		//ui::GameVideoPlayer::playVideo(disposable_scene, "Data\\Videos\\test.wmv", volume);
		//ui::GameVideoPlayer::playVideo(disposable_scene, "Data\\Videos\\introduction_final.wmv", volume);
		//ui::GameVideoPlayer::playVideo(disposable_scene, "Data\\Videos\\chicken.wmv", volume);
		//ui::GameVideoPlayer::playVideo(disposable_scene, "Data\\Videos\\ruby.avi", volume);

		IStorm3D_StreamBuilder *builder = 0;
		if(soundMixer)
			builder = soundMixer->getStreamBuilder();

		ui::GameVideoPlayer::playVideo(disposable_scene, "Data\\Videos\\logo.wmv", builder);
		ui::GameVideoPlayer::playVideo(disposable_scene, "Data\\Videos\\logo_pub.wmv", builder);
	}
	
	gameUI->startCommandWindow( 0 );
	// do the loop...

	Timer::update();
	DWORD startTime = Timer::getTime(); 
	DWORD curTime = startTime;
	DWORD movementTime = startTime;
	DWORD frameCountTime = startTime;
	DWORD gameCountTime = startTime;
	DWORD lastOguiUpdateTime = startTime;
	bool quitRequested = false;

	int frames = 0;
	int polys = 0;
	int fps = 0;
	int polysps = 0;
	int precount = 0;

#ifdef SCRIPT_DEBUG
	ScriptDebugger::init();
#endif

	Keyb3_UpdateDevices();

	

	while (!quitRequested)
	{
		Sleep(0);

		ogui->UpdateCursorPositions();

		// quick hack, if in single player game, don't allow very big leaps.
		// if in multiplayer, naturally must allow them.
		if (!game->isMultiplayer())
		{
			// max 0.5 sec leap max.
			if (curTime > gameCountTime + 500)
			{
				int timediff = (curTime - 500 - gameCountTime);
				// round to game ticks
				timediff = (timediff / GAME_TICK_MSEC) * GAME_TICK_MSEC;
				gameCountTime += timediff;
			}
		}

		while (curTime > gameCountTime)
		{
			gameCountTime += GAME_TICK_MSEC;
			//gameCountTime += 17; // about 60Hz (actually about 59, gotta fix...)
			game->run();

			disposable_ticks_since_last_clear++;

#ifdef SCRIPT_DEBUG
			ScriptDebugger::run(game->gameScripting);
#endif
		}

		// read input
		
		Keyb3_UpdateDevices();

		// can't use curTime here, because game may have just
		// loaded a map, or something else alike -> curTime 
		// would be badly behind... thus Timer::update and getTime.
		Timer::update();

	// psd: time hack
		int maxfps = SimpleOptions::getInt(DH_OPT_I_RENDER_MAX_FPS);
		if (maxfps > 0)
		{
			int maxfps_in_msec = 1000 / maxfps;
			while(int(Timer::getTime() - curTime) < maxfps_in_msec)
			{
				Sleep(0);
				Timer::update();
			}
		}

	//while(Timer::getTime() - curTime < 33)
	//	Timer::update();

		gameUI->runUI(Timer::getTime() - startTime);

		//int lastFrameTime = curTime;

		// current time
		Timer::update();
		curTime = Timer::getTime(); //- startTime;

		//disposable_frametime_msec = curTime - lastFrameTime;
		disposable_frames_since_last_clear++;

		// TODO: should be handled in the GameUI, using the gameController
		if (gameUI->isQuitRequested())
		{
			// need to end combat before quit, otherwise cannot delete gameUI,
			// because the game is still alive - thus projectiles may still
			// refer to visualeffects (which gameui would delete ;)
			if (game->inCombat)
				game->endCombat(); 
			quitRequested = true;
			// break; // why break here?
		}


		//int mouseDeltaX, mouseDeltaY;
		//Keyb3_ReadMouse(NULL, NULL, &mouseDeltaX, &mouseDeltaY);


		// movement every 20 ms (50 Hz)
		//while (curTime - movementTime > 20)
		//{

			if (curTime - movementTime > 0)
			{
				// VEEERY jerky... 
				//doMovement(game->gameMap, curTime - movementTime);
				// attempt to fix that...
				float delta;
				if (disable_camera_timing)
				{
					delta = 20.0f;
				} else {
					delta = 100.0f;
					if (fps > 0) delta = 1000.0f/fps;
					if (!no_delta_time_limit)
					{
						if (delta < 1.0f) delta = 1.0f;
						if (delta > 100.0f) delta = 100.0f;
					}
				}
				camera_time_factor = gameUI->getCameraTimeFactor();
				delta = (delta * camera_time_factor);
				if (game->inCombat)
				{
					gameUI->getGameCamera()->doMovement(delta);
				}

				movementTime = curTime;
			}
		//}

		// frame/poly counting
		frames++;
		{
			if (curTime - frameCountTime >= 100) 
			{
			 float seconds = (curTime - frameCountTime) / 1000.0f;
			 fps = (int)(frames / seconds);
			 polysps = (int)(polys / seconds);
			 frameCountTime = curTime;
			 frames = 0;
			 polys = 0;
			}
		}

		// added by Pete
		gameUI->updateCameraDependedElements();

		// error window to top
		if (errorWin->isVisible())
		{
			errorWin->raise();
		}

		// run the gui
		int oguiTimeDelta = curTime - lastOguiUpdateTime;
		// max 200ms warps
		// NOTE: this may cause inconsistency between game and ogui timing
		// (fadeout, etc. effects may not match)
		if (oguiTimeDelta > 200)
			oguiTimeDelta = 200;

		ogui->Run(oguiTimeDelta);

		lastOguiUpdateTime = curTime;

		// render stats

		if (SimpleOptions::getBool(DH_OPT_B_SHOW_FPS))
			show_fps = true;
		else
			show_fps = false;
		if (SimpleOptions::getBool(DH_OPT_B_SHOW_POLYS))
			show_polys = true;
		else
			show_polys = false;

		/*
		if (show_polys || show_fps)
		{
			char infobuf[150];
			int polyspf = 0;
			if (fps > 0) polyspf = polysps / fps;
			if (show_polys)
			{
				if (show_fps)
					sprintf(infobuf, "FPS:%d   POLYS PER SEC:%d   POLYS PER FRAME:%d", fps, polysps, polyspf);
				else
					sprintf(infobuf, "POLYS PER SEC:%d   POLYS PER FRAME:%d", polysps, polyspf);
			} else {
					sprintf(infobuf, "FPS:%d", fps);
			}
			gameUI->gameMessage(infobuf, NULL, 1, 1000, GameUI::MESSAGE_TYPE_NORMAL);
		}
		*/

		if (show_polys || show_fps)
		{
			// WARNING: unsafe cast!
			if (ui::defaultIngameFont != NULL)
			{
				IStorm3D_Font *fpsFont = ((OguiStormFont *)ui::defaultIngameFont)->fnt;

				float polyoffset = 0;
				float terroffset = 0;
				char polytextbuf[40];
				char polyframetextbuf[40];
				char fpstextbuf[40];
				if (show_fps)
				{
					polyoffset = 16;
					terroffset = 16;
					sprintf(fpstextbuf, "FPS:%d", fps);
					disposable_scene->Render2D_Text(fpsFont, VC2(0,0), VC2(16, 16), fpstextbuf);
				}
				if (show_polys)
				{
					terroffset += 32;
					sprintf(polytextbuf, "POLYS PER S:%d", polysps);
					int polyspf = 0;
					if (fps > 0) polyspf = polysps / fps;
					sprintf(polyframetextbuf, "POLYS PER F:%d", polyspf);
					disposable_scene->Render2D_Text(fpsFont, VC2(0,polyoffset), VC2(16, 16), polytextbuf);
					disposable_scene->Render2D_Text(fpsFont, VC2(0,polyoffset+16), VC2(16, 16), polyframetextbuf);
				}
				/*
				if (show_terrain_mem_info)
				{
					char terrtextbuf[80];
					if (game->gameUI->getTerrain() != NULL)
					{
						int t1 = 0; //game->gameUI->getTerrain()->GetTerrain()->getVisibleBlockCount();
						//if (game->gameUI->getTerrain()->GetTerrain()->getPrecachedBlockCount() > 0)
						//	precount++;
						//else
							precount=0;
						int t3 = 0; //game->gameUI->getTerrain()->GetTerrain()->getTotalGeneratedBlockCount();
						sprintf(terrtextbuf, "TERRAIN:VIS %d, PRE %d, TOT %d", t1, precount, t3);
						disposable_scene->Render2D_Text(fpsFont, VC2(0,terroffset), VC2(16, 16), terrtextbuf);
					}
				}
				*/
			}
		}

		// WARNING: assmuing that no thread is accessing storm3d's data structures at this moment
		// (should be true, as physics thread should not do that in any other way, that by calling
		// the logger, which itself is thread safe)
		Logger::getInstance()->syncListener();

		// Render scene
		polys += renderfunc(disposable_scene);

		SelectionBox *sbox = gameUI->getSelectionBox();
		if (sbox != NULL)
			sbox->render();

		// psd hack, get terrain colors!
		//if(Keyb3_IsKeyPressed(KEYCODE_F12))
		//	game->gameUI->getTerrain()->GetTerrain()->SaveColorMap("terraincolors.raw");

		// jpk hack, get memory leaks! ;)
#ifdef _DEBUG
		/*
		if(Keyb3_IsKeyPressed(KEYCODE_F12))
		{
			//fopen("foobar.txt", "rb");
			//fopen("foobar.txt", "wb");
			frozenbyte::debug::dumpLeakSnapshot();
			frozenbyte::debug::markLeakSnapshot();
		}
		*/
#endif

		DebugDataView::getInstance(game)->run();

		if (next_interface_generation_request)
		{
			DebugDataView::getInstance(game)->cleanup();

			// if quit requested, no point in creating a next generation...
			if (!quitRequested)
			{
				next_interface_generation_request = false;
				interface_generation_counter++;
				if (interface_generation_counter >= SimpleOptions::getInt(DH_OPT_I_CLEANUP_SKIP_RATE))
				{
					interface_generation_counter = 0;
					Logger::getInstance()->debug("About to create next interface generation.");
					gameUI->nextInterfaceGeneration();
				} else {
					Logger::getInstance()->debug("Skipping next interface generation due to cleanup skip rate.");
				}
				if (gameUI->getEffects() != NULL)
				{
					gameUI->getEffects()->startFadeIn(500);
				}
			}
		}

		if (!next_interface_generation_request)
		{
			game->advanceMissionStartState();
		}

		if (apply_options_request)
		{
			Logger::getInstance()->debug("About to apply game options...");
			apply_options_request = false;
			game::GameOptionManager *oman = game::GameOptionManager::getInstance();
			game::OptionApplier::applyOptions(game, oman, ogui);
			Logger::getInstance()->debug("Game options applied.");
		}

		if (compileOnly)
		{
			quitRequested = true;
//			exit(0);
		}

		// wait here if minimized...
		while (quitRequested == false)
		{
			// psd -- handle windows messages
			MSG msg = { 0 };
			while(PeekMessage(&msg, s3d->GetRenderWindow(), 0, 0, PM_NOREMOVE))
			{
				if(GetMessage(&msg, s3d->GetRenderWindow(), 0, 0) <= 0)
				{
					gameUI->setQuitRequested();
					//quitRequested = true;
				}
				else
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}
			if (lostFocusPause)
			{
				Sleep(500);
			} else {
				break;
			}
		}
	}

// TEMP: end splash screen.
/*
OguiWindow *win = ogui->CreateSimpleWindow(0,0,1024,768,"Data/Pictures/Cinematic/germany_demo_splash.tga");

win->Raise();

// HACK!
ogui->SetCursorImageState(0, DH_CURSOR_INVISIBLE);
ogui->Run(1);
disposable_scene->RenderScene();

bool quitSplashRequested = false;

Timer::update();
int splashStartTime = Timer::getTime();
while (Timer::getTime() - splashStartTime < 30000)
{
  Timer::update();
	Sleep(20);

	Keyb3_UpdateDevices();
	if (Keyb3_IsKeyPressed(KEYCODE_ESC)
		|| (Keyb3_IsKeyPressed(KEYCODE_MOUSE_BUTTON1) && Timer::getTime() - splashStartTime > 1000)
		|| Keyb3_IsKeyPressed(KEYCODE_SPACE)
		|| Keyb3_IsKeyPressed(KEYCODE_ENTER))
	{
		quitSplashRequested = true;
	}

	if (Timer::getTime() - splashStartTime > 10000
		&& quitSplashRequested)
	{
		break;
	}

}

delete win;
*/

	// clean up

	bool errorWhine = SimpleOptions::getBool(DH_OPT_B_CREATE_ERROR_LOG);

	msgproc_gameUI = NULL;

	Logger::getInstance()->debug("Starting exit cleanup...");

	(Logger::getInstance())->setListener(NULL);

	DebugDataView::cleanInstance();

	Animator::uninit();

	unloadDHCursors(ogui, 0); 

	delete gameUI;
	game->gameUI = 0;
	delete game;

	::util::ScriptManager::cleanInstance();

	GameRandom::uninit();

	deleteUIDefaults();

	deleteUnitTypes();
	deleteUnitActors();

	PlayerWeaponry::uninitWeaponry();

	deletePartTypes();

	if (soundMixer != NULL)
		delete soundMixer;
	if (soundLib != NULL)
		delete soundLib;

	delete errorWin;

	ogui->Uninit();
	delete ogui;
	delete ogdrv;

	Keyb3_Free();
	
	delete s3d;

	GameOptionManager::cleanInstance();
	GameConfigs::cleanInstance();

	SystemRandom::cleanInstance();

	uninitLinkedListNodePool();

	Timer::uninit();

	Logger::getInstance()->debug("Cleanup done, exiting.");

	Logger::cleanInstance();

	//assert(ui::visual_object_allocations == 0);
	//assert(ui::visual_object_model_allocations == 0);

	if (errorWhine)
	{
		error_whine();
	}

	return 0;
}
 
