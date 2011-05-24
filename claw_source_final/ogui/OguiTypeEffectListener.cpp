
#include "precompiled.h"

#include  "OguiTypeEffectListener.h"
#include "../game/GameUI.h"
#include "../game/Game.h"

using namespace game;

///////////////////////////////////////////////////////////////////////////////

OguiTypeEffectListener::OguiTypeEffectListener( Game* game, const std::string& effectFile ) :
  game( game ),
  effectFile( effectFile )
{

}

//.............................................................................

OguiTypeEffectListener::~OguiTypeEffectListener()
{
}

///////////////////////////////////////////////////////////////////////////////

void OguiTypeEffectListener::EffectEvent( OguiEffectEvent* eve )
{
	if( eve->eventType == OguiEffectEvent::EVENT_TYPE_TEXTTYPE )
	{
		game->gameUI->playGUISound( effectFile.c_str() );	
	}
}

///////////////////////////////////////////////////////////////////////////////
