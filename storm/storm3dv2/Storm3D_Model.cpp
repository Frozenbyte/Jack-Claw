// Copyright 2002-2004 Frozenbyte Ltd.

#pragma warning(disable:4103)

//------------------------------------------------------------------
// Includes
//------------------------------------------------------------------
#include "S3D_ModelFile.h"
#include "storm3d.h"
#include "storm3d_model.h"
#include "storm3d_model_object.h"
#include "storm3d_light.h"
#include "storm3d_helper.h"
#include "storm3d_mesh.h"
#include "Iterator.h"

// psd
#include <vector>
#include <map>
#include <string>
#include <stdio.h>
#include <cassert>
#include <algorithm>
#include <functional>
#include <boost/lexical_cast.hpp>
#include "Storm3D_Bone.h"
#include "IStorm3D_Logger.h"
#include "../../filesystem/input_stream_wrapper.h"
#include "../../util/Debug_MemoryManager.h"

#ifdef WORLD_FOLDING_ENABLED
#include "WorldFold.h"
#endif

using namespace std;


#ifdef BONE_MODEL_SPHERE_TRANSFORM
#pragma message("--- NOTICE!!! Bounding sphere transform enabled for models with bones! ---") 
#pragma message("---           This may cause inaccuracies to raytraces. (optimization) ---") 
#endif
// -- jpk

// using fixed points for speed factor (advanced time calculation)
// trying to avoid one float -> int cast. :)
#define MODEL_BONEANIMATION_SPEED_FACTOR_MULT 256
#define MODEL_BONEANIMATION_SPEED_FACTOR_SHIFT 8

static const float BOUNDING_INITIAL_RADIUS = 20.0f;
//static const float BOUNDING_INITIAL_RADIUS = 5.0f;

// HACK: for statistics
int storm3d_model_boneanimation_allocs = 0;
int storm3d_model_loads = 0;
int storm3d_model_bone_loads = 0;
int storm3d_model_objects_created = 0;
int storm3d_model_allocs = 0;

using namespace frozenbyte;

namespace {

	bool hasInfiniteCylinderCollision(const VC3 &rayOrigin, const VC3 &rayEnd, const VC3 &cylinderPoint1, const VC3 &cylinderPoint2, float cylinderRadius)
	{
		VC3 d = cylinderPoint2 - cylinderPoint1;
		VC3 m = rayOrigin - cylinderPoint1;
		VC3 n = rayEnd - rayOrigin;

		float md = m.GetDotWith(d);
		float nd = n.GetDotWith(d);
		float dd = d.GetDotWith(d);

		if(md < 0.0f && md + nd < 0.0f)
			return false;
		if(md > dd && md + nd > dd)
			return false;

		float nn = n.GetDotWith(n);
		float mn = m.GetDotWith(n);
		float a = (dd * nn) - (nd * nd);
		float k = m.GetDotWith(m) - cylinderRadius*cylinderRadius;
		float c = (dd * k) - (md * md);
		float t = 0.f;

		if(fabsf(a) < 0.001f)
		{
			if(c > 0.0f)
				return false;

			if(md < 0.0f)
				t = -mn / nn;
			else if(md > dd)
				t = (nd - mn) / nn;
			else
				t = 0.0f;

			return true;
		}

		float b = (dd * mn) - (nd * md);
		float discr = (b * b) - (a * c);
		if(discr < 0.0f)
			return false;

		t = (-b - sqrtf(discr)) / a;
		if(t < 0.f || t > 1.0f)
			return false;

		if(md + (t * nd) < 0.0f)
		{
			if(nd <= 0.0f)
				return false;

			t = -md / nd;
			return k + (2.f * t * (mn + (t * nn))) <= 0.0f;
		}
		else if(md + (t * nd) > dd)
		{
			if(nd >= 0.0f)
				return false;

			t = (dd - md) / nd;
			return k + dd - ((2.f * md) + (t * (2.f * (mn - nd)) + (t * nn))) <= 0.0f;
		}

		return true;
	}

} // unnamed

Model_BoneAnimation::Model_BoneAnimation()
:	transition(0),
	animation(NULL),
	animation_loop(false),
	blend_time(0),
	animation_time(0),
	elapsed_time(0),
	speed_factor(MODEL_BONEANIMATION_SPEED_FACTOR_MULT),
	state(Normal),
	blend_factor(1.f)
{
	storm3d_model_boneanimation_allocs++;
}

Model_BoneAnimation::Model_BoneAnimation(const Model_BoneAnimation &animation_)
:	transition(0),
	animation(NULL),
	animation_loop(false),
	blend_time(0),
	animation_time(0),
	elapsed_time(0),
	speed_factor(MODEL_BONEANIMATION_SPEED_FACTOR_MULT),
	state(Normal),
	blend_factor(1.f)
{
	*this = animation_;
	storm3d_model_boneanimation_allocs++;
}


Model_BoneAnimation::Model_BoneAnimation(Storm3D_BoneAnimation *transition_, Storm3D_BoneAnimation *animation_, bool looping, int blend_time_, int animation_time_)
:	transition(transition_),
	animation(animation_),
	animation_loop(looping),
	blend_time(blend_time_),
	animation_time(animation_time_),
	elapsed_time(0),
	speed_factor(MODEL_BONEANIMATION_SPEED_FACTOR_MULT),
	state(Normal),
	blend_factor(1.f)
{
	if(animation)
		animation->AddReference();
	if(transition)
		transition->AddReference();
	storm3d_model_boneanimation_allocs++;
}
	
Model_BoneAnimation::~Model_BoneAnimation()
{
	if(transition)
		transition->Release();
	if(animation)
		animation->Release();
	storm3d_model_boneanimation_allocs--;
}

void Model_BoneAnimation::AdvanceAnimation(int time_delta, bool doUpdate)
{
	if(animation == 0)
		return;

	// time delta scaled based on speed_factor
	// --jpk
	time_delta = (time_delta * speed_factor) >> MODEL_BONEANIMATION_SPEED_FACTOR_SHIFT;
	
	// FIXME: proper implementation...
	// now we may sometimes scale the time_delta to zero.
	// (if frame time small enough)
	// -> results to animation never advancing
	// the time delta should be calculated differently, so that values
	// small enough to be zeroed will never even be called, the delta
	// is increased until it is big enough to be factored.
	// quick hack to prevent zeros:
	//if (time_delta == 0) time_delta = 1;
	// can't do this, cos time_delta == 0 used when animations paused.


	elapsed_time += time_delta;
	
	//if(state != Normal && !doUpdate)
	//	return;
	// We need to loop even when blending
	//if(!doUpdate)
	//	return;

	animation_time += time_delta;

	if(transition)
	{
		if(animation_time >= transition->GetLength())
		{
			animation_time -= transition->GetLength();
			transition->Release();
			transition = 0;
		}
	}
	else
	{
		if(animation_loop == true)
		{
			// Proper looping
			animation_time %= animation->GetLength();
		}
		else
		{
			// Else set to final pose
			if(animation_time >= animation->GetLength())
				animation_time = animation->GetLength() - 1;
		}
	}
}

void Model_BoneAnimation::SetSpeedFactor(float speed_factor)
{
	this->speed_factor = (int)(MODEL_BONEANIMATION_SPEED_FACTOR_MULT * speed_factor);
	// to make sure we don't slow it down too much...
	// (1/8 speed slowmotion at max.)
	if (this->speed_factor < MODEL_BONEANIMATION_SPEED_FACTOR_MULT / 8)
		this->speed_factor = MODEL_BONEANIMATION_SPEED_FACTOR_MULT / 8;
}

void Model_BoneAnimation::SetBlendFactor(float value)
{
	blend_factor = value;
}

void Model_BoneAnimation::SetLooping(bool loop)
{
	animation_loop = loop;
}

void Model_BoneAnimation::SetState(AnimationState state_, int blend_time_)
{
	// Test
/*	if(state == BlendIn && state_ == BlendOut)
	{
		float blend_factor = float(elapsed_time) / float(blend_time);
		state = state_;
		blend_time = blend_time_;
		elapsed_time = int((1.f - blend_factor) * blend_time);
	}
	else if(state == state_)
	{
		// Nop
	}
	else
	*/{
		state = state_;
		blend_time = blend_time_;
		elapsed_time = 0;
	}
}

Model_BoneAnimation &Model_BoneAnimation::operator = (const Model_BoneAnimation &animation_)
{
	if(this->animation != animation_.animation)
	{
		if(animation)
			animation->Release();
		if(transition)
			transition->Release();
	
		animation = animation_.animation;
		transition = animation_.transition;

		if(animation)
			animation->AddReference();
		if(transition)
			transition->AddReference();
	}

	animation_time = animation_.animation_time;
	elapsed_time = animation_.elapsed_time;

	speed_factor = animation_.speed_factor;	
	blend_factor = animation_.blend_factor;	

	blend_time = animation_.blend_time;
	animation_loop = animation_.animation_loop;
	state = animation_.state;
	return *this;
}

bool Model_BoneAnimation::operator < (const Model_BoneAnimation &animation) const
{
	int t1 = blend_time - elapsed_time;
	int t2 = animation.blend_time - animation.elapsed_time;

	// oh man... this is a problem, speed factor may change this result 
	t1 = (t1 * MODEL_BONEANIMATION_SPEED_FACTOR_SHIFT) / speed_factor;
	t2 = (t2 * MODEL_BONEANIMATION_SPEED_FACTOR_SHIFT) / animation.speed_factor;

	return t1 < t2;
}


bool Model_BoneAnimation::GetRotation(int bone_index, Rotation *result) const
{
	if(transition)
	{
		assert(animation_time < transition->GetLength());
		return transition->GetRotation(bone_index, animation_time, result);
	}
	else if(animation)
	{
		assert(animation_time < animation->GetLength());
		return animation->GetRotation(bone_index, animation_time, result);
	}
	else
		return false;
}

bool Model_BoneAnimation::GetPosition(int bone_index, Vector *result) const
{
	if(transition)
	{
		assert(animation_time < transition->GetLength());
		return transition->GetPosition(bone_index, animation_time, result);
	}
	else if(animation)
	{
		assert(animation_time < animation->GetLength());
		return animation->GetPosition(bone_index, animation_time, result);
	}
	else
		return false;
}

bool Model_BoneAnimation::GetBlendProperties(float *blend_factor) const
{
	if(blend_time == 0)
		return false;

	// Linear increase
	*blend_factor = float(elapsed_time) / float(blend_time);

	{
		//*blend_factor = sinf(PI * (*blend_factor - 0.5f)) *0.5f + 0.5f;
		//*blend_factor *= PI/2;
		//*blend_factor = sinf(*blend_factor);
		//*blend_factor = sinf(PI * (*blend_factor - 0.5f)) *0.5f + 0.5f-0.27f*(-1+cosf(2*PI*(*blend_factor)));
	}

	
	if(state == BlendOut)
		*blend_factor = 1.f - *blend_factor;

	return true;
}

int Model_BoneAnimation::GetAnimationTime() const
{
	return animation_time;
}

Storm3D_BoneAnimation *Model_BoneAnimation::GetAnimation() const
{
	return animation;
}

bool Model_BoneAnimation::HasAnimation() const
{
	if(animation)
		return true;
	return false;
}

bool Model_BoneAnimation::HasEnded() const
{
	if(animation)
	{
		if(elapsed_time >= blend_time)
			return true;
	}

	return false;
}

Model_BoneAnimation::AnimationState Model_BoneAnimation::GetState() const
{
	return state;
}

/***/

Model_BoneBlend::Model_BoneBlend()
{
	state = Model_BoneAnimation::Normal;
	bone_index = -1;
	blend_time = 0;
	elapsed_time = 0;
}

Model_BoneBlend::Model_BoneBlend(int bone_index_, const QUAT rotation_, int blend_time_)
{
	state = Model_BoneAnimation::BlendIn;
	rotation = rotation_;
	bone_index = bone_index_;
	blend_time = blend_time_;
	elapsed_time = 0;
}

void Model_BoneBlend::AdvanceAnimation(int time_delta)
{
	elapsed_time += time_delta;
	if(elapsed_time > blend_time && state == Model_BoneAnimation::BlendIn)
	{
		elapsed_time = 0;
		state = Model_BoneAnimation::Normal;
	}
}

void Model_BoneBlend::SetState(Model_BoneAnimation::AnimationState state_, int blend_time_)
{
	elapsed_time = 0;
	state = state_;
	blend_time = blend_time_;
}

bool Model_BoneBlend::GetBlendProperties(float *blend_factor) const
{
	if(blend_time == 0)
		return false;

	// Linear increase
	*blend_factor = (float) (elapsed_time) / blend_time;
	if(state == Model_BoneAnimation::BlendOut)
		*blend_factor = 1.f - *blend_factor;

	return true;
}

bool Model_BoneBlend::GetRotation(int bone_index_, Rotation *result) const
{
	if(bone_index != bone_index_)
		return false;

	*result = rotation;
	return true;
}

Model_BoneAnimation::AnimationState Model_BoneBlend::GetState() const
{
	return state;
}

bool Model_BoneBlend::HasEnded() const
{
	if(state == Model_BoneAnimation::BlendOut)
		return (elapsed_time >= blend_time);
	else
		return false;
}

int Model_BoneBlend::GetIndex() const
{
	return bone_index;
}

//------------------------------------------------------------------
// Storm3D_Model::Render
//------------------------------------------------------------------
/*void Storm3D_Model::Render(Storm3D_Scene &scene)
{
	
	// Render each object in set
	for(set<Storm3D_Model_Object*>::iterator io=objects.begin();io!=objects.end();io++)
	{
		(*io)->Render(scene);
	}
}*/


// Simple helper function
std::string LoadStringFromFile(filesystem::FB_FILE *fp)
{
	std::string fname;

	while(true)
	{
		char c = 0; //fgetc(fp);
		filesystem::fb_fread(&c, 1, 1, fp);
		fname += c;

		if(c == '\0')
			break;
	}

	/*
	if(fname.size() > 0)
	{
		char *filename = new char[fname.size()];
		for(int i = 0; i < fname.size(); i++)
			filename[i] = fname[i];
	
		return filename;
	}
	*/

	return fname;
}

void Storm3D_Model::updateEffectTexture()
{
	effectTextureName = std::string();
	std::set<IStorm3D_Model_Object *>::iterator it = objects.begin();
	for(; it != objects.end(); ++it)
	{
		Storm3D_Material *m = static_cast<Storm3D_Material *> ((*it)->GetMesh()->GetMaterial());
		const std::string &name = m->getEffectTextureName();

		if(!name.empty())
			effectTextureName = name;
	}
}

bool Storm3D_Model::fits(const AABB &area)
{
	if(!bones.empty())
		return false;

	const AABB &volume = GetBoundingBox();

	if(volume.mmin.x > area.mmin.x)
	if(volume.mmin.z > area.mmin.z)
	if(volume.mmax.x < area.mmax.x)
	if(volume.mmax.z < area.mmax.z)
		return true;

	return false;
}

//------------------------------------------------------------------
// Storm3D_Model::LoadS3D
//------------------------------------------------------------------
bool Storm3D_Model::LoadS3D(const char *filename)
{
	// First empty model (delete everything)
//	Empty(false, true);

	if (Storm3D2->getLogger() != NULL)
	{
		Storm3D2->getLogger()->debug("Storm3D_Model::LoadS3D - Loading a model file.");
		Storm3D2->getLogger()->debug(filename);
	}

	bool disableShadows = false;

	bool mirrored = false;
	bool mirroredX = false;
	bool mirroredY = false;
	bool mirroredZ = false;
	bool mirroredU = false;
	bool mirroredV = false;
	bool sideways = false;

	bool scaled = false;
	float scaledX = 1.0f;
	float scaledY = 1.0f;
	float scaledZ = 1.0f;

	bool blackEdge = false;
	float blackEdgeSize = 0.0f;
	float blackEdgeU = -1.0f;
	float blackEdgeV = -1.0f;

	bool fatboy = false;
	float fatboySize = 0.0f;

	bool reflectionMask = false;
	bool delayedAlpha = false;
	bool earlyAlpha = false;
	bool alphaTestPass = false;
	bool conditionalAlphaTestPass = false;
	int alphaTestValue = 0x70;

	if(!filename || strlen(filename) < 4)
		return false;

	// Load new data...

	// the haxored load time modifications to model...
	// foobar.s3d@90 - "hard vertex rotation" 
	// foobar.s3d@F0.01 - "fatboy" (a.k.a. LW smooth scale / move verteices on vertex normals)
	// foobar.s3d@B0.01 - "blackedge" (duplicate vertices, move on vertex normals, set UV to 0,0 - assuming that is black)
	// foobar.s3d@BU0.0 - define blackedge u texture coordinate instead of 0
	// foobar.s3d@BV0.0 - define blackedge v texture coordinate instead of 0
	// foobar.s3d@NS - no shadows (this model does not get rendered in depth buffer)
	// foobar.s3d@RM - "reflection mask" (masks the cheap-ass-reflection in depth buffer)
	// foobar.s3d@DA - "delayed alpha" (delay the rendering of this alpha object - assuming it really is an alpha object!)
	// foobar.s3d@EA - "early alpha" (make the rendering of this alpha object earlier - assuming it really is an alpha object!)
	// foobar.s3d@ATP123 - "alpha test pass" (_always_ make the rendering of this alpha blended object to be rendered with alpha test pass too)
	// foobar.s3d@CATP123 - "conditional alpha test pass" (_if such option is on_, make the rendering of this alpha blended object to be rendered with alpha test pass too)
	// foobar.s3d@MX / MY / MZ / MU / MV - "mirrored" (mirrored vertices in x/y/z or direction or mirrored textures in u/v direction)
	// foobar.s3d@SI - "sideways" (rotate object so that y axis is rotated on z axis, topdown game model to sideways game model)
	// foobar.s3d@SA1.1 / SX.. / SY.. / SZ.. - "scale" (scale object uniformly in all axises or non-uniform scaling in single axis)
	// foobar.s3d@[name] "remark/duplicate" (no modification to object in any special way, can be used to make duplicate identifications of same model)
	// --jpk


	char actual_filename[256+1];
	int filename_len = strlen(filename);
	int hard_rotation = 0;
	if (filename_len < 256) 
	{
		strcpy(actual_filename, filename);
		for (int i = 0; i < filename_len; i++)
		{
			if (actual_filename[i] == '@')
			{
				int restoreAtPos = -1;
				for (int j = i+1; j < filename_len; j++)
				{
					if (actual_filename[j] == '@')
					{
						actual_filename[j] = '\0';
						restoreAtPos = j;
						break;
					}
				}
				actual_filename[i] = '\0';
				if (actual_filename[i+1] == 'B')
				{
					if (actual_filename[i+2] == 'U')
					{
						blackEdgeU = (float)atof(&actual_filename[i + 3]);
					}
					else if (actual_filename[i+2] == 'V')
					{
						blackEdgeV = (float)atof(&actual_filename[i + 3]);
					} else {
						blackEdge = true;
						blackEdgeSize = (float)atof(&actual_filename[i + 2]);
					}
				}
				if (actual_filename[i+1] == 'N')
				{
					if (actual_filename[i+2] == 'S')
					{
						disableShadows = true;
					}
				}
				else if (actual_filename[i+1] == 'M')
				{
					if (actual_filename[i+2] == 'X')
					{
						mirrored = true;
						mirroredX = true;
					}
					if (actual_filename[i+2] == 'Y')
					{
						mirrored = true;
						mirroredY = true;
					}
					if (actual_filename[i+2] == 'Z')
					{
						mirrored = true;
						mirroredZ = true;
					}
					if (actual_filename[i+2] == 'U')
					{
						mirrored = true;
						mirroredU = true;
					}
					if (actual_filename[i+2] == 'V')
					{
						mirrored = true;
						mirroredV = true;
					}
				} 
				else if (actual_filename[i+1] == 'S')
				{
					if (actual_filename[i+2] == 'A')
					{
						scaled = true;
						scaledX = (float)atof(&actual_filename[i + 3]);
						scaledY = scaledX;
						scaledZ = scaledX;
					}
					if (actual_filename[i+2] == 'X')
					{
						scaled = true;
						scaledX = (float)atof(&actual_filename[i + 3]);
					}
					if (actual_filename[i+2] == 'Y')
					{
						scaled = true;
						scaledY = (float)atof(&actual_filename[i + 3]);
					}
					if (actual_filename[i+2] == 'Z')
					{
						scaled = true;
						scaledZ = (float)atof(&actual_filename[i + 3]);
					}
					if (actual_filename[i+2] == 'I')
					{
						sideways = true;
					} 
				} 
				else if (actual_filename[i+1] == 'F')
				{
					fatboy = true;
					fatboySize = (float)atof(&actual_filename[i + 2]);
				} 
				else if (actual_filename[i+1] == 'R'
					&& actual_filename[i+2] == 'M')
				{
					reflectionMask = true;
				} 
				else if (actual_filename[i+1] == 'D'
					&& actual_filename[i+2] == 'A')
				{
					delayedAlpha = true;
				} 
				else if (actual_filename[i+1] == 'E'
					&& actual_filename[i+2] == 'A')
				{
					earlyAlpha = true;
				} 
				else if (actual_filename[i+1] == 'A'
					&& actual_filename[i+2] == 'T'
					&& actual_filename[i+3] == 'P')
				{
					alphaTestPass = true;
					if (actual_filename[i+4] >= '0' && actual_filename[i+4] <= '9')
						alphaTestValue = atoi(&actual_filename[i + 4]);
				} 
				else if (actual_filename[i+1] == 'C'
					&& actual_filename[i+2] == 'A'
					&& actual_filename[i+3] == 'T'
					&& actual_filename[i+4] == 'P')
				{
					conditionalAlphaTestPass = true;
					if (actual_filename[i+5] >= '0' && actual_filename[i+5] <= '9')
						alphaTestValue = atoi(&actual_filename[i + 5]);
				} 
				else if (actual_filename[i+1] >= '0' && actual_filename[i+1] <= '9')
				{
					hard_rotation = atoi(&actual_filename[i + 1]);
					//assert(hard_rotation != 0);
				}
				if (restoreAtPos != -1)
				{
					actual_filename[restoreAtPos] = '@';
				}
			}	
		}
	} else {
		assert(0);
		return false;
	}
	
	// Open file
	filesystem::FB_FILE *f = filesystem::fb_fopen(actual_filename,"rb");
	if (f==NULL)
	{
		//char s[256];
		//if (strlen(actual_filename < 200))
		//{
		//sprintf(s,"File not found (%s)",actual_filename);
		//MessageBox(NULL,s,"Model load error",0);
		//}
		return false;	// File is not found? (no loading, no crashing TM)
	}

	// 10 -> lod index buffers (3)
	// 9 -> 2x texture coordinates
	// 8 -> FB's bone version without bones ;-)
	// 7 -> FB's version with bones
	// 6 -> FB's version without bones
	int s3d_version = 7;

	// Read header data
	S3D_HEADER header;
	filesystem::fb_fread(header.id, sizeof(header.id), 1, f);
	//filesystem::fb_fread(&header,sizeof(S3D_HEADER),1,f);

	// Test file format/version
	if (memcmp(header.id,"S3D7",4)!=0)
	{
		if (memcmp(header.id,"S3D",3)!=0)
		{
			//MessageBox(NULL,"File is not in S3D format!","Model load error",0);
			filesystem::fb_fclose(f);
			return false;
		}
		else 
		{
			s3d_version = header.id[3] - '0';

			if(s3d_version == 0)
				filesystem::fb_fread(&s3d_version, 1, sizeof(int), f);
			else if((s3d_version != 6) && (s3d_version != 7) && (s3d_version != 8) && (s3d_version != 9))
			{
				filesystem::fb_fclose(f);
				return false;
			}
		}
	}

	// Read rest of the header
	filesystem::fb_fread(&header.num_textures, sizeof(WORD), 1, f);
	filesystem::fb_fread(&header.num_materials, sizeof(WORD), 1, f);
	filesystem::fb_fread(&header.num_objects, sizeof(WORD), 1, f);
	filesystem::fb_fread(&header.num_lights, sizeof(WORD), 1, f);
	filesystem::fb_fread(&header.num_helpers, sizeof(WORD), 1, f);

	if(s3d_version <= 7)
	{
		// Bones no longer here
		WORD foo;
		filesystem::fb_fread(&foo, sizeof(WORD), 1, f);
	}
	else
	{
		// Load bone id instead
		filesystem::fb_fread(&model_boneid, sizeof(int), 1, f);

		// If bones already loaded and not compatible, return
		if((bone_boneid != 0) && (model_boneid != bone_boneid))
		{
			filesystem::fb_fclose(f);
			return false;
		}
	}

	if(s3d_version < 9)
	{
		filesystem::fb_fclose(f);
		return false;
	}

	// Separate path from filename
	char path[256];
	strcpy(path,filename);
	for (int i=strlen(path);(i>=0)&&(filename[i]!='\\')&&(filename[i]!='/');i--) path[i]=0;

	// Read textures
	char temptexname[256];
	IStorm3D_Texture *texhandles[500];	// temporary texturehandles
	int texn=0;
	for(int i=0;i<header.num_textures;i++)
	{
		// Read data
		S3D_TEXTURE tex;
		
		if(s3d_version >= 6)
		{
			tex.filename = LoadStringFromFile(f);
			filesystem::fb_fread(&tex.identification, sizeof(DWORD), 1, f);
			filesystem::fb_fread(&tex.start_frame, sizeof(WORD), 1, f);
			filesystem::fb_fread(&tex.framechangetime, sizeof(WORD), 1, f);
			filesystem::fb_fread(&tex.dynamic, sizeof(BYTE), 1, f);
		}
		
		// Generate path+texfilename (loads textures from the same path as model)
		// v3
		strcpy(temptexname,path);
		strcat(temptexname,tex.filename.c_str());

		// Create new texture (and put it into temporary texture list)
		texhandles[texn++]=Storm3D2->CreateNewTexture(temptexname,tex.identification);
	}

	// Read materials
	IStorm3D_Material *mathandles[500];	// temporary materialhandles
	int matn=0;
	for(int i=0;i<header.num_materials;i++)
	{
		// Read data
		S3D_MATERIAL mat;
		short int texture_distortion = -1;

		if(s3d_version >= 6)
		{
			mat.name = LoadStringFromFile(f);
			
			filesystem::fb_fread(&mat.texture_base, sizeof(short int), 1, f);
			filesystem::fb_fread(&mat.texture_base2, sizeof(short int), 1, f);
			filesystem::fb_fread(&mat.texture_bump, sizeof(short int), 1, f);
			filesystem::fb_fread(&mat.texture_reflection, sizeof(short int), 1, f);

			if(s3d_version >= 14)
				filesystem::fb_fread(&texture_distortion, sizeof(short int), 1, f);

			filesystem::fb_fread(mat.color, sizeof(float), 3, f);
			filesystem::fb_fread(mat.self_illum, sizeof(float), 3, f);
			filesystem::fb_fread(mat.specular, sizeof(float), 3, f);
			filesystem::fb_fread(&mat.specular_sharpness, sizeof(float), 1, f);
			filesystem::fb_fread(&mat.doublesided, sizeof(bool), 1, f);
			filesystem::fb_fread(&mat.wireframe, sizeof(bool), 1, f);
			filesystem::fb_fread(&mat.reflection_texgen, sizeof(int), 1, f);
			filesystem::fb_fread(&mat.alphablend_type, sizeof(int), 1, f);
			filesystem::fb_fread(&mat.transparency, sizeof(float), 1, f);
		}

		if(s3d_version >= 12)
			filesystem::fb_fread(&mat.glow, sizeof(float), 1, f);

		VC2 scrollSpeed;
		bool scrollAutoStart = false;

		if(s3d_version >= 13)
		{
			filesystem::fb_fread(&scrollSpeed, sizeof(float), 2, f);
			char scrollStart = 0;
			filesystem::fb_fread(&scrollStart, sizeof(char), 1, f);
			if(scrollStart)
				scrollAutoStart = true;
		}

		// Create new material (and put it into temporary texture list)
		Storm3D_Material *tmat= static_cast<Storm3D_Material *> (Storm3D2->CreateNewMaterial(mat.name.c_str()));
		mathandles[matn++]=tmat;
		
		// Set material's properties
		tmat->SetColor(COL(mat.color));
		tmat->SetSelfIllumination(COL(mat.self_illum));
		tmat->SetSpecular(COL(mat.specular),mat.specular_sharpness);
		tmat->SetSpecial(mat.doublesided,mat.wireframe);
		tmat->SetAlphaType(mat.alphablend_type);
		tmat->SetTransparency(mat.transparency);
		tmat->SetGlow(mat.glow);
		tmat->SetScrollSpeed(scrollSpeed);
		tmat->EnableScroll(scrollAutoStart);

		// Set material's textures
		if (mat.texture_base>=0) 
			tmat->SetBaseTexture(texhandles[mat.texture_base]);
		if (mat.texture_base2>=0) 
			tmat->SetBaseTexture2(texhandles[mat.texture_base2]);
		if (mat.texture_bump>=0) 
			tmat->SetBumpTexture(texhandles[mat.texture_bump]);
		if (mat.texture_reflection>=0) 
			tmat->SetReflectionTexture(texhandles[mat.texture_reflection]);
		if (texture_distortion>=0) 
			tmat->SetDistortionTexture(texhandles[texture_distortion]);
		
		// Texturelayer special properties (for base2 and reflection only)
		if (mat.texture_base2>=0)
		{
			// Read data
			S3D_MATERIAL_TEXTURELAYER tlayer;
			filesystem::fb_fread(&tlayer,sizeof(S3D_MATERIAL_TEXTURELAYER),1,f);
			tmat->ChangeBaseTexture2Parameters(tlayer.blend_op,tlayer.blend_factor);

			//MessageBox(NULL,"DEBUG","SITEX",0);
		}

		if (mat.texture_reflection>=0)
		{
			// Read data
			S3D_MATERIAL_TEXTURELAYER tlayer;
			filesystem::fb_fread(&tlayer,sizeof(S3D_MATERIAL_TEXTURELAYER),1,f);
			tmat->ChangeReflectionTextureParameters(tlayer.blend_op,tlayer.blend_factor,mat.reflection_texgen);
		}

		const char *searchString = "Reflection(";
		const char *reflectionString = strstr(tmat->GetName(), searchString);
		if(reflectionString != 0)
		{
			//MessageBox(0, reflectionString + strlen(searchString), "Woot?!?", MB_OK);
			//float factor = float(atof(reflectionString + strlen(searchString)));
			std::string temp = reflectionString + strlen(searchString);
			for(unsigned int i = 0; i < temp.size(); ++i)
			{
				if(temp[i] == ')')
					temp[i] = ' ';
			}

			float factor = float(atof(temp.c_str()));
			if(factor > 0.001f && factor <= 1.0001f)
				tmat->SetLocalReflection(true, factor);
		}
	}

	// Read objects
	for(int i=0;i<header.num_objects;i++)
	{
		// Read data
		S3D_OBJECT obj;
		bool has_weights = false;
		bool hasLods = false;
		bool blackEdgeThisObject = false;

		int originalVertexAmount = 0;
		int originalFaceAmount = 0;
		int vertexAmountMult = 1;
		int faceAmountMult = 1;

		if(s3d_version >= 6)
		{
			obj.name = LoadStringFromFile(f);
			obj.parent = LoadStringFromFile(f);
			
			filesystem::fb_fread(&obj.material_index, sizeof(short int), 1, f);
			assert(obj.material_index < header.num_materials);

			// only non-alphablended/non-alphatested objects get black edges
			if (blackEdge)
			{
				if (mathandles[obj.material_index]->GetAlphaType() == IStorm3D_Material::ATYPE_NONE)
				{
					blackEdgeThisObject = true;
				}
			}

			filesystem::fb_fread(obj.position, sizeof(float), 3, f);
			filesystem::fb_fread(obj.rotation, sizeof(float), 4, f);
			filesystem::fb_fread(obj.scale, sizeof(float), 3, f);

			// the haxored hard vertex rotation...
			// -- jpk
			if (hard_rotation != 0)
			{
				// TODO: rotate rotation too
				// rotation.y += hard_rotation (changed to radians)
				float origp0 = obj.position[0];
				obj.position[0] = obj.position[0] * cosf((float)hard_rotation * 3.1415927f / 180) + obj.position[2] * sinf((float)hard_rotation * 3.1415927f / 180);
				obj.position[2] = obj.position[2] * cosf((float)hard_rotation * 3.1415927f / 180) - origp0 * sinf((float)hard_rotation * 3.1415927f / 180);
			}
			if (mirrored)
			{
				if (mirroredX)
					obj.position[0] = -obj.position[0];
				if (mirroredY)
					obj.position[1] = -obj.position[1];
				if (mirroredZ)
					obj.position[2] = -obj.position[2];
			}
			if (scaled)
			{
				if (scaledX)
					obj.position[0] *= scaledX;
				if (scaledY)
					obj.position[1] *= scaledY;
				if (scaledZ)
					obj.position[2] *= scaledZ;
			}
			if (sideways)
			{
				float origp1 = obj.position[1];
				obj.position[1] = -obj.position[2];
				obj.position[2] = origp1;
			}
			
			char foo = 0;
			filesystem::fb_fread(&foo, sizeof(char), 1, f);
			if(foo == 1)
				obj.no_collision = true;
			else
				obj.no_collision = false;
			filesystem::fb_fread(&foo, sizeof(char), 1, f);
			if(foo == 1)
				obj.no_render = true;
			else
				obj.no_render = false;

			// !!!!!!!!!!!
			// HACK: show the collision models
			//if (!obj.no_collision && obj.no_render)
			//	obj.no_render = false;
			//else
			//	obj.no_render = true;

			if(s3d_version >= 11)
			{
				// lightobject
				filesystem::fb_fread(&foo, sizeof(char), 1, f);

				if(foo == 1)
					obj.light_object = true;
				else
					obj.light_object = false;
			}
			else
				obj.light_object = false;

			if(s3d_version <= 7)
			{
				// Not used anymore
				bool is_volume_fog = false;
				filesystem::fb_fread(&is_volume_fog, sizeof(bool), 1, f);

				filesystem::fb_fread(&obj.is_mirror, sizeof(bool), 1, f);
				filesystem::fb_fread(&obj.shadow_level, sizeof(BYTE), 1, f);
				filesystem::fb_fread(&obj.keyframe_endtime, sizeof(int), 1, f);
				filesystem::fb_fread(&obj.lod_amount, sizeof(BYTE), 1, f);
				filesystem::fb_fread(&obj.poskey_amount, sizeof(WORD), 1, f);
				filesystem::fb_fread(&obj.rotkey_amount, sizeof(WORD), 1, f);
				filesystem::fb_fread(&obj.scalekey_amount, sizeof(WORD), 1, f);
				filesystem::fb_fread(&obj.meshkey_amount, sizeof(WORD), 1, f);
			}

			filesystem::fb_fread(&obj.vertex_amount, sizeof(WORD), 1, f);
			filesystem::fb_fread(&obj.face_amount, sizeof(WORD), 1, f);

			originalVertexAmount = obj.vertex_amount;
			originalFaceAmount = obj.face_amount;
			vertexAmountMult = 1;
			faceAmountMult = 1;

			if (blackEdgeThisObject)
			{
				vertexAmountMult = 2;
				faceAmountMult = 2;
				obj.vertex_amount *= vertexAmountMult;
				obj.face_amount *= faceAmountMult;
			}

			char lodFlag = 0;
			if(s3d_version >= 10)
			{
				filesystem::fb_fread(&lodFlag, sizeof(char), 1, f);
				hasLods = (lodFlag != 0);
			}

			if(s3d_version >= 7)
			{
				char foo;
				filesystem::fb_fread(&foo, sizeof(char), 1, f);
				if(foo == 1)
					has_weights = true;
				else
					has_weights = false;
			}
		}

		// obj.shadow_level = 0;

		// Create new object and mesh
		Storm3D_Model_Object *tobj=(Storm3D_Model_Object*)Object_New(obj.name.c_str());
		Storm3D_Mesh *tmesh=(Storm3D_Mesh*)this->Storm3D2->CreateNewMesh();
		//tobj->mesh=tmesh;

		if (reflectionMask)
		{
			tobj->EnableRenderPass(RENDER_PASS_BIT_CAREFLECTION_DEPTH_MASKS);
		}
		if (delayedAlpha)
		{
			tobj->EnableRenderPass(RENDER_PASS_BIT_DELAYED_ALPHA);
		}
		if (earlyAlpha)
		{
			tobj->EnableRenderPass(RENDER_PASS_BIT_EARLY_ALPHA);
		}
		if (alphaTestPass || conditionalAlphaTestPass)
		{
			tobj->EnableRenderPass(RENDER_PASS_BIT_ADDITIONAL_ALPHA_TEST_PASS);
			tobj->SetAlphaTestPassParams(conditionalAlphaTestPass, alphaTestValue);
		}
			
		// Set parent (if has)
		if (strlen(obj.parent.c_str())>0)
		{
			IStorm3D_Model_Object *opar = SearchObject(obj.parent.c_str());
			if (opar) 
				opar->AddChild(tobj);
			else
			{
				// Search for bone parent
				for(unsigned int i = 0; i < bones.size(); ++i)
				{
					if(strcmp(bones[i]->GetName(), obj.parent.c_str()) == 0)
					{
						bones[i]->AddChild(tobj);
						break;
					}
				}
			}
		}

		// Set object's special propertios
		tobj->SetNoCollision(obj.no_collision);
		tobj->SetNoRender(obj.no_render);

		// TEMP: show floormap only
		/*
		if (obj.no_collision && obj.no_render)
		{
			tobj->SetNoRender(false);
		} else {
			tobj->SetNoRender(true);
		}
		*/
		// TEMP: show collision
		/*
		if (obj.no_collision)
		{
			tobj->SetNoRender(true);
		} else {
			tobj->SetNoRender(false);
		}
		*/

		if(s3d_version >= 11)
		{
			tobj->light_object = obj.light_object;
		}
		else
			tobj->light_object = false;

		// Set mesh's material
		if (obj.material_index>=0) 
			tmesh->UseMaterial(mathandles[obj.material_index]);
		
		// Allocate memory for vertex data
		Storm3D_Vertex *tempverts=new Storm3D_Vertex[obj.vertex_amount];

		// Allocate memory for bone weights
		if(has_weights)
			tmesh->bone_weights = new Storm3D_Weight[obj.vertex_amount];

		int vertexSize = (2 * 3 * sizeof(float)) + (2 * 2 * sizeof(float));
		std::vector<char> vbuffer(originalVertexAmount * vertexSize);
		filesystem::fb_fread(&vbuffer[0], sizeof(char), vbuffer.size(), f);

		// Read vertex data
		for (int i2=0;i2<originalVertexAmount;i2++)
		{
			S3D_VERTEX &vertex = *((S3D_VERTEX *) (&vbuffer[i2 * vertexSize]));

			/*
			// Read data
			S3D_VERTEX vertex = { 0 };

			if (s3d_version >= 6)
			{
				fread(vertex.position, sizeof(float), 3, f);
				fread(vertex.normal, sizeof(float), 3, f);
				fread(vertex.texturecoords, sizeof(float), 2, f);

				if(s3d_version >= 9)
					fread(vertex.texturecoords2, sizeof(float), 2, f);
				else
				{
					vertex.texturecoords2[0] = vertex.texturecoords[0];
					vertex.texturecoords2[1] = vertex.texturecoords[1];
				}
			}

			// Not used
			if(s3d_version == 6)
			{
				signed char c = 0;
				fread(&c, sizeof(signed char), 1, f);
			}
			*/

			// the haxored hard vertex rotation...
			// -- jpk
			if (hard_rotation != 0)
			{
				float origp0 = vertex.position[0];
				float orign0 = vertex.normal[0];
				vertex.position[0] = vertex.position[0] * cosf((float)hard_rotation * 3.1415927f / 180) + vertex.position[2] * sinf((float)hard_rotation * 3.1415927f / 180);
				vertex.position[2] = vertex.position[2] * cosf((float)hard_rotation * 3.1415927f / 180) - origp0 * sinf((float)hard_rotation * 3.1415927f / 180);
				vertex.normal[0] = vertex.normal[0] * cosf((float)hard_rotation * 3.1415927f / 180) + vertex.normal[2] * sinf((float)hard_rotation * 3.1415927f / 180);
				vertex.normal[2] = vertex.normal[2] * cosf((float)hard_rotation * 3.1415927f / 180) - orign0 * sinf((float)hard_rotation * 3.1415927f / 180);
			}
			if (mirrored)
			{
				if (mirroredX)
				{
					vertex.position[0] = -vertex.position[0];
					vertex.normal[0] = -vertex.normal[0];
				}
				if (mirroredY)
				{
					vertex.position[1] = -vertex.position[1];
					vertex.normal[1] = -vertex.normal[1];
				}
				if (mirroredZ)
				{
					vertex.position[2] = -vertex.position[2];
					vertex.normal[2] = -vertex.normal[2];
				}
				if (mirroredU)
				{
					vertex.texturecoords[0] = 1.0f - vertex.texturecoords[0];
					vertex.texturecoords2[0] = 1.0f - vertex.texturecoords2[0];
				}
				if (mirroredV)
				{
					vertex.texturecoords[1] = 1.0f - vertex.texturecoords[1];
					vertex.texturecoords2[1] = 1.0f - vertex.texturecoords2[1];
				}
			}

#ifdef PROJECT_AOV
// fix y coords being generally negative (0.0 - -1.0)
vertex.texturecoords[1] += 1.0f;
// and use the second set of texture coordinates for black edge purposes instead of lightmap
// NOTE: this breaks lightmaps!
vertex.texturecoords2[1] = 1.0f;
#endif

			if (scaled)
			{
				if (scaledX)
					vertex.position[0] *= scaledX;
				if (scaledY)
					vertex.position[1] *= scaledY;
				if (scaledZ)
					vertex.position[2] *= scaledZ;
			}
			if (sideways)
			{
				float origp1 = vertex.position[1];
				float orign1 = vertex.normal[1];
				vertex.position[1] = -vertex.position[2];
				vertex.position[2] = origp1;
				vertex.normal[1] = -vertex.normal[2];
				vertex.normal[2] = orign1;
			}

			tempverts[i2]=Storm3D_Vertex(VC3(vertex.position),VC3(vertex.normal),VC2(vertex.texturecoords),VC2(vertex.texturecoords2));

			//if (fatboy)
			//{
				// old fatboy move, does not handle seams...
				//tempverts[i2].position += tempverts[i2].normal * fatboySize;
			//}

			if (blackEdgeThisObject)
			{
				tempverts[i2 + originalVertexAmount] = tempverts[i2];
				tempverts[i2 + originalVertexAmount].texturecoordinates = VC2(blackEdgeU,blackEdgeV);
				//tempverts[i2 + originalVertexAmount].texturecoordinates2 = VC2(blackEdgeU,blackEdgeV);
				tempverts[i2 + originalVertexAmount].texturecoordinates2 = VC2(0,0);
				tempverts[i2 + originalVertexAmount].normal = -tempverts[i2].normal;
				// old blackedge move, does not handle seams...
				//tempverts[i2 + originalVertexAmount].position += tempverts[i2].normal * blackEdgeSize;
			}
		}

		// New seamless fatboy/blackedge code...
		// NOTE: there may be some errors in the result (for example in a box) depending on how each
		// affecting side is tripled. (how many vertices affect the normal per box side, 1 or 2)
		if (blackEdge || fatboy)
		{
			VC3 *vxposcopy = NULL;
			if (fatboy)
			{
				vxposcopy = new VC3[originalVertexAmount];
				for (int i2=0;i2<originalVertexAmount;i2++)
				{
					vxposcopy[i2] = tempverts[i2].position;
				}
			}
			for (int i2=0;i2<originalVertexAmount;i2++)
			{
				int seamAmount = 0;
				VC3 seamnorm(0,0,0);
				for (int seami = 0; seami < originalVertexAmount; seami++)
				{
					VC3 otherpos = tempverts[seami].position;
					if (fatboy)
						otherpos = vxposcopy[seami];
					VC3 diff = otherpos - tempverts[i2].position;
					if (diff.GetSquareLength() < 0.001f * 0.001f)
					{
						seamnorm += tempverts[seami].normal;
						seamAmount++;
					}
				}
				if (seamAmount == 0)
				{
					assert(!"blackEdge/fatboy - 0 vertices match, this should never happen.");
				}
				if (seamAmount >= 2)
				{
					seamnorm /= (float)seamAmount;
					if (seamnorm.GetSquareLength() > 0.0f)
						seamnorm.Normalize();
					if (fatboy)
					{
						tempverts[i2].position += seamnorm * fatboySize;
					}
					if (blackEdgeThisObject)
					{
						tempverts[i2 + originalVertexAmount].position = tempverts[i2].position + seamnorm * blackEdgeSize;
					}
				} else {
					if (fatboy)
					{
						tempverts[i2].position += tempverts[i2].normal * fatboySize;
					}
					if (blackEdgeThisObject)
					{
						tempverts[i2 + originalVertexAmount].position = tempverts[i2].position + tempverts[i2].normal * blackEdgeSize;
					}
				}
			}
			if (fatboy)
			{
				delete [] vxposcopy;
			}
		}


		// Allocate memory for face data
		Storm3D_Face *tempfaces=new Storm3D_Face[obj.face_amount];

		int faceSize = (3 * sizeof(unsigned short));
		std::vector<char> fbuffer(originalFaceAmount * faceSize);
		filesystem::fb_fread(&fbuffer[0], sizeof(char), fbuffer.size(), f);

		int mirrorCount = 0;
		if (mirrored)
		{
			if (mirroredX) mirrorCount++;
			if (mirroredY) mirrorCount++;
			if (mirroredZ) mirrorCount++;
		}

		// Read face data
		for(int i2=0;i2<originalFaceAmount;i2++)
		{
			S3D_FACE &face = *((S3D_FACE *) (&fbuffer[i2 * faceSize]));

			/*
			// Read data
			S3D_FACE face;
			fread(&face,sizeof(S3D_FACE),1,f);
			*/
		
			for(int i = 0; i < 3; ++i)
				assert(face.vertex[i] >= 0 && face.vertex[i] < obj.vertex_amount);

			tempfaces[i2]=Storm3D_Face(face.vertex[0],face.vertex[1],face.vertex[2],VC3(0,0,0));

			if (mirrorCount & 1)
			{
				tempfaces[i2] = tempfaces[i2];
				unsigned int orig0 = tempfaces[i2].vertex_index[0];
				tempfaces[i2].vertex_index[0] = tempfaces[i2].vertex_index[2];
				//tempfaces[i2].vertex_index[1] = tempfaces[i2].vertex_index[1];
				tempfaces[i2].vertex_index[2] = orig0;
			}
			if (blackEdgeThisObject)
			{
				tempfaces[i2 + originalFaceAmount] = tempfaces[i2];
				tempfaces[i2 + originalFaceAmount].vertex_index[0] = tempfaces[i2].vertex_index[2] + originalVertexAmount;
				tempfaces[i2 + originalFaceAmount].vertex_index[1] = tempfaces[i2].vertex_index[1] + originalVertexAmount;
				tempfaces[i2 + originalFaceAmount].vertex_index[2] = tempfaces[i2].vertex_index[0] + originalVertexAmount;
			}
		}

		// Set vertices/faces
		tmesh->ChangeFaceCount(obj.face_amount);
		memcpy(tmesh->GetFaceBuffer(),tempfaces,sizeof(Storm3D_Face)*obj.face_amount);

		tmesh->ChangeVertexCount(obj.vertex_amount);
		memcpy(tmesh->GetVertexBuffer(),tempverts,sizeof(Storm3D_Vertex)*obj.vertex_amount);

		tmesh->hasLods = hasLods;
		if(hasLods)
		{
			// TODO: lods don't have proper support for blackEdge

			for(int j = 1; j < IStorm3D_Mesh::LOD_AMOUNT; ++j)
			{
				WORD faceAmount = 0;
				filesystem::fb_fread(&faceAmount, sizeof(WORD), 1, f);

				Storm3D_Face *buffer = new Storm3D_Face[faceAmount];
				for(int i = 0; i < faceAmount; ++i)
				{
					S3D_FACE face;
					filesystem::fb_fread(&face, sizeof(S3D_FACE), 1, f);

					buffer[i] = Storm3D_Face(face.vertex[0], face.vertex[1], face.vertex[2], VC3(0,0,0));
				}

				delete[] tmesh->faces[j];

				tmesh->face_amount[j] = faceAmount;
				tmesh->faces[j] = buffer;
			}
		}

		// Free memory
		delete[] tempfaces;

		// Set object's properties
		tobj->SetPosition(VC3(obj.position));
		tobj->SetRotation(QUAT(obj.rotation));
		tobj->SetScale(VC3(obj.scale));
		tobj->SetMesh(tmesh);

		// Free memory
		delete[] tempverts;

		/* Bone weights */
		if(has_weights)
		{
			for(int j = 0; j < originalVertexAmount; j++)
			{
				int bone1;
				int bone2;
				signed char weight1;
				signed char weight2;

				filesystem::fb_fread(&bone1, sizeof(int), 1, f);
				filesystem::fb_fread(&bone2, sizeof(int), 1, f);
				filesystem::fb_fread(&weight1, sizeof(signed char), 1, f);
				filesystem::fb_fread(&weight2, sizeof(signed char), 1, f);

				// bone1 = bone2 = 0;
				//weight1 = weight2 = 0;

				tmesh->bone_weights[j].index1 = bone1;
				tmesh->bone_weights[j].index2 = bone2;
				tmesh->bone_weights[j].weight1 = weight1;
				//tmesh->bone_weights[j].weight2 = 100 - weight1;
				tmesh->bone_weights[j].weight2 = weight2;

				if (blackEdgeThisObject)
				{
					tmesh->bone_weights[j + originalVertexAmount] = tmesh->bone_weights[j];
				}

				//assert(weight1 + weight2 == 100);
			}
		}

		if (blackEdgeThisObject)
		{
			tmesh->SetCollisionFaces(obj.face_amount / 2);
		}

		// Rebuild
		// psd: don't rebuild
		tmesh->ReBuild();

		// Read animation keyframes
		//tobj->Animation_SetLoop(obj.keyframe_endtime);
		for (int i2=0;i2<obj.poskey_amount;i2++)	// Position
		{
			// Read keyframe from disk
			S3D_V3KEY temp_kf;
			filesystem::fb_fread(&temp_kf,sizeof(S3D_V3KEY),1,f);
		}

		for (int i2=0;i2<obj.rotkey_amount;i2++)	// QUAT
		{
			// Read keyframe from disk
			S3D_ROTKEY temp_kf;
			filesystem::fb_fread(&temp_kf,sizeof(S3D_ROTKEY),1,f);
		}

		for (int i2=0;i2<obj.scalekey_amount;i2++)	// VC3
		{
			// Read keyframe from disk
			S3D_V3KEY temp_kf;
			filesystem::fb_fread(&temp_kf,sizeof(S3D_V3KEY),1,f);
		}

		for (int i2=0;i2<obj.meshkey_amount;i2++)	// Mesh
		{
			// Read keyframe from disk
			S3D_MESHKEY temp_kf;
			filesystem::fb_fread(&temp_kf,sizeof(S3D_MESHKEY),1,f);
			
			// Temp vertexes
			Storm3D_Vertex *temp_vxs=new Storm3D_Vertex[tmesh->vertex_amount];

			// Read vertexes from disk
			for (int vn=0;vn<tmesh->vertex_amount;vn++)
			{
				S3D_VERTEX vertex;
				filesystem::fb_fread(&vertex,sizeof(S3D_VERTEX),1,f);

				// NOTE: animation keyframes do not support blackEdge

				// the haxored hard vertex rotation...
				// -- jpk
				if (hard_rotation != 0)
				{
					float origp0 = vertex.position[0];
					float orign0 = vertex.normal[0];
					vertex.position[0] = vertex.position[0] * cosf((float)hard_rotation * 3.1415927f / 180) + vertex.position[2] * sinf((float)hard_rotation * 3.1415927f / 180);
					vertex.position[2] = vertex.position[2] * cosf((float)hard_rotation * 3.1415927f / 180) - origp0 * sinf((float)hard_rotation * 3.1415927f / 180);
					vertex.normal[0] = vertex.normal[0] * cosf((float)hard_rotation * 3.1415927f / 180) + vertex.normal[2] * sinf((float)hard_rotation * 3.1415927f / 180);
					vertex.normal[2] = vertex.normal[2] * cosf((float)hard_rotation * 3.1415927f / 180) - orign0 * sinf((float)hard_rotation * 3.1415927f / 180);
				}
				if (mirrored)
				{
					if (mirroredX)
					{
						vertex.position[0] = -vertex.position[0];
						vertex.normal[0] = -vertex.normal[0];
					}
					if (mirroredY)
					{
						vertex.position[1] = -vertex.position[1];
						vertex.normal[1] = -vertex.normal[1];
					}
					if (mirroredZ)
					{
						vertex.position[2] = -vertex.position[2];
						vertex.normal[2] = -vertex.normal[2];
					}
					if (mirroredU)
					{
						vertex.texturecoords[0] = 1.0f - vertex.texturecoords[0];
						vertex.texturecoords2[0] = 1.0f - vertex.texturecoords2[0];
					}
					if (mirroredV)
					{
						vertex.texturecoords[1] = 1.0f - vertex.texturecoords[1];
						vertex.texturecoords2[1] = 1.0f - vertex.texturecoords2[1];
					}
				}
				if (scaled)
				{
					if (scaledX)
						vertex.position[0] *= scaledX;
					if (scaledY)
						vertex.position[1] *= scaledY;
					if (scaledZ)
						vertex.position[2] *= scaledZ;
				}
				if (sideways)
				{
					float origp1 = vertex.position[1];
					float orign1 = vertex.normal[1];
					vertex.position[1] = -vertex.position[2];
					vertex.position[2] = origp1;
					vertex.normal[1] = -vertex.normal[2];
					vertex.normal[2] = orign1;
				}

				temp_vxs[vn]=Storm3D_Vertex(VC3(vertex.position), VC3(vertex.normal),VC2(vertex.texturecoords));
			}

			// Add it to object
			//tmesh->Animation_AddNewMeshKeyFrame(temp_kf.keytime,temp_vxs);

			// Release temp vertexes
			delete[] temp_vxs;
		}

		// Create object's collision table (if has no bones and no_collision not set)
		if(bones.empty() && obj.no_collision == 0)
		{
			tmesh->collision.ReBuild(tmesh);
			tmesh->col_rebuild_needed=false;
		}

		// psd!
		updateRadiusToContain(tobj->position, tmesh->GetRadius());
	}

	//if(s3d_version >= 11)
	{
		bool hasLightObjects = false;

		std::set<IStorm3D_Model_Object *>::iterator it = objects.begin();
		for(; it != objects.end(); ++it)
		{
			Storm3D_Model_Object *o = static_cast<Storm3D_Model_Object *> (*it);
			if(o->light_object)
				hasLightObjects = true;
		}

		it = objects.begin();
		for(; it != objects.end(); ++it)
		{
			Storm3D_Model_Object *o = static_cast<Storm3D_Model_Object *> (*it);

			if(hasLightObjects)
			{
				if(!o->light_object)
					light_objects.erase(o);
			}
		}

		std::string foo;
		foo += std::string("Objects: ") + boost::lexical_cast<std::string> (objects.size());
		foo += "\r\n";
		foo += std::string("Light objects: ") + boost::lexical_cast<std::string> (light_objects.size());
		foo += "\r\n";
		//MessageBox(0, foo.c_str(), "Info", MB_OK);
	}

	// Read lights
	for(int i=0;i<header.num_lights;i++)
	{
		// Read data
		S3D_LIGHT lgt;

		if(s3d_version >= 6)
		{
			lgt.name = LoadStringFromFile(f);
			lgt.parent = LoadStringFromFile(f);

			filesystem::fb_fread(&lgt.light_type, sizeof(int), 1, f);
			filesystem::fb_fread(&lgt.lensflare_index, sizeof(int), 1, f);
			filesystem::fb_fread(lgt.color, sizeof(float), 3, f);
			filesystem::fb_fread(lgt.position, sizeof(float), 3, f);
			filesystem::fb_fread(lgt.direction, sizeof(float), 3, f);
			filesystem::fb_fread(&lgt.cone_inner, sizeof(float), 1, f);
			filesystem::fb_fread(&lgt.cone_outer, sizeof(float), 1, f);
			filesystem::fb_fread(&lgt.multiplier, sizeof(float), 1, f);
			filesystem::fb_fread(&lgt.decay, sizeof(float), 1, f);
			filesystem::fb_fread(&lgt.keyframe_endtime, sizeof(int), 1, f);
			filesystem::fb_fread(&lgt.poskey_amount, sizeof(WORD), 1, f);
			filesystem::fb_fread(&lgt.dirkey_amount, sizeof(WORD), 1, f);
			filesystem::fb_fread(&lgt.lumkey_amount, sizeof(WORD), 1, f);
			filesystem::fb_fread(&lgt.conekey_amount, sizeof(WORD), 1, f);
		}
	}

	// Read helpers
	for(int i=0;i<header.num_helpers;i++)
	{
		// Read data
		S3D_HELPER help;

		if(s3d_version >= 6)
		{
			help.name = LoadStringFromFile(f);
			help.parent = LoadStringFromFile(f);

			filesystem::fb_fread(&help.helper_type, sizeof(int), 1, f);
			filesystem::fb_fread(help.position, sizeof(float), 3, f);
			filesystem::fb_fread(help.other, sizeof(float), 3, f);
			filesystem::fb_fread(help.other2, sizeof(float), 3, f);
			filesystem::fb_fread(&help.keyframe_endtime, sizeof(int), 1, f);
			filesystem::fb_fread(&help.poskey_amount, sizeof(WORD), 1, f);
			filesystem::fb_fread(&help.o1key_amount, sizeof(WORD), 1, f);
			filesystem::fb_fread(&help.o2key_amount, sizeof(WORD), 1, f);

			// the haxored hard vertex rotation...
			// -- jpk
			if (hard_rotation != 0)
			{
				float origp0 = help.position[0];
				help.position[0] = help.position[0] * cosf((float)hard_rotation * 3.1415927f / 180) + help.position[2] * sinf((float)hard_rotation * 3.1415927f / 180);
				help.position[2] = help.position[2] * cosf((float)hard_rotation * 3.1415927f / 180) - origp0 * sinf((float)hard_rotation * 3.1415927f / 180);
			}
			if (mirrored)
			{
				if (mirroredX)
				{
					help.position[0] = -help.position[0];
				}
				if (mirroredY)
				{
					help.position[1] = -help.position[1];
				}
				if (mirroredZ)
				{
					help.position[2] = -help.position[2];
				}
			}
			if (scaled)
			{
				if (scaledX)
					help.position[0] *= scaledX;
				if (scaledY)
					help.position[1] *= scaledY;
				if (scaledZ)
					help.position[2] *= scaledZ;
			}
			if (sideways)
			{
				float origp1 = help.position[1];
				help.position[1] = -help.position[2];
				help.position[2] = origp1;
			}
		}

		// Create helper
		IStorm3D_Helper *thelp=NULL;
		if (help.helper_type==IStorm3D_Helper::HTYPE_POINT)
		{
			thelp=Helper_Point_New(help.name.c_str(),VC3(help.position));
		}
		else if (help.helper_type==IStorm3D_Helper::HTYPE_VECTOR)
		{
			thelp=Helper_Vector_New(help.name.c_str(),VC3(help.position),VC3(help.other));
		}
		else if (help.helper_type==IStorm3D_Helper::HTYPE_CAMERA)
		{
			thelp=Helper_Camera_New(help.name.c_str(),VC3(help.position),VC3(help.other),VC3(help.other2));
		}
		else if (help.helper_type==IStorm3D_Helper::HTYPE_BOX)
		{
			thelp=Helper_Box_New(help.name.c_str(),VC3(help.position),VC3(help.other));
		}
		else if (help.helper_type==IStorm3D_Helper::HTYPE_SPHERE)
		{
			thelp=Helper_Sphere_New(help.name.c_str(),VC3(help.position),help.other[0]);
			/*char s[100];
			sprintf(s,"%s (%f,%f,%f) RAD:%f",help.name,help.position[0],help.position[1],help.position[2],help.other[0]);
			MessageBox(NULL,s,"SPH_DEB",0);*/
		}

		// Set animation loop
		thelp->Animation_SetLoop(help.keyframe_endtime);

		// Load animation keyframes
		for (int i2=0;i2<help.poskey_amount;i2++)	// Position
		{
			// Read keyframe from disk
			S3D_V3KEY temp_kf;
			filesystem::fb_fread(&temp_kf,sizeof(S3D_V3KEY),1,f);
			
			// Add it to helper
			((IStorm3D_Helper_Point*)thelp)->Animation_AddNewPositionKeyFrame(
				temp_kf.keytime,VC3(temp_kf.x,temp_kf.y,temp_kf.z));
		}

		// Not for point helpers
		if (thelp->GetHelperType()!=IStorm3D_Helper::HTYPE_POINT)
		for (int i2=0;i2<help.o1key_amount;i2++)	// Other1
		{
			// Read keyframe from disk
			S3D_V3KEY temp_kf;
			filesystem::fb_fread(&temp_kf,sizeof(S3D_V3KEY),1,f);
			
			// Add it to helper
			if (thelp->GetHelperType()==IStorm3D_Helper::HTYPE_VECTOR)
			{
				((IStorm3D_Helper_Vector*)thelp)->Animation_AddNewDirectionKeyFrame(temp_kf.keytime,VC3(temp_kf.x,temp_kf.y,temp_kf.z));
			}
			else if (thelp->GetHelperType()==IStorm3D_Helper::HTYPE_BOX)
			{
				((IStorm3D_Helper_Box*)thelp)->Animation_AddNewSizeKeyFrame(temp_kf.keytime,VC3(temp_kf.x,temp_kf.y,temp_kf.z));
			}
			else if (thelp->GetHelperType()==IStorm3D_Helper::HTYPE_CAMERA)
			{
				((IStorm3D_Helper_Camera*)thelp)->Animation_AddNewDirectionKeyFrame(temp_kf.keytime,VC3(temp_kf.x,temp_kf.y,temp_kf.z));
			}
			else if (thelp->GetHelperType()==IStorm3D_Helper::HTYPE_SPHERE)
			{
				((IStorm3D_Helper_Sphere*)thelp)->Animation_AddNewRadiusKeyFrame(temp_kf.keytime,temp_kf.x);
				//MessageBox(NULL,"RADIUS KEYFRAME","DEBUG",0);
			}
		}

		// Only for camera helpers
		if (thelp->GetHelperType()==IStorm3D_Helper::HTYPE_CAMERA)
		for (int i2=0;i2<help.o2key_amount;i2++)	// Other2
		{
			// Read keyframe from disk
			S3D_V3KEY temp_kf;
			filesystem::fb_fread(&temp_kf,sizeof(S3D_V3KEY),1,f);
			
			// Add it to helper
			((IStorm3D_Helper_Camera*)thelp)->Animation_AddNewUpVectorKeyFrame(temp_kf.keytime,VC3(temp_kf.x,temp_kf.y,temp_kf.z));
		}

		// Set parent (if has)
		if (strlen(help.parent.c_str())>0)
		{
			IStorm3D_Model_Object *opar = SearchObject(help.parent.c_str());
			if (opar) 
				opar->AddChild(thelp);
			else
			{
				// Bone parent?
			}
		}
	}

	// ToDo: set bone parents
/*
	// Read bones (version 7+)
	if(s3d_version >= 7)
	for (i=0;i<header.num_bones;i++)
	{
		Storm3D_Bone *bone = new Storm3D_Bone();
		S3D_BONE s3d_bone;
		
		// Get actual properties
		s3d_bone.name = LoadStringFromFile(f);
		fread(&s3d_bone.position, sizeof(float), 3, f);
		fread(&s3d_bone.rotation, sizeof(float), 4, f);
		fread(&s3d_bone.original_position, sizeof(float), 3, f);
		fread(&s3d_bone.original_rotation, sizeof(float), 4, f);
		fread(&s3d_bone.max_angles, sizeof(float), 3, f);
		fread(&s3d_bone.max_angles, sizeof(float), 3, f);
		fread(&s3d_bone.parent_index, sizeof(int), 1, f);

		bone->SetName(s3d_bone.name);
		bone->SetOriginalProperties(s3d_bone.position, s3d_bone.rotation, s3d_bone.original_position, s3d_bone.original_rotation);
		bone->SetParentIndex(s3d_bone.parent_index);

		delete[] s3d_bone.name;
		bones.push_back(bone);
	}
*/
	// Close file
	filesystem::fb_fclose(f);

	storm3d_model_loads++;
	updateEffectTexture();

	if (disableShadows)
	{
		this->cast_shadows = false;
	}

	return true;
}


//------------------------------------------------------------------
// Storm3D_Model::LoadBones
//------------------------------------------------------------------
bool Storm3D_Model::LoadBones(const char *filename)
{
	// First empty model (delete everything)
//	Empty(true, false);
	// 
	// foo.b3d@bar.b3d,wing_lower_l,wing_higher_l,wing_lower_l,wing_higher_l
	// --jpk
	bool combine = false;
	bool removeBones = false;
	std::string combineWithFilename;
	std::vector<std::string> combineBoneNames;
	std::vector<std::string> removeBoneNames;

	char actual_filename[512+1];
	int filename_len = strlen(filename);
	if (filename_len < 512) 
	{
		strcpy(actual_filename, filename);
		for (int i = 0; i < filename_len; i++)
		{
			if (actual_filename[i] == '@')
			{
				int restoreAtPos = -1;
				for (int j = i+1; j < filename_len; j++)
				{
					if (actual_filename[j] == '@')
					{
						actual_filename[j] = '\0';
						restoreAtPos = j;
						break;
					}
				}
				actual_filename[i] = '\0';

				if (actual_filename[i + 1] == '-')
				{
					removeBones = true;
					removeBoneNames.push_back(&actual_filename[i + 2]);
				} else {
					combine = true;

					bool gotFirstOne = false;
					int lastCommaPos = i;
					for (int j = i + 1; j < filename_len + 1; j++)
					{
						if (actual_filename[j] == ','
							|| actual_filename[j] == '\0')
						{
							// interpret first token as filename, rest as bone names
							if (!gotFirstOne)
							{
								// file name
								assert(j >= (lastCommaPos + 1));
								combineWithFilename = std::string(&actual_filename[lastCommaPos + 1]).substr(0, j - (lastCommaPos + 1));
								if (combineWithFilename.empty())
								{
									if (Storm3D2->getLogger() != NULL)
									{
										Storm3D2->getLogger()->warning("Storm3D_Model::LoadBones - Combine filename empty.");
										Storm3D2->getLogger()->debug(filename);
									}
								}
								lastCommaPos = j;
								gotFirstOne = true;
							} else {
								// bone name
								assert(j >= (lastCommaPos + 1));
								std::string tmp = std::string(&actual_filename[lastCommaPos + 1]).substr(0, j - (lastCommaPos + 1));
								if (tmp.empty())
								{
									if (Storm3D2->getLogger() != NULL)
									{
										Storm3D2->getLogger()->warning("Storm3D_Model::LoadBones - Combine bone name empty.");
										Storm3D2->getLogger()->debug(filename);
									}
								} else {
									combineBoneNames.push_back(tmp);
								}
								lastCommaPos = j;							
							}
							if (actual_filename[j] == '\0')
								break;
						}
					}
				}
				if (restoreAtPos != -1)
				{
					actual_filename[restoreAtPos] = '@';
				}
			}	
		}
	} else {
		assert(0);
		return false;
	}

	if (combine)
	{
		if (Storm3D2->getLogger() != NULL)
		{
			Storm3D2->getLogger()->debug((std::string("Storm3D_Model::LoadBones - Combining with model: ") + combineWithFilename).c_str());
			for (int i = 0; i < (int)combineBoneNames.size(); i++)
			{
				Storm3D2->getLogger()->debug((std::string("bone: ") + combineBoneNames[i]).c_str());
			}
		}
		assert(!"Storm3D_Model::LoadBones - combine TODO");
	}
		
	if (removeBones)
	{
		if (Storm3D2->getLogger() != NULL)
		{
			Storm3D2->getLogger()->debug("Storm3D_Model::LoadBones - Removing bones:");
			for (int i = 0; i < (int)removeBoneNames.size(); i++)
			{
				Storm3D2->getLogger()->debug((std::string("bone: ") + removeBoneNames[i]).c_str());
			}
		}
	}
		
	if (Storm3D2->getLogger() != NULL)
	{
		Storm3D2->getLogger()->debug("Storm3D_Model::LoadBones - Loading a bones file.");
		Storm3D2->getLogger()->debug(actual_filename);
	}

	// Open file
	filesystem::FB_FILE *fp = filesystem::fb_fopen(actual_filename, "rb");
	if(fp == 0)
		return false;

	char header[5] = { 0 };
	filesystem::fb_fread(header, sizeof(char), 5, fp);
	if((memcmp(header,"B3D10", 5) != 0) && (memcmp(header,"B3D11", 5) != 0))
	{
		filesystem::fb_fclose(fp);
		return false;
	}

	filesystem::fb_fread(&bone_boneid, sizeof(int), 1, fp); // id
	int bone_count = 0;
	filesystem::fb_fread(&bone_count, sizeof(int), 1, fp); // amount

	// If not compatible with models bones
	if((model_boneid != 0) && (model_boneid != bone_boneid))
	{
		filesystem::fb_fclose(fp);
		return false;
	}

	if((memcmp(header,"B3D11", 5) == 0))
		use_cylinder_collision = true;

	for(int i = 0; i < bone_count; ++i)
	{
		// Get actual properties
		std::string name = LoadStringFromFile(fp);

		bool skipThis = false;
		if (removeBones)
		{
			for (int j = 0; j < (int)removeBoneNames.size(); j++)
			{
				if (name == removeBoneNames[j])
				{
					skipThis = true;
					break;
				}
			}
		}

		if (skipThis)
		{
			// in reality, we cannot just skip bones if we don't remap the bone IDs
			// thus, we'll instead rather create a silly "dummy bone"
			//continue;
		}

		Storm3D_Bone *bone = new Storm3D_Bone();		

		Vector position;
		Rotation rotation;
		
		Rotation original_rotation;
		Vector original_position;

		Vector max_angles;
		Vector min_angles;

		float length = 0;
		float thickness = 0;

		int parent_index = -1;

		filesystem::fb_fread(&position, sizeof(float), 3, fp);
		filesystem::fb_fread(&rotation, sizeof(float), 4, fp);
		filesystem::fb_fread(&original_position, sizeof(float), 3, fp);
		filesystem::fb_fread(&original_rotation, sizeof(float), 4, fp);
		filesystem::fb_fread(&max_angles, sizeof(float), 3, fp);
		filesystem::fb_fread(&max_angles, sizeof(float), 3, fp);
		filesystem::fb_fread(&length, sizeof(float), 1, fp);
		filesystem::fb_fread(&thickness, sizeof(float), 1, fp);
		filesystem::fb_fread(&parent_index, sizeof(int), 1, fp);

//length += 2.f * thickness;

		bone->SetName(name.c_str());
		bone->SetOriginalProperties(position, rotation, original_position, original_rotation);
		if (skipThis)
		{
			bone->SetSpecialProperties(0.01f, 0.01f);
			bone->SetNoCollision(true);
		} else {
			bone->SetSpecialProperties(length, thickness);
		}
		bone->SetParentIndex(parent_index);

		bones.push_back(bone);
	}

	// Build weight buffers
	for(std::set<IStorm3D_Model_Object *>::iterator io = objects.begin(); io != objects.end(); ++io)
	{
		Storm3D_Model_Object *object = static_cast<Storm3D_Model_Object*>(*io);
		Storm3D_Mesh *mesh = static_cast<Storm3D_Mesh *> (object->GetMesh());

		if(mesh)
		{
			mesh->ReBuild();

			// !!!
			if(!terrain_object)
			{
				//mesh->col_rebuild_needed = false;
				//mesh->collision.reset();
			}
		}
	}

	int bone_helper_amount = 0;
	filesystem::fb_fread(&bone_helper_amount, sizeof(int), 1, fp);

	// No helpers?
	if(filesystem::fb_feof(fp) != 0)
	{
		filesystem::fb_fclose(fp);
		return true;
	}

	for(int i = 0; i < bone_helper_amount; ++i)
	{
		std::string name = LoadStringFromFile(fp);
		std::string parent = LoadStringFromFile(fp);

		int helper_type = 0;
		filesystem::fb_fread(&helper_type, sizeof(int), 1, fp);
		Vector position, other, other2;

		filesystem::fb_fread(&position, sizeof(float), 3, fp);
		filesystem::fb_fread(&other, sizeof(float), 3, fp);
		filesystem::fb_fread(&other2, sizeof(float), 3, fp);

		int end_time;
		WORD foo;

		filesystem::fb_fread(&end_time, sizeof(int), 1, fp);
		filesystem::fb_fread(&foo, sizeof(WORD), 1 ,fp);
		filesystem::fb_fread(&foo, sizeof(WORD), 1 ,fp);
		filesystem::fb_fread(&foo, sizeof(WORD), 1 ,fp);

		IStorm3D_Helper *helper = 0;
		if(helper_type == IStorm3D_Helper::HTYPE_CAMERA)
			helper = Helper_Camera_New(name.c_str(), position, other, other2);

		if(helper == 0)
			continue;

		// Search bone parent
		for(unsigned int j = 0; j < bones.size(); ++j)
		if(strcmp(bones[j]->GetName(), parent.c_str()) == 0)
		{
			bones[j]->AddChild(helper);
			break;
		}

		helpers.insert(helper);
	}

	filesystem::fb_fclose(fp);
	storm3d_model_bone_loads++;

	return true;
}

//------------------------------------------------------------------
// Storm3D_Model::Empty
//------------------------------------------------------------------
void Storm3D_Model::Empty(bool leave_geometry, bool leave_bones)
{
	if(leave_geometry == false)
	{
		// Delete objects
		for(set<IStorm3D_Model_Object*>::reverse_iterator io=objects.rbegin();io!=objects.rend();io++)
		{
			delete (*io);
			storm3d_model_objects_created--;
		}

		// Delete helpers
		for(set<IStorm3D_Helper*>::iterator ih=helpers.begin();ih!=helpers.end();ih++)
		{
			delete (*ih);
		}

		// Clear sets
		objects.clear();
		collision_objects.clear();
		objects_array.clear();
		helpers.clear();
		model_boneid = 0;
	}

	if(leave_bones == false)
	{
		// Delete animations
		normal_animations.clear();
		blending_with_animations.clear();
		manual_blendings.clear();

		// Delete bones
		for(unsigned int ib = 0; ib < bones.size(); ++ib)
			delete bones[ib];

		bones.clear();
		bone_boneid = 0;

		normal_animations.clear();
		blending_with_animations.clear();
		manual_blendings.clear();
	}
}



IStorm3D_Model *Storm3D_Model::GetClone(bool cloneGeometry, bool cloneHelpers, bool cloneBones) const
{
	Storm3D_Model *clone = static_cast<Storm3D_Model *> (Storm3D2->CreateNewModel());

	// Loses hierarchy information. It's relevant for helpers?

	if(cloneGeometry)
	{
		set<IStorm3D_Model_Object *>::const_iterator it = objects.begin();
		for(; it != objects.end(); ++it)
		{
			Storm3D_Model_Object *object = static_cast<Storm3D_Model_Object *> (*it);
			Storm3D_Model_Object *newObject = static_cast<Storm3D_Model_Object *> (clone->Object_New(object->GetName()));

			if(object->IsLightObject())
				newObject->SetAsLightObject();

			newObject->SetNoCollision(object->GetNoCollision());
			newObject->SetNoRender(object->GetNoRender());
			newObject->SetPosition(object->GetPosition());
			newObject->SetRotation(object->GetRotation());

			newObject->renderPassMask = object->renderPassMask;
			for (int i = 0; i < RENDER_PASS_BITS_AMOUNT; i++)
			{
				newObject->renderPassOffset[i] = object->renderPassOffset[i];
				newObject->renderPassScale[i] = object->renderPassScale[i];
			}
			newObject->alphaTestPassConditional = object->alphaTestPassConditional;
			newObject->alphaTestValue = object->alphaTestValue;
			Storm3D_Mesh *mesh = static_cast<Storm3D_Mesh *> (object->GetMesh());
			if(!mesh)
				continue;

			newObject->SetMesh(mesh);
		}
	}

	if(cloneBones)
	{
		vector<Storm3D_Bone *>::const_iterator it = bones.begin();
		for(; it != bones.end(); ++it)
		{
			const Storm3D_Bone *bone = *it;
			Storm3D_Bone *newBone = new Storm3D_Bone(*bone);

			newBone->name = new char[strlen(bone->name) + 1];
			strcpy(newBone->name, bone->name);

			newBone->objects.clear();
			newBone->helpers.clear();

			clone->bones.push_back(newBone);
		}

		clone->model_boneid = model_boneid;
		clone->bone_boneid = bone_boneid;
		clone->use_cylinder_collision = use_cylinder_collision;

		// ToDo: Animations?
	}

	if(cloneHelpers)
	{
		set<IStorm3D_Helper *>::const_iterator it = helpers.begin();
		for(; it != helpers.end(); ++it)
		{
			IStorm3D_Helper *originalHelper = *it;
			IStorm3D_Helper::HTYPE type = originalHelper->GetHelperType();
			Storm3D_Helper_AInterface *base = 0;
			IStorm3D_Helper *newHelper = 0;

			if(type == IStorm3D_Helper::HTYPE_POINT)
			{
				Storm3D_Helper_Point *helper = static_cast<Storm3D_Helper_Point *> (originalHelper);
				Storm3D_Helper_Point *point = static_cast<Storm3D_Helper_Point *> (clone->Helper_Point_New(helper->GetName(), helper->position));
				base = point;
				newHelper = point;
			}
			else if(type == IStorm3D_Helper::HTYPE_VECTOR)
			{
				Storm3D_Helper_Vector *helper = static_cast<Storm3D_Helper_Vector *> (originalHelper);
				Storm3D_Helper_Vector *vector = static_cast<Storm3D_Helper_Vector *> (clone->Helper_Vector_New(helper->GetName(), helper->position, helper->direction));
				base = vector;
				newHelper = vector;
			}
			else if(type == IStorm3D_Helper::HTYPE_CAMERA)
			{
				Storm3D_Helper_Camera *helper = static_cast<Storm3D_Helper_Camera *> (originalHelper);
				Storm3D_Helper_Camera *camera = static_cast<Storm3D_Helper_Camera *> (clone->Helper_Camera_New(helper->name, helper->position, helper->direction, helper->upvec));
				base = camera;
				newHelper = camera;
			}

			// ToDo: Other types

			if(base && newHelper)
			{
				Storm3D_Bone *parentBone = static_cast<Storm3D_Bone *> (originalHelper->GetParentBone());
				if(parentBone)
				{
					const char *name = parentBone->GetName();

					vector<Storm3D_Bone *>::iterator it = clone->bones.begin();
					for(; it != clone->bones.end(); ++it)
					{
						if(strcmp((**it).name, name) == 0)
						{
							(**it).AddChild(newHelper);
							break;
						}
					}
				}
			}
		}
	}

	clone->self_illumination = self_illumination;
	clone->cast_shadows = cast_shadows;
	clone->bone_collision = bone_collision;
	clone->original_bounding_radius = original_bounding_radius;
	clone->bounding_radius = bounding_radius;
	clone->no_collision = no_collision;
	clone->updateEffectTexture();

	return clone;
}


//------------------------------------------------------------------
// Storm3D_Model::Object_GetFirst
//------------------------------------------------------------------
/*Storm3D_Model_Object* Storm3D_Model::Object_GetFirst()
{
	return *objects.begin();
}*/



//------------------------------------------------------------------
// Storm3D_Model::Object_New
//------------------------------------------------------------------
IStorm3D_Model_Object *Storm3D_Model::Object_New(const char *name)
{
	// Create new object and add it into the set
	Storm3D_Model_Object *obj=new Storm3D_Model_Object(Storm3D2,name,this, Storm3D2->getResourceManager());
	objects.insert(obj);
	objects_array.push_back(obj);
	light_objects.insert(obj);

	SetCollision(obj);
	storm3d_model_objects_created++;

	// Return objects pointer for user modifications
	return obj;
}

void Storm3D_Model::GetVolume(VC3 &min_, VC3 &max_)
{
	min_.x = 10000000000.f;
	min_.y = 10000000000.f;
	min_.z = 10000000000.f;
	max_.x = -10000000000.f;
	max_.y = -10000000000.f;
	max_.z = -10000000000.f;

	bool setValues = false;

	set<IStorm3D_Model_Object *>::iterator it = objects.begin();
	for(; it != objects.end(); ++it)
	{
		IStorm3D_Model_Object *o = *it;
		if(!o)
			continue;

		IStorm3D_Mesh *mesh = o->GetMesh();
		if(!mesh)
			continue;

		VC3 opos = o->GetPosition();
		QUAT orot = o->GetRotation();
		MAT opostm;
		MAT orottm;
		opostm.CreateTranslationMatrix(opos);
		orottm.CreateRotationMatrix(orot);
		MAT tm = orottm * opostm;

		int vertexCount = mesh->GetVertexCount();
		const Storm3D_Vertex *vertexBuffer = mesh->GetVertexBufferReadOnly();

		for(int i = 0; i < vertexCount; ++i)
		{
			setValues = true;
			VC3 pos = tm.GetTransformedVector(vertexBuffer[i].position);

			min_.x = min(min_.x, pos.x);
			min_.y = min(min_.y, pos.y);
			min_.z = min(min_.z, pos.z);
			max_.x = max(max_.x, pos.x);
			max_.y = max(max_.y, pos.y);
			max_.z = max(max_.z, pos.z);
		}
	}

	if(!setValues)
	{
		min_.x = min_.y = min_.z = 0.f;
		max_.x = max_.y = max_.z = 0.f;
	}
}

void Storm3D_Model::GetVolumeApproximation(VC3 &min_, VC3 &max_)
{
	min_.x = 10000000000.f;
	min_.y = 10000000000.f;
	min_.z = 10000000000.f;
	max_.x = -10000000000.f;
	max_.y = -10000000000.f;
	max_.z = -10000000000.f;

	bool setValues = false;

	set<IStorm3D_Model_Object *>::iterator it = objects.begin();
	for(; it != objects.end(); ++it)
	{
		Storm3D_Model_Object *o = static_cast<Storm3D_Model_Object *> (*it);
		if(!o)
			continue;

		VC3 opos = o->GetPosition();
		QUAT orot = o->GetRotation();
		MAT opostm;
		MAT orottm;
		opostm.CreateTranslationMatrix(opos);
		orottm.CreateRotationMatrix(orot);
		MAT tm = orottm * opostm;

		const OOBB &oobb = o->GetObjectBoundingBox();
		for(int i = 0; i < 6; ++i)
		{
			VC3 position = oobb.center;
			if(i == 0)
				position += oobb.axes[0] * oobb.extents.x;
			else if(i == 1)
				position -= oobb.axes[0] * oobb.extents.x;
			else if(i == 2)
				position += oobb.axes[1] * oobb.extents.y;
			else if(i == 3)
				position -= oobb.axes[1] * oobb.extents.y;
			else if(i == 4)
				position += oobb.axes[2] * oobb.extents.z;
			else if(i == 5)
				position -= oobb.axes[2] * oobb.extents.z;

			tm.TransformVector(position);
			min_.x = min(min_.x, position.x);
			min_.y = min(min_.y, position.y);
			min_.z = min(min_.z, position.z);
			max_.x = max(max_.x, position.x);
			max_.y = max(max_.y, position.y);
			max_.z = max(max_.z, position.z);
		}

		setValues = true;
	}

	if(!setValues)
	{
		min_.x = min_.y = min_.z = 0.f;
		max_.x = max_.y = max_.z = 0.f;
	}
}

//------------------------------------------------------------------
// Storm3D_Model::Object_Delete
//------------------------------------------------------------------
void Storm3D_Model::Object_Delete(IStorm3D_Model_Object *iobject)
{
	//Storm3D_Model_Object *object=(Storm3D_Model_Object*)iobject;

	// First check if this object exists in model
	set<IStorm3D_Model_Object*>::iterator it;
	it=objects.find(iobject);
	if(it != objects.end())
	{
		Storm3D_Model_Object *object = static_cast<Storm3D_Model_Object *> (iobject);
		RemoveCollision(object);

		if(observer)
			observer->remove(object);

		// Delete object and remove it from the set
		delete iobject;
		objects.erase(it);

		collision_objects.erase(object);
		light_objects.erase(object);

		for(unsigned int i = 0; i < objects_array.size(); ++i)
		{
			if(objects_array[i] == object)
			{
				objects_array.erase(objects_array.begin() + i);
				break;
			}
		}
	}

	storm3d_model_objects_created--;
}


//------------------------------------------------------------------
// Storm3D_Model::Helper_Point_New
//------------------------------------------------------------------
IStorm3D_Helper_Point *Storm3D_Model::Helper_Point_New(const char *name, const VC3 &_position)
{
	// Create new helper and add it into the set
	Storm3D_Helper_Point *help=new Storm3D_Helper_Point(name,this,_position);
	helpers.insert(help);

	// Return helper's pointer for user modifications
	return help;
}



//------------------------------------------------------------------
// Storm3D_Model::Helper_Vector_New
//------------------------------------------------------------------
IStorm3D_Helper_Vector *Storm3D_Model::Helper_Vector_New(const char *name, const VC3 &_position,
	const VC3 &_direction)
{
	// Create new helper and add it into the set
	Storm3D_Helper_Vector *help=new Storm3D_Helper_Vector(name,this,_position,_direction);
	helpers.insert(help);

	// Return helper's pointer for user modifications
	return help;
}



//------------------------------------------------------------------
// Storm3D_Model::Helper_Camera_New
//------------------------------------------------------------------
IStorm3D_Helper_Camera *Storm3D_Model::Helper_Camera_New(const char *name, const VC3 &_position,
	const VC3 &_direction, const VC3 &_up)
{
	// Create new helper and add it into the set
	Storm3D_Helper_Camera *help=new Storm3D_Helper_Camera(name,this,_position,_direction,_up);
	helpers.insert(help);

	// Return helper's pointer for user modifications
	return help;
}



//------------------------------------------------------------------
// Storm3D_Model::Helper_Box_New
//------------------------------------------------------------------
IStorm3D_Helper_Box *Storm3D_Model::Helper_Box_New(const char *name, const VC3 &_position,
	const VC3 &_size)
{
	// Create new helper and add it into the set
	Storm3D_Helper_Box *help=new Storm3D_Helper_Box(name,this,_position,_size);
	helpers.insert(help);

	// Return helper's pointer for user modifications
	return help;
}



//------------------------------------------------------------------
// Storm3D_Model::Helper_Sphere_New
//------------------------------------------------------------------
IStorm3D_Helper_Sphere *Storm3D_Model::Helper_Sphere_New(const char *name, const VC3 &_position,
	float radius)
{
	// Create new helper and add it into the set
	Storm3D_Helper_Sphere *help=new Storm3D_Helper_Sphere(name,this,_position,radius);
	helpers.insert(help);

	// Return helper's pointer for user modifications
	return help;
}



//------------------------------------------------------------------
// Storm3D_Model::Helper_Delete
//------------------------------------------------------------------
void Storm3D_Model::Helper_Delete(IStorm3D_Helper *help)
{
	// First check if this light exists in model
	set<IStorm3D_Helper*>::iterator it;
	it=helpers.find(help);
	if (it!=helpers.end())
	{
		// Delete helper and remove it from the set
		delete help;
		helpers.erase(it);
	}
}



//------------------------------------------------------------------
// Storm3D_Model::InformChangeToChilds
// Called when model is rotated/scaled/moved
//------------------------------------------------------------------
void Storm3D_Model::InformChangeToChilds()
{
	// Objects
	for(set<IStorm3D_Model_Object*>::iterator io=objects.begin();io!=objects.end();++io)
	{
		((Storm3D_Model_Object*)(*io))->mxg_update=true;
		((Storm3D_Model_Object*)(*io))->gpos_update_needed=true;
	}
/*
	// Lights
	for(set<IStorm3D_Light*>::iterator il=lights.begin();il!=lights.end();++il)
	{
		switch ((*il)->GetLightType())
		{
			case IStorm3D_Light::LTYPE_POINT:
				((Storm3D_Light_Point*)(*il))->update_globals=true;
				break;

			case IStorm3D_Light::LTYPE_SPOT:
				((Storm3D_Light_Spot*)(*il))->update_globals=true;
				break;
			
			case IStorm3D_Light::LTYPE_DIRECTIONAL:
				((Storm3D_Light_Directional*)(*il))->update_globals=true;
				break;
		}
	}
*/
/*
	// Helpers
	for(set<IStorm3D_Helper*>::iterator ih=helpers.begin();ih!=helpers.end();++ih)
	{
		switch ((*ih)->GetHelperType())
		{
			case IStorm3D_Helper::HTYPE_POINT:
				((Storm3D_Helper_Point*)(*ih))->update_globals=true;
				break;

			case IStorm3D_Helper::HTYPE_VECTOR:
				((Storm3D_Helper_Vector*)(*ih))->update_globals=true;
				break;
			
			case IStorm3D_Helper::HTYPE_CAMERA:
				((Storm3D_Helper_Camera*)(*ih))->update_globals=true;
				break;
			
			case IStorm3D_Helper::HTYPE_BOX:
				((Storm3D_Helper_Box*)(*ih))->update_globals=true;
				break;
			
			case IStorm3D_Helper::HTYPE_SPHERE:
				((Storm3D_Helper_Sphere*)(*ih))->update_globals=true;
				break;
		}
	}
*/
	// Bones
// BARRELHAX
//	for(unsigned int i = 0; i < bones.size(); ++i)
//		bones[i]->ParentMoved(GetMX());
}

bool Storm3D_Model::hasBones ()
{
	return !bones.empty();
}

//------------------------------------------------------------------
// Storm3D_Model::SetPosition
//------------------------------------------------------------------
void Storm3D_Model::SetPosition(const VC3 &_position)
{
	if(fabsf(position.x - _position.x) < 0.01f)
	if(fabsf(position.y - _position.y) < 0.01f)
	if(fabsf(position.z - _position.z) < 0.01f)
		return;

	position=_position;

	// Inform objects and lights of change
	// Only if they haven't been already informed (optimization)
	if (!mx_update) InformChangeToChilds();

	// Set update flag
	mx_update=true;

// BARRELHAX
//	const Matrix &mx = GetMX();
//	for(unsigned int i = 0; i < bones.size(); ++i)
//		bones[i]->ParentMoved(mx);
for(unsigned int i = 0; i < bones.size(); ++i)
	bones[i]->ParentPositionMoved(position);

box_ok = false;
collision_box_ok = false;

	if(observer)
		observer->updatePosition(position);
}



//------------------------------------------------------------------
// Storm3D_Model::SetRotation
//------------------------------------------------------------------
void Storm3D_Model::SetRotation(const QUAT &_rotation)
{
	rotation=_rotation;

	// Inform objects and lights of change
	// Only if they haven't been already informed (optimization)
	if (!mx_update) InformChangeToChilds();

box_ok = false;
collision_box_ok = false;

	// Set update flag
	mx_update=true;

// BARRELHAX
//	for(unsigned int i = 0; i < bones.size(); ++i)
//		bones[i]->ParentMoved(GetMX());
}



//------------------------------------------------------------------
// Storm3D_Model::SetScale
//------------------------------------------------------------------
void Storm3D_Model::SetScale(const VC3 &_scale)
{
	// HACK: re-scale if already calculated bounding sphere...?
	// WARNING: incorrect result when calling SetScale several times!!!
	// WARNING: may have not effect when adding model objects after setting scale
	/*
	if (scale.x > 0 && scale.y > 0 && scale.z > 0)
	{
		float maxScale = _scale.x / scale.x;
		if (_scale.y / scale.y > maxScale) maxScale = _scale.y / scale.y;
		if (_scale.z / scale.z > maxScale) maxScale = _scale.z / scale.z;
		original_bounding_radius *= maxScale;
		bounding_radius *= maxScale;

		if(observer)
			observer->updateRadius(bounding_radius);
	}
  */

	if(observer)
		observer->updateRadius(bounding_radius * max_scale);

	box_ok = false;
	collision_box_ok = false;
	scale=_scale;

	// Inform objects and lights of change
	// Only if they haven't been already informed (optimization)
	if (!mx_update) InformChangeToChilds();

	// Set update flag
	mx_update=true;

	max_scale = max(scale.x, scale.y);
	max_scale = max(max_scale, scale.z);
}

void Storm3D_Model::ResetObjectLights() 
{ 
	for(set<IStorm3D_Model_Object*>::iterator io=objects.begin();io!=objects.end();++io)
	{
		Storm3D_Model_Object *o = ((Storm3D_Model_Object*)(*io));
		//o->light_index1 = -1;
		//o->light_index2 = -1;

		for(int i = 0; i < LIGHT_MAX_AMOUNT; ++i)
			o->light_index[i] = -1;
	}
}

/*
void Storm3D_Model::SetSelfIllumination(const COL &color)
{
	std::set<IStorm3D_Model_Object *>::iterator it = objects.begin();
	for(; it != objects.end(); ++it)
	{
		Storm3D_Model_Object *object = static_cast<Storm3D_Model_Object *> (*it);
		if(!object)
			continue;

		Storm3D_Mesh *mesh = static_cast<Storm3D_Mesh *> (object->GetMesh());
		if(!mesh)
			continue;

		mesh->GetMaterial()->SetSelfIllumination(color);
	}	
}
*/
//------------------------------------------------------------------
// Storm3D_Model::Storm3D_Model
//------------------------------------------------------------------
Storm3D_Model::Storm3D_Model(Storm3D *s2) :
	Storm3D2(s2),
	mx_update(true),
	no_collision(false),
	scale(1,1,1),
	bone_boneid(0),
	model_boneid(0),
	lodLevel(0),
	custom_data(0),
	bounding_radius(0.f),
	original_bounding_radius(0.f),
	cast_shadows(true),
	bone_collision(true),
	observer(0),
	type_flag(0),
	terrain_object(false),
	terrain_lightmapped_object(false),
	terrainInstanceId(-1),
	terrainModelId(-1),
	sun_strength(0),
	always_use_sun(false),
	max_scale(1.f),
	box_ok(false),
	collision_box_ok(false),
	use_cylinder_collision(false),
	occluded(false),
	skyModel(false),
	animation_paused(false),
	need_cull_adding(false)
{
	for(int i = 0; i < LIGHT_MAX_AMOUNT; ++i)
		light_index[i] = -1;

	// Create iterators
	ITObject=new ICreateIM_Set<IStorm3D_Model_Object*>(&(objects));
	ITHelper=new ICreateIM_Set<IStorm3D_Helper*>(&(helpers));

	storm3d_model_allocs++;

#ifdef WORLD_FOLDING_ENABLED
	mfold = WorldFold::getWorldFoldForPosition(position);
	mfold_key = WorldFold::getWorldFoldKeyForPosition(position);
	last_mfold_key_value = *mfold_key;
#endif

}



//------------------------------------------------------------------
// Storm3D_Model::~Storm3D_Model
//------------------------------------------------------------------
Storm3D_Model::~Storm3D_Model()
{
	storm3d_model_allocs--;

	// Remove from Storm3D's list
	Storm3D2->Remove(this);

	// Delete iterators
	delete ITObject;
	delete ITHelper;

	// Empty model
	Empty();
}

//------------------------------------------------------------------
// Storm3D_Model::GetMX
//------------------------------------------------------------------
MAT &Storm3D_Model::GetMX()
{
	// Rebuild MX if needed
	if (mx_update)
	{
		mx_update=false;
		MAT ms,mr,mp;
		ms.CreateScaleMatrix(scale);
		mr.CreateRotationMatrix(rotation);
		mp.CreateTranslationMatrix(position);
		mx=ms*mr*mp;

#ifdef WORLD_FOLDING_ENABLED
		mfold = WorldFold::getWorldFoldForPosition(position);
		mfold_key = WorldFold::getWorldFoldKeyForPosition(position);
		// by setting the mx_fold_key != mfold_key, forcing below rebuild of mx_folded 
		last_mfold_key_value = *mfold_key + 1;
#endif
	}

#ifdef WORLD_FOLDING_ENABLED
	if (*mfold_key != last_mfold_key_value)
	{
		// if not here because of xm_update, then probably mfold_key has changed - should inform children as well...
		if (!mx_update) 
		{
			InformChangeToChilds();
		}
		last_mfold_key_value = *mfold_key;
		mx_folded = mx * (*mfold);
	}
	return mx_folded;
#else
	return mx;
#endif
}

VC3 Storm3D_Model::GetApproximatedPosition()
{
	return approx_position;
}

void Storm3D_Model::SetCustomData(IStorm3D_Model_Data *data)
{
	custom_data = data;
}

IStorm3D_Model_Data *Storm3D_Model::GetCustomData()
{
	return custom_data;
}

void Storm3D_Model::SetTypeFlag(unsigned int flag)
{
	type_flag = flag;
}

unsigned int Storm3D_Model::GetTypeFlag() const
{
	return type_flag;
}

void Storm3D_Model::updateRadiusToContain(const VC3 &pos, float radius)
{
	// update model's bounding sphere 
	// (grow it if necessary, it is never shrunk!)
	// --jpk
	float need_radius = pos.GetLength() + radius; 
	if (bounding_radius < need_radius)
	{
		bounding_radius = need_radius;
		box_ok = false;
		collision_box_ok = false;
		
		if(observer)
			observer->updateRadius(bounding_radius * max_scale);
	}

	original_bounding_radius = bounding_radius;	
}

//------------------------------------------------------------------
// Storm3D_Model::CopyModel
//------------------------------------------------------------------
/*void Storm3D_Model::CopyModel(IStorm3D_Model *iother)
{
	Storm3D_Model *other=(Storm3D_Model*)iother;

	// Test
	if (other==NULL) return;

	// Empty old stuff first
	Empty();

	// Copy objects
	for(set<IStorm3D_Model_Object*>::iterator io=other->objects.begin();io!=other->objects.end();io++)
	{
		// Typecast (to simplify code)
		Storm3D_Model_Object *orig_obj=(Storm3D_Model_Object*)*io;

		// Create new object
		Storm3D_Model_Object *obj=(Storm3D_Model_Object*)Object_New(orig_obj->name);
		obj->CopyObject(orig_obj);
		
		// Store original parents... 
		obj->parent=orig_obj->parent;
	}

	// Loop through objects to set linking
	for(io=objects.begin();io!=objects.end();io++)
	{
		// Typecast (to simplify code)
		Storm3D_Model_Object *obj=(Storm3D_Model_Object*)*io;

		if (obj->parent)
		{
			// Do some tricks...
			IStorm3D_Model_Object *tpa=obj->parent;
			obj->parent=NULL;

			// Search parent object
			IStorm3D_Model_Object *op=SearchObject(tpa->GetName());
			if (op) op->AddChild(obj);
		}
	}

	// BETA!!: Copy lights/helpers too!

	// Copy lights
	for(set<IStorm3D_Light*>::iterator il=other->lights.begin();il!=other->lights.end();il++)
	{
		// Typecast (to simplify code)
		/*IStorm3D_Light *orig_lgt=(IStorm3D_Light*)*io;

		switch (orig_lgt->GetLightType())
		{
			case IStorm3D_Light::LTYPE_POINT:
			{
				// Create new light
				Storm3D_Light_Point *opl=(Storm3D_Light_Point*)orig_lgt;
				Storm3D_Light_Point *nlgt=(Storm3D_Light_Point*)Light_Point_New(opl->name,opl->color,opl->multiplier,opl->decay,opl->position,opl->lflare);
		
				// Store original parents... 
				nlgt->parent_object=opl->parent_object;
			}
			break;

		}*/
/*
	}
	
	// Copy helpers
	for(set<IStorm3D_Helper*>::iterator ih=other->helpers.begin();ih!=other->helpers.end();ih++)
	{
	}

	// Copy other parameters
	position=other->position;
	rotation=other->rotation;
	scale=other->scale;
	mx_update=true;
}*/



//------------------------------------------------------------------
// Storm3D_Model::Optimize
//------------------------------------------------------------------
/*void Storm3D_Model::Optimize()
{
	// This routine searches the model for objects with the
	// same material, and adds them together as a one big object.

	// BETA: linking-stuff does not work!

	// Copy objects
	for(set<Storm3D_Model_Object*>::iterator io=other->objects.begin();io!=other->objects.end();io++)
	{
		// Typecast (to simplify code)
		Storm3D_Model_Object *orig_obj=*io;

		// Create new object
		Storm3D_Model_Object *obj=Object_New();
		obj->CopyObject(orig_obj);
	}

	// Copy lights
	for(set<Storm3D_Light*>::iterator il=other->lights.begin();il!=other->lights.end();il++)
	{
	}
	
	// Copy helpers
	for(set<Storm3D_Helper*>::iterator ih=other->helpers.begin();ih!=other->helpers.end();ih++)
	{
	}

	// Copy other parameters
	position=other->position;
	rotation=other->rotation;
	scale=other->scale;	
	mx=other->mx;
	mx_update=other->mx_update;
}*/



//------------------------------------------------------------------
// Storm3D_Model::RayTrace
//------------------------------------------------------------------
void Storm3D_Model::RayTrace(const VC3 &position,const VC3 &direction_normalized,float ray_length,Storm3D_CollisionInfo &rti, bool accurate)
{
	if(no_collision == true)
		return;
	if(terrain_object && !rti.includeTerrainObjects)
		return;

	if(!bones.empty() && !terrain_object && bone_collision)
	{
		// First check main sphere which contains whole model
		VC3 distanceVector(position - this->position);
#ifdef BONE_MODEL_SPHERE_TRANSFORM
		float boundingSphere;
		float movedUp = 0.0f;
		if (bounding_radius < 0.6f)
		{
			movedUp = 0.3f;
			boundingSphere = bounding_radius;
		} else {
			movedUp = 0.6f;
			boundingSphere = bounding_radius - 0.6f;
			if (boundingSphere < 0.6f)
				boundingSphere = 0.6f;
		}
		distanceVector.y -= movedUp;
#else
		float boundingSphere = bounding_radius;
#endif

		// model (sphere) out of range?
		if ((ray_length + boundingSphere) * (ray_length + boundingSphere) < distanceVector.GetSquareLength())
			return;

		// then do a big bounding sphere check
		// Test if the ray intersects object's bounding sphere
		// v3: 7 multiplies (old: 8 mul + 1 sqrt)
		float vectoraylen2=distanceVector.GetSquareLength();
		float kantalen=direction_normalized.GetDotWith(distanceVector);
		float dist2=vectoraylen2-kantalen*kantalen;

		if (dist2 > boundingSphere * boundingSphere) 
			return;

		// Sphere behind ray?
		if((kantalen > 0) && (vectoraylen2 > boundingSphere * boundingSphere))
			return;

		// <--- END

		// NOTE: assuming bounding_sphere only check for any model with bones
		// thus, if not accurate raytrace, bounding sphere hit is enough
		// (a very inaccurate implementation)
		// -- jpk
		if (!accurate)
		{
			// NOTE: copy&paste programming, from below code...
			// combined with some code from model_object raytrace
			// FIXME: returns some object to avoid blowing client code
			if(objects.empty() == true)
			{
				return;
			}
			float dist = distanceVector.GetLength();
			if ((rti.hit && dist > rti.range) || dist > ray_length)
			{
				return;
			}

			rti.hit = true;
			rti.range = dist;

			rti.position = this->position;
#ifdef BONE_MODEL_SPHERE_TRANSFORM
			rti.position.y += movedUp;
#endif
			rti.model = this;
			rti.object = 0;

			rti.object = *(objects.begin());
		}

		// Test if the ray intersects bones bounding sphere
		// Should use thickness/direction for better accuracy
		
		//Vector relative_position;
		Vector collision_position;

		// Collide just to bones
		for(unsigned int i = 0; i < bones.size(); ++i)
		{
			const Matrix &bone_tm = bones[i]->GetTM();
			float bone_length = bones[i]->GetLenght();
			float bone_thickness = bones[i]->GetThickness();

			if (bones[i]->HasNoCollision())
				continue;

float scaleFactor = scale.x;
bone_length *= scaleFactor;
bone_thickness *= scaleFactor;

			VC3 real_bone_direction(bone_tm.Get(8), bone_tm.Get(9), bone_tm.Get(10));
real_bone_direction.Normalize();

			VC3 bone_direction = real_bone_direction;

			// Vector from ray origin to bone origin
			VC3 bone_position = bone_tm.GetTranslation();

			// move sphere to middle of the bone... (also halve length)
			bone_length *= 0.5f;
			bone_direction *= bone_length;
			bone_position += bone_direction;

			if(use_cylinder_collision)
				bone_length += bone_thickness;

			Sphere boneSphere(bone_position, bone_length);
			Ray ray(position, direction_normalized, ray_length);
			if (!collision(boneSphere, ray))
			{
				continue;
			}

			if(use_cylinder_collision)
			{
				const VC3 &rayStart = position;
				VC3 rayEnd = rayStart + (direction_normalized * ray_length);

				VC3 cylinderStart = bone_position - (real_bone_direction * bone_length);
				VC3 cylinderEnd = bone_position + (real_bone_direction * bone_length);

				if(!hasInfiniteCylinderCollision(rayStart, rayEnd, cylinderStart, cylinderEnd, bone_thickness))
					continue;
			}

			/*
			// Relative distance (squared)
			float relative_distance = relative_position.GetSquareLength();
			float relative_distance_nonsq = sqrtf(relative_distance);
			if((relative_distance_nonsq - bone_length)*(relative_distance_nonsq - bone_length) > ray_length*ray_length)
				continue;
 			//if(relative_distance - bone_length*bone_length > ray_length*ray_length)
			//	continue;

			// Project relative position to ray
			float projected_distance = relative_position.GetDotWith(direction_normalized);

			// Sphere behind ray?
			if((projected_distance < 0) && (relative_distance > bone_length*bone_length))
				continue;

			// Pythagoream theorem test (fancy name at least)
			// If projection to radius is bigger than actual radius -> there can be no collision
			float projection_radius_nonsq = relative_distance_nonsq - projected_distance;
			if(projection_radius_nonsq > bone_length)
				continue;
			*/

			/*
 			float projection_radius = relative_distance - projected_distance*projected_distance;
 			if(projection_radius > bone_length*bone_length)
				continue;
			*/

/*
			// Bugs ;-)

			// Distance to closest intersection
			float q = sqrtf(bone_length - projected_distance);
			float distance = 0.f;
			if(relative_distance > bone_length)
				distance = bone_length - q;
			else
				distance = bone_length + q;

			//collision_position = position + (direction_normalized * distance);
*/
			// Not accurate
			collision_position = bone_position; //bone_tm.GetTranslation();

			// Already collision closer than this
			float collision_range = (collision_position - position).GetLength();
			if((rti.hit == true) && (collision_range > rti.range))
				continue;

			rti.hit = true;
			rti.range = collision_range;

			rti.position = collision_position;
			rti.model = this;
			rti.object = 0;

			// FIXME: returns some object to avoid blowing client code
			if(objects.empty() == true)
			{
				rti.hit = false;
				continue;
			}

			rti.object = *(objects.begin());
		}
	}
	else
	{
		// Static geometry

		// Raytrace to each object	
		//for(set<IStorm3D_Model_Object*>::iterator io=objects.begin();io!=objects.end();++io)
		for(set<Storm3D_Model_Object*>::iterator io = collision_objects.begin(); io != collision_objects.end(); ++io)
		{
			// Typecast (to simplify code)
			//Storm3D_Model_Object *obj=(Storm3D_Model_Object*)*io;
			Storm3D_Model_Object *obj = *io;

			// If object has no_collision skip it
			if (obj->no_collision) continue;
			//if (obj->is_volume_fog) continue;

			// Raytrace
			obj->RayTrace(position,direction_normalized,ray_length,rti,accurate);
		}

	}
}



//------------------------------------------------------------------
// Storm3D_Model::SphereCollision
//------------------------------------------------------------------
void Storm3D_Model::SphereCollision(const VC3 &position,float radius,Storm3D_CollisionInfo &cinf, bool accurate)
{
	if(no_collision == true)
		return;

	// Spherecollide to each object	
	//for(set<IStorm3D_Model_Object*>::iterator io=objects.begin();io!=objects.end();++io)
	for(set<Storm3D_Model_Object*>::iterator io = collision_objects.begin(); io != collision_objects.end(); ++io)
	{
		// Typecast (to simplify code)
		//Storm3D_Model_Object *obj=(Storm3D_Model_Object*)*io;
		Storm3D_Model_Object *obj = *io;

		// Spherecollide
		obj->SphereCollision(position,radius,cinf, accurate);
	}
}



//------------------------------------------------------------------
// Storm3D_Model::SearchObject
//------------------------------------------------------------------
IStorm3D_Model_Object *Storm3D_Model::SearchObject(const char *name)
{
	// Search for named object
	for(set<IStorm3D_Model_Object*>::iterator io=objects.begin();io!=objects.end();++io)
	{
		// Typecast (to simplify code)
		IStorm3D_Model_Object *obj=*io;

		// Test if names are same
		if (strcmp(name,obj->GetName())==0) return obj;
	}

	// Not found
	return NULL;
}

void Storm3D_Model::CastShadows(bool shadows)
{
	cast_shadows = shadows;
}

void Storm3D_Model::EnableBoneCollision(bool enable)
{
	bone_collision = enable;
}

void Storm3D_Model::FreeMemoryResources()
{
	GetRadius();
	GetBoundingBox();

	for(set<IStorm3D_Model_Object*>::iterator io = objects.begin(); io != objects.end(); ++io)
	{
		Storm3D_Model_Object *obj = static_cast<Storm3D_Model_Object *> (*io);
		Storm3D_Mesh *m = obj->mesh;
		if(!obj->no_collision)
			m->UpdateCollisionTable();
		if(!obj->no_render || obj->no_collision)
			m->ReBuild();

		m->GetRadius();
		m->getBoundingSphere();
		m->getBoundingBox();
		obj->GetObjectBoundingBox();
		obj->GetBoundingBox();
		obj->GetBoundingSphere();

		delete[] m->vertexes;
		m->vertexes = 0;
		m->vertex_amount = 0;
		delete[] m->faces[0];
		m->faces[0] = 0;
		m->face_amount[0] = 0;
	}
}

//------------------------------------------------------------------
// Storm3D_Model::SearchHelper
//------------------------------------------------------------------
IStorm3D_Helper *Storm3D_Model::SearchHelper(const char *name)
{
	// Search for named helper
	for(set<IStorm3D_Helper*>::iterator ih=helpers.begin();ih!=helpers.end();++ih)
	{
		// Typecast (to simplify code)
		IStorm3D_Helper *help=*ih;

		// Test if names are same
		if (strcmp(name,help->GetName())==0) return help;
	}

	// Not found
	return NULL;
}


//------------------------------------------------------------------
// Storm3D_Model::SearchBone
//------------------------------------------------------------------
IStorm3D_Bone *Storm3D_Model::SearchBone(const char *name)
{
	for(unsigned int i = 0; i < bones.size(); ++i)
	if(strcmp(bones[i]->GetName(), name) == 0)
		return bones[i];

	return 0;
}

IStorm3D_Bone *Storm3D_Model::GetBone(int i)
{
	if(((unsigned int)i) >= bones.size()) return 0;
	return bones[i];
}


// Bone animations

bool Storm3D_Model::SetRandomAnimation(IStorm3D_BoneAnimation *animation_)
{
	if(bones.size() == 0)
		return false;

	bounding_radius = BOUNDING_INITIAL_RADIUS;
	if(observer)
		observer->updateRadius(BOUNDING_INITIAL_RADIUS);

	normal_animations.clear();
	if(animation_ == 0)
		return false;

	Storm3D_BoneAnimation *animation = static_cast<Storm3D_BoneAnimation*> (animation_);
	if(animation->GetId() != bone_boneid)
		return false;

	//!!
	normal_animations.clear();

	Model_BoneAnimation foo(0, animation, true, 0, 0);
	foo.SetState(Model_BoneAnimation::Normal, 0);
	foo.animation_time = rand() % foo.animation->GetLength();
	normal_animations.push_back(foo);
	return true;
}

bool Storm3D_Model::SetAnimation(IStorm3D_BoneAnimation *transition_, IStorm3D_BoneAnimation *animation_, bool loop)
{
	if(bones.size() == 0)
		return false;

	bounding_radius = BOUNDING_INITIAL_RADIUS;
	if(observer)
		observer->updateRadius(BOUNDING_INITIAL_RADIUS);

/*
	if(animation_ == 0)
	{
		current_animation = Model_BoneAnimation();
		return false;
	}

	Storm3D_BoneAnimation *animation = static_cast<Storm3D_BoneAnimation*> (animation_);
	if(animation->GetId() != bone_boneid)
		return false;

	current_animation = Model_BoneAnimation(animation, loop);
*/
	normal_animations.clear();
	if(animation_ == 0)
		return false;

	Storm3D_BoneAnimation *animation = static_cast<Storm3D_BoneAnimation*> (animation_);
	if(animation->GetId() != bone_boneid)
		return false;
	Storm3D_BoneAnimation *transition = static_cast<Storm3D_BoneAnimation*> (transition_);
	if(transition && transition->GetId() != bone_boneid)
		return false;

	//!!
	normal_animations.clear();

	Model_BoneAnimation foo(transition, animation, loop, 0, 0);
	foo.SetState(Model_BoneAnimation::Normal, 0);
	normal_animations.push_back(foo);
	return true;
}

bool Storm3D_Model::BlendToAnimation(IStorm3D_BoneAnimation *transition_, IStorm3D_BoneAnimation *animation_, int blend_time, bool loop)
{
	if((bones.size() == 0) || (animation_ == NULL))
		return false;

	bounding_radius = BOUNDING_INITIAL_RADIUS;
	if(observer)
		observer->updateRadius(BOUNDING_INITIAL_RADIUS);

	Storm3D_BoneAnimation *animation = static_cast<Storm3D_BoneAnimation*> (animation_);
	if(animation->GetId() != bone_boneid)
		return false;
	Storm3D_BoneAnimation *transition = static_cast<Storm3D_BoneAnimation*> (transition_);
	if(transition && transition->GetId() != bone_boneid)
		return false;

	int animation_time = 0;

/*
	// If new animation is looping, set to same animation time
	// (walking/running etc are often synced -> looks smoother)
	if((loop == true) && (current_animation.HasAnimation() == true))
	{
		int t = current_animation.GetAnimationTime();
		if(t < animation->GetLength())
			animation_time = t;
	}
*/
  
	Model_BoneAnimation foo(transition, animation, loop, blend_time, animation_time);
	foo.SetState(Model_BoneAnimation::BlendIn, blend_time);

	/*
	// Fade out current animations
	{
		std::list<Model_BoneAnimation>::iterator it;
		for(it = normal_animations.begin(); it != normal_animations.end(); ++it)
		{
			it->SetState(Model_BoneAnimation::BlendOut, blend_time);
		}
	}
	*/

	/*
	std::list<Model_BoneAnimation>::iterator it;
	for(it = blending_to_animations.begin(); it != blending_to_animations.end(); ++it)
	{
		if(foo < (*it))
		{
			blending_to_animations.insert(it, foo);
			return true;
		}
	}

	blending_to_animations.push_back(foo);
	*/

	normal_animations.push_back(foo);	
	return true;
}

bool Storm3D_Model::BlendWithAnimationIn(IStorm3D_BoneAnimation *transition_, IStorm3D_BoneAnimation *animation_, int blend_in_time, bool loop, float blend_factor)
{
	if((bones.size() == 0) || (animation_ == NULL))
		return false;

	bounding_radius = BOUNDING_INITIAL_RADIUS;
	if(observer)
		observer->updateRadius(BOUNDING_INITIAL_RADIUS);

	Storm3D_BoneAnimation *animation = static_cast<Storm3D_BoneAnimation*> (animation_);
	if(animation->GetId() != bone_boneid)
		return false;	
	Storm3D_BoneAnimation *transition = static_cast<Storm3D_BoneAnimation*> (transition_);
	if(transition && transition->GetId() != bone_boneid)
		return false;

/*
	// Fix? -- if already has same animation set, blend it out
	{
		std::list<Model_BoneAnimation>::iterator it;
		for(it = blending_with_animations.begin(); it != blending_with_animations.rend(); ++it)
		{
			Model_BoneAnimation &a = *it;
			if(a.GetAnimation() == animation)
			{
				a.SetState(Model_BoneAnimation::BlendOut, blend_in_time);

				if(a.GetState() == Model_BoneAnimation::BlendIn)
					a.elapsed_time = blend_in_time - a.elapsed_time;
			}
		}
	}
*/
	Model_BoneAnimation foo(transition, animation, loop, blend_in_time);
	foo.SetState(Model_BoneAnimation::BlendIn, blend_in_time);
	foo.SetBlendFactor(blend_factor);

	blending_with_animations.push_back(foo);
	return true;
}

bool Storm3D_Model::BlendWithAnimationOut(IStorm3D_BoneAnimation *transition_, IStorm3D_BoneAnimation *animation, int blend_time)
{
	Storm3D_BoneAnimation *transition = static_cast<Storm3D_BoneAnimation*> (transition_);
	if(transition && transition->GetId() != bone_boneid)
		return false;

	std::list<Model_BoneAnimation>::reverse_iterator it;
	for(it = blending_with_animations.rbegin(); it != blending_with_animations.rend(); )
	{
		if(((*it).GetAnimation() == animation) && ((*it).GetState() != Model_BoneAnimation::BlendOut))
		{
			it->SetState(Model_BoneAnimation::BlendOut, blend_time);
			if(transition)
			{
				it->transition = transition;
				it->transition->AddReference();
			}

			return true;
		}
		else
			++it;
	}

	return false;
}

void Storm3D_Model::BlendBoneIn(IStorm3D_Bone *bone, const QUAT &rotation, int blend_time)
{
	// !! HAX!
	manual_blendings.clear();

	for(unsigned int i = 0; i < bones.size(); ++i)
	{
		if(bones[i] == bone)
		{
			std::list<Model_BoneBlend>::iterator it;
			for(it = manual_blendings.begin(); it != manual_blendings.end(); ++it)
			{
				if(it->GetIndex() == int(i))
				{
					if(it->GetState() != Model_BoneAnimation::BlendOut)
						it->SetState(Model_BoneAnimation::BlendOut, blend_time);

					break;
				}
			}

			QUAT blendRotation = bones[i]->GetOriginalGlobalRotation() * rotation;

			Model_BoneBlend blend(i, blendRotation, blend_time);
			blend.SetState(Model_BoneAnimation::Normal, 0);
			manual_blendings.push_back(blend);
			return;
		}
	}
}

void Storm3D_Model::BlendBoneOut(IStorm3D_Bone *bone, int blend_time)
{
	for(unsigned int i = 0; i < bones.size(); ++i)
	{
		if(bones[i] == bone)
		{
			std::list<Model_BoneBlend>::iterator it;
			for(it = manual_blendings.begin(); it != manual_blendings.end(); ++it)
			{
				if(((*it).GetIndex() == int(i)) && ((*it).GetState() != Model_BoneAnimation::BlendOut))
				{
					(*it).SetState(Model_BoneAnimation::BlendOut, blend_time);
					return;
				}
			}
		}
	}
}


void Storm3D_Model::SetAnimationSpeedFactor(
	IStorm3D_BoneAnimation *animation, float speedFactor)
{
	if(bones.empty() == true)
		return;

	std::list<Model_BoneAnimation>::reverse_iterator it;
	for(it = blending_with_animations.rbegin(); it != blending_with_animations.rend(); ++it)
	{
		if((it->GetAnimation() == animation) && (it->GetState() != Model_BoneAnimation::BlendOut))
		{
			it->SetSpeedFactor(speedFactor);
			// ...here same as above, we may stop here, if no duplicates
			return;
		}
	}

	for(it = normal_animations.rbegin(); it != normal_animations.rend(); ++it)
	{
		if((it->GetAnimation() == animation) && (it->GetState() != Model_BoneAnimation::BlendOut))
		{
			it->SetSpeedFactor(speedFactor);
			// ...here same as above, we may stop here, if no duplicates
			return;
		}
	}
}


void Storm3D_Model::AdvanceAnimation(int time_delta)
{
	if(bones.empty() == true)
		return;

	if(animation_paused)
		return;

//BARRELHAX
{
for(unsigned int i = 0; i < bones.size(); ++i)
	bones[i]->ParentMoved(GetMX());
}

	// Reset bones
	for(unsigned int i = 0; i < bones.size(); ++i)
		bones[i]->ResetAnimations();
	
	// Way too much looping over same containers
	// Should really optimize this stuff, >10% storms cpu usage 
	//	-- psd

	if(!normal_animations.empty())
	{
		std::list<Model_BoneAnimation>::iterator it;
		for(it = normal_animations.begin(); it != normal_animations.end(); ++it)
			(*it).AdvanceAnimation(time_delta, true);

		int normalAnimations = 0;
		for(it = normal_animations.begin(); it != normal_animations.end(); ++it)
		{
			if(it->GetState() == Model_BoneAnimation::Normal)
				++normalAnimations;
		}
		
		for(it = normal_animations.begin(); it != normal_animations.end();)
		{
			if(((*it).HasEnded() == true) && ((*it).GetState() == Model_BoneAnimation::BlendOut))
			{
				it = normal_animations.erase(it);
				continue;
			}

			if(it->GetState() == Model_BoneAnimation::Normal && --normalAnimations > 0)
			{
				it = normal_animations.erase(it);
			}
			else
				++it;
		}

		/*
		for(it = normal_animations.begin(); it != normal_animations.end();)
		{
			if(((*it).HasEnded() == true) && ((*it).GetState() == Model_BoneAnimation::BlendOut))
				it = normal_animations.erase(it);
			else
				++it;
		}	
		*/

		for(it = normal_animations.begin(); it != normal_animations.end(); ++it)
		{
			if(((*it).HasEnded() == true) && ((*it).GetState() == Model_BoneAnimation::BlendIn))
				(*it).SetState(Model_BoneAnimation::Normal, 0);
		}
	}

	if(!blending_with_animations.empty())
	{
		std::list<Model_BoneAnimation>::iterator it;
		for(it = blending_with_animations.begin(); it != blending_with_animations.end(); ++it)
			(*it).AdvanceAnimation(time_delta, false);

		for(it = blending_with_animations.begin(); it != blending_with_animations.end();)
		{
			if(((*it).HasEnded() == true) && ((*it).GetState() == Model_BoneAnimation::BlendOut))
				it = blending_with_animations.erase(it);
			else
				++it;
		}

		for(it = blending_with_animations.begin(); it != blending_with_animations.end(); ++it)
		{
			if(((*it).HasEnded() == true) && ((*it).GetState() == Model_BoneAnimation::BlendIn))		
				(*it).SetState(Model_BoneAnimation::Normal, 0);
		}
	}

	if(!manual_blendings.empty())
	{
		std::list<Model_BoneBlend>::iterator il;
		for(il = manual_blendings.begin(); il != manual_blendings.end(); ++il)
			(*il).AdvanceAnimation(time_delta);

		for(il = manual_blendings.begin(); il != manual_blendings.end();)
		{
			if(((*il).HasEnded() == true) && ((*il).GetState() == Model_BoneAnimation::BlendOut))
				il = manual_blendings.erase(il);
			else
				++il;
		}
	}

	ApplyAnimations();
	Storm3D_Bone::TransformBones(&bones);
	approx_position.x = approx_position.z = 0.f;
	approx_position.y = 0.f;

	float maxRadius = 0; //bounding_radius;
	int boneAmount = bones.size();
	for(int i = 0; i < boneAmount; ++i)
	{
		const MAT &tm = bones[i]->GetTM();
		VC3 pos = tm.GetTranslation();

		approx_position.x += pos.x;
		approx_position.z += pos.z;
		if(i == 0 || pos.y < approx_position.y)
			approx_position.y = pos.y;

		pos -= position;
		float length = bones[i]->GetLenght();
		float radius = pos.GetLength() + length;

		if(radius > maxRadius)
			maxRadius = radius;
	}

	approx_position.x /= float(boneAmount);
	approx_position.z /= float(boneAmount);

	if(maxRadius > original_bounding_radius)
	{
		bounding_radius = maxRadius;
		if(observer)
			observer->updateRadius(bounding_radius);
	}
	else
	{
		bounding_radius = original_bounding_radius;
		if(observer)
			observer->updateRadius(original_bounding_radius);
	}

	std::set<IStorm3D_Model_Object *>::iterator it = objects.begin();
	for(; it != objects.end(); ++it)
	{
		Storm3D_Model_Object *o = static_cast<Storm3D_Model_Object *> (*it);
		o->sphere_ok = false;

		Storm3D_Mesh *m = static_cast<Storm3D_Mesh *> (o->GetMesh());
		m->radius = bounding_radius;
	}

	//char buf[20] = { 0 };
	//sprintf(buf, "%f\r\n", bounding_radius);
	//OutputDebugString(buf);
}

void Storm3D_Model::ApplyAnimations()
{
	// Removed those nasty heap allocations
	Rotation rotation;
	Vector position;

	// Some duplication to avoid redundant queries

/*
	if(current_animation.HasAnimation())
	for(int i = 0; i < bones.size(); ++i)
	{
		Storm3D_Bone *bone = bones[i];

		// Animate rotation
		if(current_animation.GetRotation(i, &rotation))
			bone->AnimateRotation(rotation);

		// Animate position
		if(current_animation.GetPosition(i, &position))
			bone->AnimatePosition(position);
	}
*/
	// normal animations
	std::list<Model_BoneAnimation>::iterator it;
	for(it = normal_animations.begin(); it != normal_animations.end(); ++it)
	{
		float blending_factor = 1.f;

		if((*it).GetState() != Model_BoneAnimation::Normal)
		if((*it).GetBlendProperties(&blending_factor) == false)
			continue;
		
		for(unsigned int i = 0; i < bones.size(); ++i)
		{
			Storm3D_Bone *bone = bones[i];

			// Animate rotation
			if((*it).GetRotation(i, &rotation))
				bone->AnimateRotation(rotation, blending_factor, true);

			// Animate position
			if((*it).GetPosition(i, &position))
				bone->AnimatePosition(position, blending_factor, true);
		}
	}
/*
	// blending_with_animations
	for(it = blending_with_animations_in.begin(); it != blending_with_animations_in.end(); ++it)
	{
		if((*it).GetBlendProperties(&blending_factor) == false)
			continue;

		for(int i = 0; i < bones.size(); ++i)
		{
			Storm3D_Bone *bone = bones[i];

			// Animate rotation
			if((*it).GetRotation(i, &rotation))
				bone->AnimateRotation(rotation, blending_factor, true);

			// Animate position
			if(bone->GetParentIndex() != -1)
			if((*it).GetPosition(i, &position))
				bone->AnimatePosition(position, blending_factor, true);
		}
	}
*/
	for(it = blending_with_animations.begin(); it != blending_with_animations.end(); ++it)
	{
		float blending_factor = 1.f;
		float ani_blend = it->blend_factor;

		if((*it).GetState() != Model_BoneAnimation::Normal)
		if((*it).GetBlendProperties(&blending_factor) == false)
			continue;
		
		for(unsigned int i = 0; i < bones.size(); ++i)
		{
			Storm3D_Bone *bone = bones[i];

			bool interpolate = true;
			if((*it).GetState() == Model_BoneAnimation::Normal)
				interpolate = false;

			if(ani_blend < 0.98f)
			{
				blending_factor *= ani_blend;
				interpolate = true;
			}

			// Animate rotation
			if((*it).GetRotation(i, &rotation))
				bone->AnimateRotation(rotation, blending_factor, interpolate);

			// Animate position
			if(bone->GetParentIndex() != -1)
			if((*it).GetPosition(i, &position))
				bone->AnimatePosition(position, blending_factor, interpolate);
		}
	}

	std::list<Model_BoneBlend>::iterator il;
	for(il = manual_blendings.begin(); il != manual_blendings.end(); ++il)
	{
		float blending_factor = 1.f;

		if((*il).GetState() != Model_BoneAnimation::Normal)
		if((*il).GetBlendProperties(&blending_factor) == false)
			continue;

		//(*il).GetBlendProperties(&blending_factor);

		int bone_index = il->GetIndex();
		if(bone_index < 0)
			continue;

		Storm3D_Bone *bone = bones[bone_index];

		bool interpolate = true;
		if((*il).GetState() == Model_BoneAnimation::Normal)
			interpolate = false;

		// Animate rotation
		//if((*il).GetRotation(bone_index, &rotation))
		//	bone->AnimateRotation(rotation, blending_factor, interpolate);
		// !!
		if((*il).GetRotation(bone_index, &rotation))
			bone->AnimateRelativeRotation(rotation, blending_factor, interpolate);
	}

/*
	for(it = blending_with_animations_out.begin(); it != blending_with_animations_out.end(); ++it)
	{
		if((*it).GetBlendProperties(&blending_factor) == false)
			continue;

		for(int i = 0; i < bones.size(); ++i)
		{
			Storm3D_Bone *bone = bones[i];

			// Animate rotation
			if((*it).GetRotation(i, &rotation))
				bone->AnimateRotation(rotation, blending_factor, true);

			// Animate position
			if(bone->GetParentIndex() != -1)
			if((*it).GetPosition(i, &position))
				bone->AnimatePosition(position, blending_factor, true);
		}
	}
*/
}

//------------------------------------------------------------------
// Storm3D_Model::GetPosition
//------------------------------------------------------------------
VC3 &Storm3D_Model::GetPosition()
{
	return position;
}



//------------------------------------------------------------------
// Storm3D_Model::GetRotation
//------------------------------------------------------------------
QUAT &Storm3D_Model::GetRotation()
{
	return rotation;
}



//------------------------------------------------------------------
// Storm3D_Model::GetScale
//------------------------------------------------------------------
VC3 &Storm3D_Model::GetScale()
{
	return scale;
}


void Storm3D_Model::SetNoCollision(bool no_collision_)
{
	no_collision = no_collision_;
}

const AABB &Storm3D_Model::GetBoundingBox() const
{
	if(!objects.empty() && !box_ok)
	{
		bounding_box.mmin = VC3(100000.f, 100000.f, 100000.f);
		bounding_box.mmax = VC3(-100000.f, -100000.f, -100000.f);

		MAT tm = const_cast<Storm3D_Model *> (this)->GetMX();
		VC3 v[8];

		std::set<IStorm3D_Model_Object *>::const_iterator it = objects.begin();
		for(; it != objects.end(); ++it)
		{
			Storm3D_Model_Object *o = static_cast<Storm3D_Model_Object *> (*it);
			if(!o)
				continue;

			const AABB &object_box = o->GetBoundingBox();

			v[0] = VC3(object_box.mmin.x, object_box.mmin.y, object_box.mmin.z);
			v[1] = VC3(object_box.mmax.x, object_box.mmin.y, object_box.mmin.z);
			v[2] = VC3(object_box.mmin.x, object_box.mmax.y, object_box.mmin.z);
			v[3] = VC3(object_box.mmin.x, object_box.mmin.y, object_box.mmax.z);
			v[4] = VC3(object_box.mmax.x, object_box.mmax.y, object_box.mmin.z);
			v[5] = VC3(object_box.mmin.x, object_box.mmax.y, object_box.mmax.z);
			v[6] = VC3(object_box.mmax.x, object_box.mmin.y, object_box.mmax.z);
			v[7] = VC3(object_box.mmax.x, object_box.mmax.y, object_box.mmax.z);

			for(int i = 0; i < 8; ++i)
			{
				tm.TransformVector(v[i]);

				bounding_box.mmin.x = min(bounding_box.mmin.x, v[i].x);
				bounding_box.mmin.y = min(bounding_box.mmin.y, v[i].y);
				bounding_box.mmin.z = min(bounding_box.mmin.z, v[i].z);
				bounding_box.mmax.x = max(bounding_box.mmax.x, v[i].x);
				bounding_box.mmax.y = max(bounding_box.mmax.y, v[i].y);
				bounding_box.mmax.z = max(bounding_box.mmax.z, v[i].z);
			}
		}

		box_ok = true;
	}

	return bounding_box;
}

const AABB &Storm3D_Model::GetCollisionBoundingBox() const
{
	if(!objects.empty() && !collision_box_ok)
	{
		collision_bounding_box.mmin = VC3(100000.f, 100000.f, 100000.f);
		collision_bounding_box.mmax = VC3(-100000.f, -100000.f, -100000.f);

		MAT tm = const_cast<Storm3D_Model *> (this)->GetMX();
		VC3 v[8];

		bool thereWasAtLeastOneCollisionObject = false;

		std::set<IStorm3D_Model_Object *>::const_iterator it = objects.begin();
		for(; it != objects.end(); ++it)
		{
			Storm3D_Model_Object *o = static_cast<Storm3D_Model_Object *> (*it);
			if(!o)
				continue;

			// this does not work???
			if (o->GetNoCollision())
				continue;

			// maybe this does???
			if (o->GetName() != NULL 
				&& strstr(o->GetName(), "_NoCollision") != NULL)
				continue;

			thereWasAtLeastOneCollisionObject = true;

			const AABB &object_box = o->GetBoundingBox();

			v[0] = VC3(object_box.mmin.x, object_box.mmin.y, object_box.mmin.z);
			v[1] = VC3(object_box.mmax.x, object_box.mmin.y, object_box.mmin.z);
			v[2] = VC3(object_box.mmin.x, object_box.mmax.y, object_box.mmin.z);
			v[3] = VC3(object_box.mmin.x, object_box.mmin.y, object_box.mmax.z);
			v[4] = VC3(object_box.mmax.x, object_box.mmax.y, object_box.mmin.z);
			v[5] = VC3(object_box.mmin.x, object_box.mmax.y, object_box.mmax.z);
			v[6] = VC3(object_box.mmax.x, object_box.mmin.y, object_box.mmax.z);
			v[7] = VC3(object_box.mmax.x, object_box.mmax.y, object_box.mmax.z);

			for(int i = 0; i < 8; ++i)
			{
				tm.TransformVector(v[i]);

				collision_bounding_box.mmin.x = min(collision_bounding_box.mmin.x, v[i].x);
				collision_bounding_box.mmin.y = min(collision_bounding_box.mmin.y, v[i].y);
				collision_bounding_box.mmin.z = min(collision_bounding_box.mmin.z, v[i].z);
				collision_bounding_box.mmax.x = max(collision_bounding_box.mmax.x, v[i].x);
				collision_bounding_box.mmax.y = max(collision_bounding_box.mmax.y, v[i].y);
				collision_bounding_box.mmax.z = max(collision_bounding_box.mmax.z, v[i].z);
			}
		}

		if (!thereWasAtLeastOneCollisionObject)
		{
			// WARNING: this makes a zero volume bounding box! 
			// I assume that some crappy code may broke when encountering such.
			// could be fixed by making some silly "really small box" here - which would be incorrect though.
			collision_bounding_box.mmin = VC3(0.f, 0.f, 0.f);
			collision_bounding_box.mmax = VC3(0.f, 0.f, 0.f);
		}

		collision_box_ok = true;
	}

	return collision_bounding_box;
}

const AABB &Storm3D_Model::GetPhysicsBoundingBox() const
{
	// FIXME: this should be different from GetCollisionBoundingBox, as it should take into account the
	// "_NoPhysics" tagged objects too.
	// but for now, collision bounding box shall be good enough.
	return GetCollisionBoundingBox();
}

void Storm3D_Model::RemoveCollision(Storm3D_Model_Object *object)
{
	//assert(collision_objects.find(object) != collision_objects.end());
	collision_objects.erase(object);
}

void Storm3D_Model::SetCollision(Storm3D_Model_Object *object)
{
	//assert(collision_objects.find(object) == collision_objects.end());
	if(collision_objects.find(object) == collision_objects.end())
		collision_objects.insert(object);
}

// Hacky things...
void Storm3D_Model::MakeSkyModel()
{
	skyModel = true;
	cast_shadows = false;

	std::set<IStorm3D_Model_Object *>::const_iterator it = objects.begin();
	for(; it != objects.end(); ++it)
	{
		Storm3D_Model_Object *o = static_cast<Storm3D_Model_Object *> (*it);
		if(!o)
			continue;

		Storm3D_Mesh *mesh = static_cast<Storm3D_Mesh *> (o->GetMesh());
		if(!mesh)
			continue;

		Storm3D_Material *material = static_cast<Storm3D_Material *> (mesh->GetMaterial());
		if(!material)
			continue;

		int alphaType = material->GetAlphaType();

		bool hasAlpha = false;
		if(alphaType != IStorm3D_Material::ATYPE_NONE && alphaType != IStorm3D_Material::ATYPE_USE_ALPHATEST)
			hasAlpha = true;

		if (hasAlpha)
		{
			o->EnableRenderPass(RENDER_PASS_BIT_EARLY_ALPHA);
		}
	}
}

int Storm3D_Model::GetAnimationTime(IStorm3D_BoneAnimation *animation)
{
	std::list<Model_BoneAnimation>::iterator it;
	for(it = normal_animations.begin(); it != normal_animations.end(); ++it)
	{
		if((*it).GetAnimation() == animation)
		{
			return (*it).GetAnimationTime();
		}
	}
	return 0;
}

void Storm3D_Model::SetAnimationPaused(bool paused)
{
	animation_paused = paused;
}
