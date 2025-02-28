#pragma once

namespace sp::game_support::structures {

// Partially reverse engineered game structures. Exported from Ghidra and partially cleaned up. Should probably
// be tidied up properly at some point but this is good enough and these aren't going to be commonly used structures.

typedef unsigned char undefined;
typedef unsigned char undefined1;
typedef unsigned short undefined2;
typedef unsigned int undefined3;
typedef unsigned int undefined4;

typedef unsigned char byte;
typedef unsigned int dword;

typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned long long undefined8;
typedef unsigned short ushort;
typedef unsigned short wchar16;
typedef unsigned short word;

template<typename class_Entity, typename class_EntityClass, typename struct_EntityDesc>
struct Factory;

template<typename T, typename U>
struct PblHeap {
   struct Value {
      U first;
      T second;
   };

   int size;
   int capacity;
   Value* data;
   bool unknownFlag;
   undefined field4_0xd;
   undefined field5_0xe;
   undefined field6_0xf;
};

typedef struct LeafPatchClass_vftable_for_Thread LeafPatchClass_vftable_for_Thread,
   *PLeafPatchClass_vftable_for_Thread;

typedef struct Thread_data Thread_data, *PThread_data;

typedef struct LeafPatchClass_vftable_for_EntityClass LeafPatchClass_vftable_for_EntityClass,
   *PLeafPatchClass_vftable_for_EntityClass;

typedef struct EntityClass_data EntityClass_data, *PEntityClass_data;

typedef struct LeafPatchClass_data LeafPatchClass_data, *PLeafPatchClass_data;

typedef struct PblVector3 PblVector3, *PPblVector3;

typedef enum LeafPatchType {
   LeafPatchType_Default = 0,
   LeafPatchType_Unknown = 1,
   LeafPatchType_Box = 2,
   LeafPatchType_Vine = 3
} LeafPatchType;

typedef struct LeafPatchParticle LeafPatchParticle, *PLeafPatchParticle;

typedef struct LeafPatchRenderable LeafPatchRenderable, *PLeafPatchRenderable;

typedef struct LeafPatchClassList LeafPatchClassList, *PLeafPatchClassList;

typedef struct PblVector4 PblVector4, *PPblVector4;

typedef struct LeafPatchRenderable_vftable LeafPatchRenderable_vftable,
   *PLeafPatchRenderable_vftable;

typedef struct Renderable_data Renderable_data, *PRenderable_data;

typedef struct LeafPatchRenderable_data LeafPatchRenderable_data,
   *PLeafPatchRenderable_data;

typedef struct pcRedPrimitive pcRedPrimitive, *PpcRedPrimitive;

typedef struct pcRedShader pcRedShader, *PpcRedShader;

typedef struct pcRedIndexBuffer pcRedIndexBuffer, *PpcRedIndexBuffer;

typedef struct pcRedVertexBuffer pcRedVertexBuffer, *PpcRedVertexBuffer;

typedef struct pcRedPrimitive_vftable pcRedPrimitive_vftable, *PpcRedPrimitive_vftable;

typedef struct PblRef_data PblRef_data, *PPblRef_data;

typedef struct pcRedPrimitive_data pcRedPrimitive_data, *PpcRedPrimitive_data;

typedef struct pcRedShader_vftable pcRedShader_vftable, *PpcRedShader_vftable;

typedef struct pcRedShader_data pcRedShader_data, *PpcRedShader_data;

typedef struct pcRedIndexBuffer_vftable pcRedIndexBuffer_vftable,
   *PpcRedIndexBuffer_vftable;

typedef struct pcRedIndexBuffer_data pcRedIndexBuffer_data, *PpcRedIndexBuffer_data;

typedef struct pcRedVertexBuffer_vftable pcRedVertexBuffer_vftable,
   *PpcRedVertexBuffer_vftable;

typedef struct pcRedVertexBuffer_data pcRedVertexBuffer_data, *PpcRedVertexBuffer_data;

typedef enum D3DPRIMITIVETYPE {
   D3DPT_POINTLIST = 1,
   D3DPT_LINELIST = 2,
   D3DPT_LINESTRIP = 3,
   D3DPT_TRIANGLELIST = 4,
   D3DPT_TRIANGLESTRIP = 5,
   D3DPT_TRIANGLEFAN = 6,
   D3DPT_FORCE_DWORD = 2147483647
} D3DPRIMITIVETYPE;

typedef struct pcRedVertexFormat pcRedVertexFormat, *PpcRedVertexFormat;

typedef struct AABB AABB, *PAABB;

typedef struct pcRedVertexFormat_vftable pcRedVertexFormat_vftable,
   *PpcRedVertexFormat_vftable;

typedef struct pcRedVertexFormat_data pcRedVertexFormat_data, *PpcRedVertexFormat_data;

struct pcRedVertexBuffer_data {
   int offset_0x0;
   undefined4 offset_0x4;
   int offset_0x8;
   undefined4 offset_0xc;
   undefined4 offset_0x10;
   char offset_0x14;
   undefined1 offset_0x15;
   undefined1 offset_0x16;
   undefined field8_0x17;
   undefined field9_0x18;
   undefined field10_0x19;
   undefined field11_0x1a;
   undefined field12_0x1b;
   undefined field13_0x1c;
   undefined field14_0x1d;
   undefined field15_0x1e;
   undefined field16_0x1f;
   undefined4 offset_0x20;
   uint offset_0x24;
   int offset_0x28;
   undefined4 offset_0x2c;
   undefined field21_0x30;
   undefined field22_0x31;
   undefined field23_0x32;
   undefined field24_0x33;
   undefined4 offset_0x34;
   undefined4 offset_0x38;
   undefined4 offset_0x3c;
   undefined4 offset_0x40;
};

struct LeafPatchClass_vftable_for_EntityClass {
   undefined4* (*vfunction1_for_EntityClass)(byte);
   undefined4* (*vfunction2_for_EntityClass)(void);
   int* (*vfunction3_for_EntityClass)(int);
   undefined4 (*vfunction4)(void);
   undefined4 (*vfunction5)(void);
   char* (*vfunction6)(void);
   undefined (*vfunction7_for_EntityClass)(float, char*);
   undefined (*vfunction8)(void);
};

struct PblVector3 {
   float x;
   float y;
   float z;
};

struct LeafPatchRenderable_data {
   float offset_0x0;
   struct pcRedPrimitive* primitive;
   struct pcRedShader* m_shader0;
   struct pcRedPrimitive* primitive2;
   struct pcRedShader* shader1;
   struct pcRedIndexBuffer* m_indexBuffer;
   struct pcRedIndexBuffer* m_indexBuffer2;
   struct LeafPatchClass* m_leafPatchClass;
   struct PblHeap<int, float>* m_heap;
   undefined field9_0x24;
   undefined field10_0x25;
   undefined field11_0x26;
   undefined field12_0x27;
   uint m_indexBuffer2Offset;
   uint m_indexBufferOffset;
   struct pcRedVertexBuffer* m_vertexBuffer;
   void* vertexUpload;
   struct PblVector3 cameraYAxis;
   struct PblVector3 offsetsXZ[10];
   struct PblVector3 cameraZAxis;
   uint m_numPartsIs4;
   undefined1 offset_0xcc;
   undefined field22_0xcd;
   undefined field23_0xce;
   undefined field24_0xcf;
};

struct PblRef_data {
   ushort offset_0x0;
   undefined field1_0x2;
   undefined field2_0x3;
};

struct pcRedShader_data {
   undefined field0_0x0;
   undefined field1_0x1;
   undefined field2_0x2;
   undefined field3_0x3;
   undefined field4_0x4;
   undefined field5_0x5;
   undefined field6_0x6;
   undefined field7_0x7;
   undefined field8_0x8;
   undefined field9_0x9;
   undefined field10_0xa;
   undefined field11_0xb;
   undefined field12_0xc;
   undefined field13_0xd;
   undefined field14_0xe;
   undefined field15_0xf;
   undefined4 offset_0x10;
};

struct pcRedShader {
   struct pcRedShader_vftable* vftablePtr;
   struct PblRef_data PblRef_data;
   struct pcRedShader_data pcRedShader_data;
};

struct Renderable_data {
   undefined4 offset_0x0;
   undefined4 offset_0x4;
   undefined4 offset_0x8;
   undefined4 offset_0xc;
   undefined4 offset_0x10;
   undefined4 offset_0x14;
};

struct LeafPatchRenderable {
   struct LeafPatchRenderable_vftable* vftablePtr;
   struct Renderable_data Renderable_data;
   struct LeafPatchRenderable_data LeafPatchRenderable_data;
};

struct pcRedVertexBuffer {
   struct pcRedVertexBuffer_vftable* vftablePtr;
   struct PblRef_data PblRef_data;
   struct pcRedVertexBuffer_data pcRedVertexBuffer_data;
};

struct LeafPatchClass_vftable_for_Thread {
   int* (*vfunction1_for_Thread)(byte);
   undefined4 (*vfunction2_for_Thread)(float);
   undefined (*vfunction3)(int, int);
   undefined (*vfunction4)(void);
   bool (*vfunction5)(void);
};

struct pcRedIndexBuffer_vftable {
   undefined (*vfunction1)(void);
   undefined4* (*vfunction2)(byte);
};

struct pcRedIndexBuffer_data {
   undefined4 offset_0x0;
   int offset_0x4;
   char offset_0x8;
   undefined1 offset_0x9;
   undefined field4_0xa;
   undefined field5_0xb;
   undefined4 offset_0xc;
   undefined1 offset_0x10;
   undefined field8_0x11;
   undefined field9_0x12;
   undefined field10_0x13;
   undefined4 offset_0x14;
   undefined field12_0x18;
   undefined field13_0x19;
   undefined field14_0x1a;
   undefined field15_0x1b;
   undefined4 offset_0x1c;
   uint offset_0x20;
   int offset_0x24;
   undefined4 offset_0x28;
   undefined4 offset_0x2c;
   undefined4 offset_0x30;
   undefined4 offset_0x34;
   undefined4 offset_0x38;
   undefined4 offset_0x3c;
};

struct pcRedPrimitive_vftable {
   undefined (*vfunction1)(void);
   undefined4* (*vfunction2)(byte);
};

struct PblVector4 {
   float x;
   float y;
   float z;
   float w;
};

struct LeafPatchParticle {
   struct PblVector3 position;
   float wiggle;
   float size;
   float wiggleAccum;
   uchar variation;
   undefined field5_0x19;
   undefined field6_0x1a;
   undefined field7_0x1b;
   struct PblVector4 color;
};

struct EntityClass_data {
   undefined4 offset_0x0;
   uint m_iconTextureHash;
   uint m_healthTextureHash;
   uint m_HUDModelHash;
   float m_mapScale;
   float m_mapSpeedMin;
   float m_mapSpeedMax;
   float m_mapViewMin;
   float offset_0x20;
   uint m_mapTextureHash;
   float m_mapTargetFadeRate;
   float m_mapRangeOverall;
   float m_mapRangeShooting;
   float m_mapRangeViewCone;
   float m_mapViewConeAngle;
   bool m_removeFromMapWhenDead;
   undefined field16_0x3d;
   undefined field17_0x3e;
   undefined field18_0x3f;
};

struct Thread_data {
   int offset_0x0;
   int offset_0x4;
   int offset_0x8;
   int offset_0xc;
   undefined4 offset_0x10;
};

struct AABB {
   struct PblVector3 min;
   struct PblVector3 max;
};

struct pcRedPrimitive_data {
   struct pcRedIndexBuffer* indexBuffer;
   struct pcRedVertexBuffer* vertexBuffer;
   enum D3DPRIMITIVETYPE primitiveType;
   undefined4 vertexOffset;
   uint indexBufferOffset;
   uint primitiveCount;
   uint vertexCount;
   uint offset_0x1c;
   struct pcRedVertexFormat* redVertexFormat;
   struct AABB bbox;
};

struct pcRedVertexBuffer_vftable {
   undefined (*vfunction1)(void);
   undefined4* (*vfunction2)(byte);
};

struct Factory_data {
   undefined** offset_0x0;
   undefined** offset_0x4;
   undefined* offset_0x8;
   void* offset_0xc;
   undefined4 offset_0x10;
   undefined4 offset_0x14;
   int offset_0x18;
};

struct LeafPatchClassList {
   struct LeafPatchClassList* first;
   struct LeafPatchClassList* next;
   undefined4 third;
   struct LeafPatchClass* leafPatchClass;
};

struct LeafPatchClass_data {
   float boundsRadius;
   struct PblVector3 bboxSize;
   struct PblVector3 bboxCentre;
   uchar m_maxFallingLeaves;
   uchar m_maxScatterBirds;
   undefined field5_0x1e;
   undefined field6_0x1f;
   float m_radius;
   float m_heightScale;
   float m_height;
   int m_seed;
   enum LeafPatchType m_type;
   struct LeafPatchParticle* m_particles;
   int m_numParticles;
   struct PblVector3 m_offset;
   float m_minSize;
   float m_maxSize;
   float m_alpha;
   float m_maxDistance;
   float m_coneHeight;
   struct PblVector3 m_boxSize;
   uint m_textureHash;
   float m_darknessMin;
   float m_darknessMax;
   float field24_0x74;
   int m_numParts;
   float m_vineX;
   float m_vineY;
   int m_vineLengthX;
   int m_vineLengthY;
   float m_vineSpread;
   float m_wiggleSpeed;
   float m_wiggleAmount;
   int m_numVisible;
   float m_numVisibleFrac;
   struct LeafPatchRenderable* m_renderable;
   struct LeafPatchClassList m_classList;
};

struct LeafPatchClass {
   struct LeafPatchClass_vftable_for_Thread* thread_vftablePtr;
   struct Thread_data Thread_data;
   struct LeafPatchClass_vftable_for_EntityClass* entityClass_vftablePtr;
   struct Factory_data Factory_data;
   struct EntityClass_data EntityClass_data;
   struct LeafPatchClass_data LeafPatchClass_data;
};

struct LeafPatchClass_DebugBuild {
   struct LeafPatchClass_vftable_for_Thread* thread_vftablePtr;
   struct Thread_data Thread_data;
   undefined4 extraThread_data[8];
   struct LeafPatchClass_vftable_for_EntityClass* entityClass_vftablePtr;
   struct Factory_data Factory_data;
   struct EntityClass_data EntityClass_data;
   struct LeafPatchClass_data LeafPatchClass_data;
};

struct pcRedShader_vftable {
   undefined (*vfunction1)(void);
   undefined4* (*vfunction2)(byte);
   undefined4 (*vfunction3)(void);
   undefined4 (*vfunction4)(int);
   undefined4 (*vfunction5)(void);
   undefined4 (*vfunction6)(void);
   undefined (*vfunction7)(void);
   undefined (*vfunction8)(void*);
   undefined4 (*vfunction9)(void);
   undefined (*vfunction10)(void);
   undefined (*vfunction11)(void);
   undefined (*vfunction12)(void);
   undefined4 (*vfunction13)(undefined4*);
   undefined4 (*vfunction14)(void);
   undefined4* (*vfunction15)(uint, int*, int, undefined4, undefined4*, undefined4*);
   undefined (*vfunction16)(void*);
   undefined (*vfunction17)(void);
   undefined* (*vfunction18)(void);
   undefined (*vfunction19)(int*);
};

struct pcRedVertexFormat_data {
   undefined4 offset_0x0;
   undefined4 offset_0x4;
   undefined4 offset_0x8;
   undefined4 offset_0xc;
   undefined4 offset_0x10;
   undefined4 offset_0x14;
   undefined4 offset_0x18;
   undefined4 offset_0x1c;
};

struct pcRedVertexFormat {
   struct pcRedVertexFormat_vftable* vftablePtr;
   struct PblRef_data PblRef_data;
   struct pcRedVertexFormat_data pcRedVertexFormat_data;
};

struct pcRedPrimitive {
   struct pcRedPrimitive_vftable* vftablePtr;
   struct PblRef_data PblRef_data;
   struct pcRedPrimitive_data pcRedPrimitive_data;
};

struct LeafPatchRenderable_vftable {
   undefined4* (*vfunction1)(byte);
   undefined (*vfunction2)(int);
   undefined (*vfunction3)(void);
   undefined (*vfunction4)(void);
   undefined (*vfunction5)(uint);
   undefined (*vfunction6)(void);
   undefined (*vfunction7)(void);
};

struct pcRedVertexFormat_vftable {
   undefined (*vfunction1)(void);
   undefined4* (*vfunction2)(byte);
};

struct pcRedIndexBuffer {
   struct pcRedIndexBuffer_vftable* vftablePtr;
   struct PblRef_data PblRef_data;
   struct pcRedIndexBuffer_data pcRedIndexBuffer_data;
};

typedef struct LeafPatchClass* LeafPatchClassEC;

typedef struct LeafPatchClass* LeafPatchClassEntityClass;

typedef struct LeafSpawner_vftable LeafSpawner_vftable, *PLeafSpawner_vftable;

struct LeafSpawner_vftable {
   undefined (*vfunction1)(int);
};

typedef struct LeafPatch LeafPatch, *PLeafPatch;

typedef struct RedSceneObject RedSceneObject, *PRedSceneObject;

typedef struct Entity_vftable Entity_vftable, *PEntity_vftable;

typedef struct CollisionObject CollisionObject, *PCollisionObject;

typedef struct PblMatrix PblMatrix, *PPblMatrix;

typedef struct LeafPatchListNode LeafPatchListNode, *PLeafPatchListNode;

typedef struct RedSceneObject_vftable RedSceneObject_vftable, *PRedSceneObject_vftable;

typedef struct DynDisplayable_data DynDisplayable_data, *PDynDisplayable_data;

typedef struct RedSceneObject_data RedSceneObject_data, *PRedSceneObject_data;

typedef struct CollisionObject_vftable CollisionObject_vftable,
   *PCollisionObject_vftable;

typedef struct TreeGridObject TreeGridObject, *PTreeGridObject;

typedef struct FoleyFXCollider FoleyFXCollider, *PFoleyFXCollider;

typedef struct FoleyFXCollidee FoleyFXCollidee, *PFoleyFXCollidee;

typedef struct CollisionObject_data CollisionObject_data, *PCollisionObject_data;

struct RedSceneObject_data {
   undefined4 offset_0x0;
   undefined4 offset_0x4;
   undefined4 offset_0x8;
   struct PblVector3 m_renderPosition;
   float m_renderRadius;
   undefined4 offset_0x1c;
   undefined4 offset_0x20;
   bool m_activated;
   undefined field8_0x25;
   undefined field9_0x26;
   undefined field10_0x27;
   float offset_0x28;
   undefined4 offset_0x2c;
};

struct CollisionObject_data {
   undefined4 offset_0x0;
   undefined4 offset_0x4;
   undefined4 offset_0x8;
   undefined4 offset_0xc;
   undefined4 offset_0x10;
   undefined4 offset_0x14;
   undefined4 offset_0x18;
   undefined field7_0x1c;
   undefined field8_0x1d;
   undefined field9_0x1e;
   undefined field10_0x1f;
   undefined field11_0x20;
   undefined field12_0x21;
   undefined field13_0x22;
   undefined field14_0x23;
   undefined field15_0x24;
   undefined field16_0x25;
   undefined field17_0x26;
   undefined field18_0x27;
   undefined field19_0x28;
   undefined field20_0x29;
   undefined field21_0x2a;
   undefined field22_0x2b;
   undefined field23_0x2c;
   undefined field24_0x2d;
   undefined field25_0x2e;
   undefined field26_0x2f;
   undefined field27_0x30;
   undefined field28_0x31;
   undefined field29_0x32;
   undefined field30_0x33;
   byte offset_0x34;
   undefined field32_0x35;
   undefined field33_0x36;
   undefined field34_0x37;
   undefined8 offset_0x38;
   undefined4 offset_0x40;
   undefined4 offset_0x44;
   undefined4 offset_0x48;
   undefined4 offset_0x4c;
};

struct TreeGridObject {
   undefined field0_0x0;
   undefined field1_0x1;
   undefined field2_0x2;
   undefined field3_0x3;
};

struct FoleyFXCollider {
   undefined field0_0x0;
   undefined field1_0x1;
   undefined field2_0x2;
   undefined field3_0x3;
};

struct FoleyFXCollidee {
   undefined field0_0x0;
   undefined field1_0x1;
   undefined field2_0x2;
   undefined field3_0x3;
};

struct CollisionObject {
   struct CollisionObject_vftable* vftablePtr;
   struct TreeGridObject TreeGridObject;
   undefined field2_0x8;
   undefined field3_0x9;
   undefined field4_0xa;
   undefined field5_0xb;
   struct PblVector3 position;
   float radiusX;
   float radiusY;
   undefined field9_0x20;
   undefined field10_0x21;
   undefined field11_0x22;
   undefined field12_0x23;
   undefined field13_0x24;
   undefined field14_0x25;
   undefined field15_0x26;
   undefined field16_0x27;
   struct FoleyFXCollider FoleyFXCollider;
   undefined field18_0x2c;
   undefined field19_0x2d;
   undefined field20_0x2e;
   undefined field21_0x2f;
   undefined field22_0x30;
   undefined field23_0x31;
   undefined field24_0x32;
   undefined field25_0x33;
   struct FoleyFXCollidee FoleyFXCollidee;
   struct CollisionObject_data CollisionObject_data;
};

struct PblMatrix {
   struct PblVector4 rows[4];
};

struct DynDisplayable_data {
   undefined4 offset_0x0;
   undefined4 offset_0x4;
   undefined4 offset_0x8;
   undefined4 offset_0xc;
   undefined4 offset_0x10;
   undefined4 offset_0x14;
   undefined4 offset_0x18;
   undefined4 offset_0x1c;
};

struct RedSceneObject {
   struct RedSceneObject_vftable* vftablePtr;
   struct DynDisplayable_data DynDisplayable_data;
   struct RedSceneObject_data RedSceneObject_data;
};

struct LeafPatchListNode {
   struct LeafPatchListNode* root;
   struct LeafPatchListNode* previous;
   struct LeafPatchListNode* next;
   struct LeafPatch* object;
};

struct CollisionObject_vftable {
   undefined4* (*vfunction1)(byte);
   int (*vfunction2)(int);
   undefined (*vfunction3)(void);
   undefined (*vfunction4)(void);
   undefined (*vfunction5)(void);
   float* (*vfunction6)(void);
   undefined4 (*vfunction7)(int, int*, int);
   float (*vfunction8)(int*, undefined4, undefined4, float, undefined8*, int*,
                       char, undefined4);
   uint (*vfunction9)(undefined4, undefined4, undefined4);
   undefined (*vfunction10)(int*, undefined4, int);
   undefined (*vfunction11)(void);
   undefined4 (*vfunction12)(void);
   undefined4 (*vfunction13)(void);
   undefined4 (*vfunction14)(void);
   undefined4 (*vfunction15)(void);
   undefined (*vfunction16)(void);
   int (*vfunction17)(int);
   undefined4 (*vfunction18)(void);
   undefined (*vfunction19)(void);
   undefined (*vfunction20)(void);
};

struct LeafPatch {
   struct RedSceneObject redSceneObject;
   undefined field1_0x54;
   undefined field2_0x55;
   undefined field3_0x56;
   undefined field4_0x57;
   struct Entity_vftable* entityVtbl;
   struct LeafSpawner_vftable* leafSpawnerVtbl;
   struct CollisionObject collisionObject;
   undefined field8_0xe8;
   undefined field9_0xe9;
   undefined field10_0xea;
   undefined field11_0xeb;
   undefined field12_0xec;
   undefined field13_0xed;
   undefined field14_0xee;
   undefined field15_0xef;
   struct PblMatrix transform;
   struct LeafPatchClass* leafPatchClass;
   undefined field18_0x134;
   undefined field19_0x135;
   undefined field20_0x136;
   undefined field21_0x137;
   float field22_0x138;
   float field23_0x13c;
   float field24_0x140;
   float field25_0x144;
   undefined field26_0x148;
   undefined field27_0x149;
   undefined field28_0x14a;
   undefined field29_0x14b;
   float m_maxDistance;
   struct LeafPatchListNode m_listNode;
};

struct Entity_vftable {
   undefined4 (*vfunction1)(void);
   undefined4 (*vfunction2)(void);
   char* (*vfunction3)(void);
   undefined4* (*vfunction4)(byte);
   undefined (*vfunction5)(void);
   undefined (*vfunction6)(void);
   undefined4 (*vfunction7)(void);
   undefined4 (*vfunction8)(void);
   undefined4 (*vfunction9)(void);
   undefined4 (*vfunction10)(void);
   undefined4 (*vfunction11)(void);
   undefined4 (*vfunction12)(void);
};

struct RedSceneObject_vftable {
   undefined (*vfunction1)(void);
   undefined (*vfunction2)(undefined4);
   bool (*vfunction3)(int);
   undefined (*vfunction4)(undefined4);
   bool (*vfunction5)(int);
   undefined (*vfunction6)(void);
   undefined (*vfunction7)(undefined4*);
   uint (*vfunction8)(void);
   undefined (*vfunction9)(char);
   uint (*vfunction10)(void);
   undefined (*vfunction11)(char);
   uint (*vfunction12)(void);
   undefined (*vfunction13)(char);
   undefined4 (*vfunction14)(void);
   undefined (*vfunction15)(int);
   undefined (*vfunction16)(void);
   undefined4* (*vfunction17)(byte);
   undefined (*vfunction18)(void);
   undefined (*vfunction19)(void);
   undefined (*vfunction20)(undefined4, undefined4, uint);
   undefined8* (*vfunction21)(void);
   undefined4 (*vfunction22)(void);
   undefined (*vfunction23)(void);
   undefined (*vfunction24)(float*);
   undefined (*vfunction25)(float);
};

}