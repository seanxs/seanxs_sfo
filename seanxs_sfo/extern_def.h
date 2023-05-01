#pragma once
#ifndef _EXTERN_DEF_H
#define _EXTERN_DEF_H

#include "Common.h"

EXTERN_C_START

extern const wchar_t* SrcFolders[];
extern PFLT_INSTANCE SrcInstance;
extern PFLT_INSTANCE DestInstance;
extern PFLT_FILTER gFilterHandle;
extern HANDLE ProcessFileTest;

EXTERN_C_END

#endif // !_EXTERN_DEF_H
