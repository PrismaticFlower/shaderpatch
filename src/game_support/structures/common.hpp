#pragma once

namespace sp::game_support::structures {

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

struct PblVector2 {
   float x;
   float y;
};

struct PblVector3 {
   float x;
   float y;
   float z;
};

struct PblVector4 {
   float x;
   float y;
   float z;
   float w;
};

struct PblMatrix {
   PblVector4 rows[4];
};

}