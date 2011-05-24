sp             := $(sp).x
dirstack_$(sp) := $(d)
d              := $(dir)


# process subdirectories
SUBDIRS:= \
		 # empty line

DIRS:=$(addprefix $(d)/,$(SUBDIRS))

$(eval $(foreach directory, $(DIRS), $(call directory-module,$(directory)) ))


# files in this directory
FILES:= \
		Clipper.cpp \
		IStorm3D.cpp \
		Storm3D.cpp \
		Storm3D_Adapter.cpp \
		Storm3D_Bone.cpp \
		Storm3D_Camera.cpp \
		Storm3D_Face.cpp \
		Storm3D_Font.cpp \
		Storm3D_Helper_AInterface.cpp \
		Storm3D_Helper_Animation.cpp \
		Storm3D_Helpers.cpp \
		Storm3D_KeyFrames.cpp \
		Storm3D_Line.cpp \
		Storm3D_Material.cpp \
		Storm3D_Material_TextureLayer.cpp \
		Storm3D_Mesh.cpp \
		Storm3D_Mesh_Animation.cpp \
		Storm3D_Mesh_CollisionTable.cpp \
		Storm3D_Model.cpp \
		Storm3D_Model_Object.cpp \
		Storm3D_Model_Object_Animation.cpp \
		Storm3D_ParticleSystem.cpp \
		Storm3D_ParticleSystem_PMH.cpp \
		Storm3D_PicList.cpp \
		Storm3D_ProceduralManager.cpp \
		Storm3D_Scene.cpp \
		Storm3D_Scene_PicList.cpp \
		Storm3D_Scene_PicList_Font.cpp \
		Storm3D_Scene_PicList_Texture.cpp \
		Storm3D_ShaderManager.cpp \
		Storm3D_SurfaceInfo.cpp \
		Storm3D_Terrain.cpp \
		Storm3D_Texture_Video.cpp \
		Storm3D_Vertex.cpp \
		Storm3d_Texture.cpp \
		keyb.cpp \
		render.cpp \
		storm3d_fakespotlight.cpp \
		storm3d_resourcemanager.cpp \
		storm3d_spotlight.cpp \
		storm3d_spotlight_shared.cpp \
		storm3d_string_util.cpp \
		storm3d_terrain_decalsystem.cpp \
		storm3d_terrain_groups.cpp \
		storm3d_terrain_heightmap.cpp \
		storm3d_terrain_lightmanager.cpp \
		storm3d_terrain_lod.cpp \
		storm3d_terrain_models.cpp \
		storm3d_terrain_renderer.cpp \
		storm3d_terrain_utils.cpp \
		storm3d_video_player.cpp \
		storm3d_videostreamer.cpp \
		treader.cpp \
		# empty line


SRC_$(d):=$(addprefix $(d)/,$(FILES))


ALLDIRS+=$(d)


claw_opengl_MODULES:=claw_proto container convert editor filesystem game mingw net ogui particle_editor2 physics sound storm system ui util opengl
claw_opengl_MODULES+=storm/stormgl
claw_opengl_SRC:=


ifeq ($(OPENGL),y)
PROGRAMS+=claw_opengl
endif


-include $(foreach FILE,$(FILES),$(d)/$(FILE:.cpp=.d))


d  := $(dirstack_$(sp))
sp := $(basename $(sp))
