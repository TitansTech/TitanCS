// stdafx.h: archivo de inclusión de los archivos de inclusión estándar del sistema
// o archivos de inclusión específicos de un proyecto utilizados frecuentemente,
// pero rara vez modificados
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Excluir material rara vez utilizado de encabezados de Windows
// Archivos de encabezado de Windows:

// Windows Header Files:
#include <windows.h>
#include <winsock2.h>
#include <commctrl.h>

#pragma comment ( lib , "comctl32.lib" )
#pragma comment ( lib , "WS2_32.lib" )

#define CS_CURRENT_VERSION "Ver 1.90.00 MAR 27 2012"

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <time.h>
#include <stdio.h>
#include <vector>
#include <map>
#include <queue>
#include "ReadScript.h"

#define SET_NUMBERH(x) ( (BYTE)((DWORD)(x)>>(DWORD)8) )
#define SET_NUMBERL(x) ( (BYTE)((DWORD)(x) & 0xFF) )
#define SET_NUMBERHW(x) ( (WORD)((DWORD)(x)>>(DWORD)16) )
#define SET_NUMBERLW(x) ( (WORD)((DWORD)(x) & 0xFFFF) )
#define MAKE_NUMBERW(x,y)  ( (WORD)(((BYTE)((y)&0xFF)) |   ((BYTE)((x)&0xFF)<<8 ))  )
#define MAKE_NUMBERDW(x,y) ( (DWORD)(((WORD)((y)&0xFFFF)) | ((WORD)((x)&0xFFFF)<<16))  )

#pragma warning(disable:4996 4018 4244 4101)