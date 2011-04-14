// Copyright 2002-2004 Frozenbyte Ltd.

#pragma warning(disable:4103)

//------------------------------------------------------------------
// Includes
//------------------------------------------------------------------
#include "storm3d_adapter.h"
#include "..\..\util\Debug_MemoryManager.h"



//------------------------------------------------------------------
// Storm3D_Adapter::Storm3D_Adapter
//------------------------------------------------------------------
Storm3D_Adapter::Storm3D_Adapter() :
	display_modes(NULL),
	active_display_mode(0),
	caps(0),
	multisample(0),
	max_anisotropy(0),
	multitex_layers(0),
	maxPixelShaderValue(1.f),
	stretchFilter(false)
{
}



//------------------------------------------------------------------
// Storm3D_Adapter::~Storm3D_Adapter
//------------------------------------------------------------------
Storm3D_Adapter::~Storm3D_Adapter()
{
	if (display_modes) delete[] display_modes;
}


