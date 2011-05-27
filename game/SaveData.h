
#ifndef SAVEDATA_H
#define SAVEDATA_H


// FIXME : standard stdint.h header is missing on MSVC
#ifdef _WIN32
typedef unsigned char uint8_t;
#else
#include <stdint.h>
#endif

namespace game
{

  class GameObject;


  class SaveData
  {
  public:
    SaveData(int id, int size, BYTE *data, int childAmount = 0, 
      GameObject **children = NULL);
    ~SaveData();

  // public just for easy access in save routines, don't modify directly
  // private:
    int id;
    int size;
	uint8_t *data;
    GameObject **children;
  };

}

#endif

