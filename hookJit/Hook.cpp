#include "stdafx.h"
#include <string>
#include <vector>
#include <map>
#include <fstream>
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
std::map<std::string, std::vector<unsigned char> > g_MyILCodes;


extern "C" __declspec(dllexport) void HookJIT();

void StringReplace(std::string& strBase, const std::string& strSrc, const std::string& strDes)
{
	std::string::size_type pos = 0;
	std::string::size_type srcLen = strSrc.size();
	std::string::size_type desLen = strDes.size();
	pos = strBase.find(strSrc, pos);
	while ((pos != std::string::npos))
	{
		strBase.replace(pos, srcLen, strDes);
		pos = strBase.find(strSrc, (pos + desLen));
	}
}

void StringToBinary(const std::string& source, std::vector<unsigned char>& destination)
{
	std::string s = source;
	StringReplace(s, ":", "");
	StringReplace(s, "0x", "");
	StringReplace(s, ",", "");
	StringReplace(s, " ", "");
	size_t effective_length = s.size();
	if (effective_length % 2 != 0)
		return;
	destination.reserve(effective_length / 2);
	for (size_t b = 0; b < effective_length; b += 2)
	{
		unsigned int c = 0;
		sscanf_s(s.data() + b, "%02x", (unsigned int*)&c);
		destination.push_back(c);
	}
}


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

bool initILCodes(const std::string& ILFile)
{
	std::ifstream fin(ILFile.c_str());
	if (!fin.good())
		return false;

	std::string line;
	while (std::getline(fin, line)) 
	{
		if (line.size() == 0 || line[0] == '#')
			continue;

		size_t idx = line.find(':');
		if (idx == std::string::npos)
			continue;
		std::string methodName = line.substr(0, idx);
		std::string ilHex = line.substr(idx + 1);
		std::vector<unsigned char> ilcode;
		StringToBinary(ilHex, ilcode);
		g_MyILCodes.insert(std::make_pair(methodName, ilcode));
	}

	fin.close();
	return true;
}

extern "C" __declspec(dllexport) void HookJIT()
{
	if (bHooked) 
		return;

	char curPath[MAX_PATH + 1] = { 0 };
	::GetModuleFileNameA(hInstance, curPath, MAX_PATH);
	std::string ILCodePath(curPath);
	ILCodePath = ILCodePath.substr(0, ILCodePath.find_last_of('\\')) + "\\ILCode.txt";
	initILCodes(ILCodePath);

	//LoadLibrary(_T("mscoree.dll"));
	LoadLibrary(_T("clr.dll"));

	HMODULE hJitMod = LoadLibrary(_T("clrjit.dll"));

	if (!hJitMod)
	{
		MessageBoxA(0, "LoadLibrary(_T(\"clrjit.dll\"))==NULL", "getjit", 0);
		return;
	}

	p_getJit = (ULONG_PTR *(__stdcall *)()) GetProcAddress(hJitMod, "getJit");
	if (!p_getJit)
	{
		MessageBoxA(0, "GetProcAddress(hJitMod, \"getJit\")==NULL", "getjit", 0);
		return;
	}

	JIT *pJit = (JIT *)*((ULONG_PTR *)p_getJit());
	if (!pJit)
	{
		MessageBoxA(0, "p_getJit()==NULL", "getjit", 0);
		return;
	}

	DWORD OldProtect;
	if (FALSE == VirtualProtect(pJit, sizeof(ULONG_PTR), PAGE_READWRITE, &OldProtect))
	{
		MessageBoxA(0, "VirtualProtect faild", "getjit", 0);
		return;
	}
	compileMethod = pJit->compileMethod;
	pJit->compileMethod = &my_compileMethod;
	VirtualProtect(pJit, sizeof(ULONG_PTR), OldProtect, &OldProtect);
	bHooked = TRUE;

	MessageBoxA(0, "ok", "getjit", 0);	

}

// 下面代码其实就是C#的一句：return new MyUtil().TestAdd(12, 13).ToString();
//也即private string doAddFun()的方法体


//BYTE doAddFunCode[] = 
//{
//	0x00,                         //nop
//	0x1f,0x0a,
//	0x0a,
//	0x06,
//	0x03,
//	0x5a,
//	0x0a,
//	0x12,0x00,
//	0x28,0x16,0x00,0x00,0x0a,
//	0x0b,
//	0x2b,00,
//	0x07,                         //ldloc.0
//	0x2a                          //ret
//
//};

//
// hooked compileMethod
//

int __stdcall my_compileMethod(ULONG_PTR classthis, ICorJitInfo *comp,
							   CORINFO_METHOD_INFO *info, unsigned flags,         
							   BYTE **nativeEntry, ULONG  *nativeSizeOfCode)
{
	const char* szMethodName = NULL;
	const char* szClassName = NULL;
	szMethodName = comp->getMethodName(info->ftn, &szClassName);
	//MessageBoxA(0, szMethodName, szClassName, 0);

	if (szMethodName && szClassName)
	{
		//if (strcmp(szMethodName, "doAddFun") == 0)
		//{
		//	//MessageBoxA(0, "doAddFun", "begin!", MB_ICONINFORMATION);
		//	info->ILCode = doAddFunCode;
		//	//info->ILCodeSize = sizeof(doAddFunCode);
		//}

		std::string name = std::string(szClassName) + "." + szMethodName;
		auto itFinder = g_MyILCodes.find(name);
		if (itFinder != g_MyILCodes.end())
		{
			MessageBoxA(0, szMethodName, szClassName, 0);
			info->ILCode = itFinder->second.data();
		}
	}

	// call original method

	int nRet = compileMethod(classthis, comp, info, flags, nativeEntry, nativeSizeOfCode);

	return nRet;
}
