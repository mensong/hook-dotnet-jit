#include "stdafx.h"
#include <tchar.h>
#include <CorHdr.h>
//#ifdef TARGET_X86
//#include "corinfo_x86.h"
//#include "corjit_x86.h"
//#else
#include "corinfo.h"
#include "corjit.h"
//#endif


HINSTANCE hInstance;

extern "C" __declspec(dllexport) void HookJIT();


BOOL APIENTRY DllMain( HMODULE hModule,
					   DWORD  dwReason,
					   LPVOID lpReserved
					  )
{
	hInstance = (HINSTANCE) hModule;

	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
		HookJIT();
		break;

	case DLL_THREAD_ATTACH:
		break;

	case DLL_THREAD_DETACH:
		break;

	case DLL_PROCESS_DETACH:
		break;
	}

	return TRUE;
}


//
// Hook JIT's compileMethod
//

BOOL bHooked = FALSE;

ULONG_PTR *(__stdcall *p_getJit)();
typedef int (__stdcall *compileMethod_def)(ULONG_PTR classthis, ICorJitInfo *comp, 
										   CORINFO_METHOD_INFO *info, unsigned flags,         
										   BYTE **nativeEntry, ULONG  *nativeSizeOfCode);
struct JIT
{
	compileMethod_def compileMethod;
};

compileMethod_def compileMethod;

int __stdcall my_compileMethod(ULONG_PTR classthis, ICorJitInfo *comp,
							   CORINFO_METHOD_INFO *info, unsigned flags,         
							   BYTE **nativeEntry, ULONG  *nativeSizeOfCode);

extern "C" __declspec(dllexport) void HookJIT()
{
	if (bHooked) 
		return;

	LoadLibrary(_T("mscoree.dll"));

	HMODULE hJitMod = LoadLibrary(_T("clrjit.dll"));

	if (!hJitMod)
		return;

	p_getJit = (ULONG_PTR *(__stdcall *)()) GetProcAddress(hJitMod, "getJit");

	if (p_getJit)
	{
		JIT *pJit = (JIT *) *((ULONG_PTR *) p_getJit());

		if (pJit)
		{
			DWORD OldProtect;
			VirtualProtect(pJit, sizeof (ULONG_PTR), PAGE_READWRITE, &OldProtect);
			compileMethod =  pJit->compileMethod;
			pJit->compileMethod = &my_compileMethod;
			VirtualProtect(pJit, sizeof (ULONG_PTR), OldProtect, &OldProtect);
			bHooked = TRUE;

			MessageBoxA(0, "getjit", "ok", 0);
		}
	}
}

// 下面代码其实就是C#的一句：return new MyUtil().TestAdd(12, 13).ToString();
//也即private string doAddFun()的方法体


#define CODE_SIZE	0x24


BYTE doAddFunCode[] = 
{
	0x00,                         //nop
	0x1f,0x0a,
	0x0a,
	0x06,
	0x03,
	0x5a,
	0x0a,
	0x12,0x00,
	0x28,0x16,0x00,0x00,0x0a,
	0x0b,
	0x2b,00,
	0x07,                         //ldloc.0
	0x2a                          //ret

};

//
// hooked compileMethod
//

int __stdcall my_compileMethod(ULONG_PTR classthis, ICorJitInfo *comp,
							   CORINFO_METHOD_INFO *info, unsigned flags,         
							   BYTE **nativeEntry, ULONG  *nativeSizeOfCode)
{
	// in case somebody hooks us (x86 only)
//#ifdef _M_IX86
//	__asm
//	{
//		nop
//		nop
//		nop
//		nop
//		nop
//		nop
//		nop
//		nop
//		nop
//		nop
//		nop
//		nop
//		nop
//		nop
//	}
//#endif

	//rzh add

	const char* szMethodName = NULL;
	const char* szClassName = NULL;
	const char* szNamespaceName = NULL;
	const char* szEnclosingClassName = NULL;
//#ifdef TARGET_X86
	szMethodName = comp->getMethodName(info->ftn, &szClassName);
//#else
	//szMethodName = comp->getMethodNameFromMetadata(info->ftn, &szClassName, &szNamespaceName, &szEnclosingClassName);
	
//#endif
	//MessageBoxA(0, szClassName, szNamespaceName, 0);
	if (szMethodName && szClassName)
	{
		if (strcmp(szMethodName, "doAddFun") == 0)
		{
			//MessageBoxA(0, "doAddFun", "begin!", MB_ICONINFORMATION);

			info->ILCode = doAddFunCode;
			//info->ILCodeSize = sizeof(doAddFunCode);

		}
	}

	// call original method

	int nRet = compileMethod(classthis, comp, info, flags, nativeEntry, nativeSizeOfCode);

	return nRet;
}
