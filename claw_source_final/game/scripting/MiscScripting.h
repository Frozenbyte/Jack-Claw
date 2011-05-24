
#ifndef MISCSCRIPTING_H
#define MISCSCRIPTING_H

namespace util
{
	class ScriptProcess;
}

namespace game
{
	class Game;
	class GameScriptData;

	class MiscScripting
	{
		public:			
			/** 
			 * Just processes one command...
			 */
			static void MiscScripting::process(util::ScriptProcess *sp, 
				int command, int intData, char *stringData, ScriptLastValueType *lastValue, 
				GameScriptData *gsd, Game *game, bool *pause);
	};
}

#endif


