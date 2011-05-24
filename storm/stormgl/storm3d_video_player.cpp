// Copyright 2002-2004 Frozenbyte Ltd.

#ifdef _MSC_VER
#pragma warning(disable:4103)
#endif

#include <string>
#include <vector>

#include "istorm3d_streambuffer.h"
#include "storm3d_video_player.h"
#include "storm3d_videostreamer.h"
#include "storm3d.h"
#include "storm3d_scene.h"
#include "keyb3.h"

struct Storm3D_VideoPlayerData
{
	Storm3D &storm;
	Storm3D_Scene &scene;
	boost::shared_ptr<Storm3D_VideoStreamer> streamer;
	IStorm3D_StreamBuilder *streamBuilder;  // not owned

	Storm3D_VideoPlayerData(Storm3D &storm_, Storm3D_Scene &scene_, const char *filename, class IStorm3D_StreamBuilder *streamBuilder_)
	:	storm(storm_),
		scene(scene_)
	,	streamBuilder(streamBuilder_)
	{
		streamer.reset(static_cast<Storm3D_VideoStreamer *> (storm.CreateVideoStreamer(filename, streamBuilder, false)));
	}
};

Storm3D_VideoPlayer::Storm3D_VideoPlayer(Storm3D &storm, Storm3D_Scene &scene, const char *fileName, IStorm3D_StreamBuilder *streamBuilder)
{
	boost::scoped_ptr<Storm3D_VideoPlayerData> tempData(new Storm3D_VideoPlayerData(storm, scene, fileName, streamBuilder));
	data.swap(tempData);
}

Storm3D_VideoPlayer::~Storm3D_VideoPlayer()
{
}

void Storm3D_VideoPlayer::play()
{
	if(!data->streamer)
		return;

	while(!data->streamer->hasEnded())
	{
#ifdef WIN32
		Sleep(0);
#else
		usleep(0);
#endif
		data->streamer->render(&data->scene);
		data->scene.RenderScene(true);

		if (data->streamBuilder) data->streamBuilder->update();

// Requiring keyb3 breaks stuff on Windows
// Videos won't play in any case so this isn't really necessary anyway
#ifndef WIN32
		Keyb3_UpdateDevices();

		if (Keyb3_IsKeyPressed(KEYCODE_ESC) || Keyb3_IsKeyPressed(KEYCODE_SPACE)
			|| Keyb3_IsKeyPressed(KEYCODE_ENTER) || Keyb3_IsKeyPressed(KEYCODE_MOUSE_BUTTON1)
			|| Keyb3_IsKeyPressed(KEYCODE_MOUSE_BUTTON2)
			|| Keyb3_IsKeyPressed(KEYCODE_MOUSE_BUTTON3)
			|| Keyb3_IsKeyPressed(KEYCODE_JOY_BUTTON1)
			|| Keyb3_IsKeyPressed(KEYCODE_JOY_BUTTON2)
			|| Keyb3_IsKeyPressed(KEYCODE_JOY_BUTTON3)
			|| Keyb3_IsKeyPressed(KEYCODE_JOY_BUTTON4))
		{
			break;
		}
#endif
	}
}
