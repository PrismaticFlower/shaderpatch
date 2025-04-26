#pragma once

#include "common.hpp"

#include <d3d9.h>

namespace sp::game_support::structures {

enum RedLightFlags : uint {
   IsStatic = 256,
   CastShadow = 512,
   Active = 1024,
   CastSpecular = 2048
};

enum RedLightType : uint { Ambient = 0, Directional = 1, Omni = 2, Spot = 3 };

enum RedLightTextureAddressing : uint { Wrap = 0, Clamp = 1 };

struct RedLight;

struct RedLightListNode {
   RedLightListNode* root;
   RedLightListNode* previous;
   RedLightListNode* next;
   RedLight* light;
};

struct RedColor {
   float r;
   float g;
   float b;
   float a;
};

struct RedLight_vftable {
   void* RedLight_Destructor;
   void* Activate;
   void* Deactivate;
   void* vfunction4;
   void* vfunction5;
   void* vfunction6;
   void* vfunction7;
   void* vfunction8;
};

struct RedLight_data {
   RedLightFlags flags;
   uint nameHash;
   RedLightType type;
   uint textureHash;
   RedLightTextureAddressing textureAddressing;
   undefined4 offset_0x14;
   undefined field6_0x18;
   undefined field7_0x19;
   undefined field8_0x1a;
   undefined field9_0x1b;
   RedColor color;
   RedLightListNode listNode;
   int offset_0x3c;
   int offset_0x40;
   int offset_0x44;
   undefined4 offset_0x48;
};

struct RedDirectionalLight_data {
   PblVector3 direction;
   uint regionNameHash;
   undefined4 offset_0x10;
   D3DLIGHT9 d3dLight;
   undefined field4_0x7c;
   undefined field5_0x7d;
   undefined field6_0x7e;
   undefined field7_0x7f;
   PblMatrix textureMatrix;
   bool textureMatrixDirty;
   undefined field10_0xc1;
   undefined field11_0xc2;
   undefined field12_0xc3;
   PblVector2 textureOffset;
   PblVector2 textureTile;
};

struct RedDirectionalLight { /* class RedDirectionalLight : RedLight */
   RedLight_vftable* vftablePtr;
   RedLight_data RedLight_data;
   RedDirectionalLight_data RedDirectionalLight_data;
};

struct RedLight { /* class RedLight */
   RedLight_vftable* vftablePtr;
   RedLight_data RedLight_data;
};

struct RedSpotLight_data {
   float innerAngleTan;
   float outerAngleTan;
   float innerAngleCos;
   float outerAngleCos;
   float falloff;
   float range;
   PblVector3 direction;
   PblVector3 position;
   PblMatrix textureMatrix;
   bool textureMatrixDirty;
   bool bidirectional;
   undefined field11_0x72;
   undefined field12_0x73;
   D3DLIGHT9 d3dLight;
   undefined field14_0xdc;
   undefined field15_0xdd;
   undefined field16_0xde;
   undefined field17_0xdf;
   float invRangeSq;
   float coneRadius;
};

struct RedSpotLight { /* class RedSpotLight : RedLight */
   RedLight_vftable* vftablePtr;
   RedLight_data RedLight_data;
   RedSpotLight_data RedSpotLight_data;
};

struct RedOmniLight_data {
   PblVector3 position;
   float range;
   D3DLIGHT9 d3dLight;
   float invRange;
   float invRangeSq;
   PblMatrix textureMatrix;
   bool textureMatrixDirty;
   undefined field7_0xc1;
   undefined field8_0xc2;
   undefined field9_0xc3;
};

struct RedOmniLight { /* class RedOmniLight : RedLight */
   RedLight_vftable* vftablePtr;
   RedLight_data RedLight_data;
   RedOmniLight_data RedOmniLight_data;
};

struct RedLightList {
   RedLightListNode node;
   uint count;
};

}