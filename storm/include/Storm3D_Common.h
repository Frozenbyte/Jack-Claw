/*

  Storm3D v2.0 T&L Graphics Engine
  (C) Sebastian Aaltonen 2000-2001

  Storm3D_Common defines

*/


#pragma once


//------------------------------------------------------------------
// Defines for DLL export/import
//------------------------------------------------------------------
#if defined(__GNUC__) || defined (DISABLE_STORM_DLL)
	// on targets using gcc we build a single monolithic binary
	#define ST3D_EXP_DLLAPI
	#define ST3D_IMP_DLLAPI

#else

#ifdef STORM3DV2_EXPORTS	// Do not define this inside your program!
	#define ST3D_EXP_DLLAPI __declspec(dllexport)
	#define ST3D_IMP_DLLAPI __declspec(dllimport)
#else
	#define ST3D_EXP_DLLAPI __declspec(dllimport)
	#define ST3D_IMP_DLLAPI __declspec(dllexport)
#endif  // STORM3DV2_EXPORTS

#endif  // __GNUC__


//------------------------------------------------------------------
// Iterator template stuff
//------------------------------------------------------------------
template <class A> class ST3D_EXP_DLLAPI Iterator
{
	
public:

	virtual void Next()=0;
	virtual bool IsEnd()=0;

	virtual A GetCurrent()=0;

	virtual ~Iterator() {}
};



template <class A> class ST3D_EXP_DLLAPI ICreate
{
	
public:
	virtual ~ICreate() {}
	virtual Iterator<A> *Begin()=0;
};


