
#ifndef WATERSCRIPTING_H
#define WATERSCRIPTING_H

namespace util
{
	class ScriptProcess;
}

namespace game
{
	class Game;
	class GameScriptData;

	/** 
	 * Water related scripting.
	 */
	class WaterScripting
	{
		public:			
			/** 
			 * Just processes one command...
			 */
			static void WaterScripting::process(util::ScriptProcess *sp, 
				int command, int intData, char *stringData, ScriptLastValueType *lastValue, 
				GameScriptData *gsd, Game *game);
	};
}

#endif




