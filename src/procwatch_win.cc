#include "nan.h"
#include "procwatch.h"
#include <windows.h>
#include <psapi.h>
#include <Strsafe.h>
#include <tlhelp32.h>
#include <tchar.h>
#include <stdio.h>
#include <algorithm>
#include <cctype>

#pragma comment(lib, "version.lib")
#pragma comment(lib, "psapi.lib")

#define MYSTRLEN 256

using namespace v8;
using namespace Nan;

//  Forward declarations:
BOOL findStringIC(const std::string & strHaystack, const std::string & strNeedle);
BOOL findStringVIC(const std::string &strHaystack, std::vector<std::string> & strNeedles);

void WatchWorker::Execute()
{
  std::vector<DWORD> pids;
  BOOL found = ProcessSearch(searchStrings, (BOOL) doDeepSearch, &pids);
  char title[MYSTRLEN];

  if (found)
		{
			for (std::vector<DWORD>::iterator it = pids.begin(); it != pids.end(); ++it)
			{
				if (GetVisibleWindowTitle(*it, title, MYSTRLEN))
				{
					if (title[0] != '\0')
					{
						pid_title_map[*it] = title;
					}
				}
			}
		}
}

void WatchWorker::HandleOKCallback()
{
  Nan::HandleScope scope;

  if (!isError)
  {
    v8::Local<v8::Array> results = New<v8::Array>(pid_title_map.size());
    int i = 0;
    for(std::map<DWORD, std::string>::iterator it = pid_title_map.begin(); it != pid_title_map.end(); ++it)
    {
      v8::Local<v8::Object> pair = New<v8::Object>();
      Nan::Set(pair, (New<v8::String>("pid")).ToLocalChecked(), New<v8::Number>(it->first));
      Nan::Set(pair, (New<v8::String>("title")).ToLocalChecked(), (New<v8::String>(it->second)).ToLocalChecked());
      Nan::Set(results, i++, pair);
    };

    Local<Value> argv[] = {
      Null(),
      results
    };

    callback->Call(2, argv);
  }
  else
  {

    Local<Value> argv[] = {
      (New<v8::String>(errorMsg)).ToLocalChecked(),
      Null()
    };

    callback->Call(2, argv);
  }
}

BOOL WatchWorker::ProcessSearch(/*IN*/std::vector<std::string> search, /*IN*/BOOL doDeepSearch, /*OUT*/std::vector<DWORD> *pids)
{
	HANDLE hProcessSnap;
	PROCESSENTRY32 pe32;
	pids->clear();

	// Take a snapshot of all processes in the system.
	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap == INVALID_HANDLE_VALUE)
	{
		printError(TEXT("CreateToolhelp32Snapshot (of processes)"));
		return FALSE;
	}

	// Set the size of the structure before using it.
	pe32.dwSize = sizeof(PROCESSENTRY32);

	// Retrieve information about the first process,
	// and exit if unsuccessful
	if (!Process32First(hProcessSnap, &pe32))
	{
		printError(TEXT("Process32First")); // show cause of failure
		CloseHandle(hProcessSnap);          // clean the snapshot object
		return FALSE;
	}

	do
	{
		if (findStringVIC(pe32.szExeFile, search)
			|| (doDeepSearch && DeepProcessSearch(pe32.th32ProcessID, search)))
		{
			pids->push_back(pe32.th32ProcessID);
		}


	} while (Process32Next(hProcessSnap, &pe32));

	CloseHandle(hProcessSnap);
	return (pids->size() > 0);
}

struct LANGANDCODEPAGE {
	WORD wLanguage;
	WORD wCodePage;
} *lpTranslate;


BOOL WatchWorker::DeepProcessSearch(/*IN*/DWORD pid, /*IN*/std::vector<std::string>search)
{
	HANDLE hndl = NULL;
	BOOL res;
	char filename[MAX_PATH];

	hndl = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
	if (!hndl)
	{
		printError("OpenProcess");
		return FALSE;
	}

	if (!GetModuleFileNameEx(hndl, NULL, filename, MAX_PATH))
	{
		printError("GetModuleFileNameEx");
		CloseHandle(hndl);
		return FALSE;
	}

	if (findStringVIC(filename, search))
	{
		// found
		CloseHandle(hndl);
		return TRUE;
	}

	DWORD infoSize = GetFileVersionInfoSize(filename, NULL);
	if (!infoSize)
	{
		printError("GetFileVersionInfoSize");
		CloseHandle(hndl);
		return FALSE;
	}

	BYTE *data = new BYTE[infoSize];
	if (!GetFileVersionInfo(filename, NULL, infoSize, data))
	{
		printError("GetFileVersionInfo");
		delete data;
		CloseHandle(hndl);
		return FALSE;
	}

	char SubBlock[50];
	UINT dsize;
	HRESULT hr;
	res = VerQueryValue(data,
		TEXT("\\VarFileInfo\\Translation"),
		(LPVOID*)&lpTranslate, &dsize);

	if (!res)
	{
		printError("VerQueryValue");
		delete data;
		CloseHandle(hndl);
		return FALSE;
	}

	for (int i = 0; i < (dsize / sizeof(struct LANGANDCODEPAGE)); i++)
	{

		hr = StringCchPrintf(SubBlock, 50,
			TEXT("\\StringFileInfo\\%04x%04x\\FileDescription"),
			lpTranslate[i].wLanguage, lpTranslate[i].wCodePage);

		if (FAILED(hr))
		{
			printError("StringCchPrintf");
			continue;
		}
		LPVOID descPtr = NULL;
		UINT descSize;
		// Retrieve file description for language and code page "i".
		res = VerQueryValue(data, SubBlock, &descPtr, &descSize);

		if (!res || !descSize)
		{
			printError("VerQueryValue");
			continue;
		}

		if (findStringVIC((LPCTSTR)descPtr, search))
		{
			// found
			delete data;
			CloseHandle(hndl);
			return TRUE;
		}
	}

	delete data;
	CloseHandle(hndl);
	return FALSE;
}

BOOL WatchWorker::GetVisibleWindowTitle(/*IN*/DWORD pid, /*OUT*/char *title, /*IN*/UINT titlelen)
{
	std::pair<HWND, DWORD> params = { 0, pid };

	// Enumerate the windows using a lambda to process each window
	BOOL bResult = EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL
	{
		auto pParams = (std::pair<HWND, DWORD>*)(lParam);

		DWORD processId;
		if (GetWindowThreadProcessId(hwnd, &processId)
			&& processId == pParams->second)
		{
			// Stop enumerating
			SetLastError(-1);
			pParams->first = hwnd;
			// if the window is visible, it is the one we want
			if (IsWindowVisible(hwnd))
			{
				return FALSE;
			}
			return TRUE;
		}

		// Continue enumerating
		return TRUE;
	}, (LPARAM)&params);

	if (!bResult && GetLastError() == -1 && params.first)
	{
		GetWindowText(params.first, title, titlelen);
		return TRUE;
	}

	return FALSE;
}

BOOL findStringIC(const std::string & strHaystack, const std::string & strNeedle)
{
	auto it = std::search(
		strHaystack.begin(), strHaystack.end(),
		strNeedle.begin(), strNeedle.end(),
		[](char ch1, char ch2) { return std::toupper(ch1) == std::toupper(ch2); }
	);
	return (it != strHaystack.end());
}

BOOL findStringVIC(const std::string &strHaystack, std::vector<std::string> & strNeedles)
{
	for (std::vector<std::string>::iterator it = strNeedles.begin(); it != strNeedles.end(); ++it)
	{
		if (findStringIC(strHaystack, *it))
		{
			return TRUE;
		}
	}
	return FALSE;
}

void WatchWorker::printError(TCHAR* msg)
{
  isError = TRUE;
  DWORD eNum;
	TCHAR sysMsg[MYSTRLEN];
	TCHAR* p;

	eNum = GetLastError();
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, eNum,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		sysMsg, 256, NULL);

	// Trim the end of the line and terminate it with a null
	p = sysMsg;
	while ((*p > 31) || (*p == 9))
		++p;
	do { *p-- = 0; } while ((p >= sysMsg) &&
		((*p == '.') || (*p < 33)));

	errorMsg += msg;
  errorMsg += " failed with error ";
  errorMsg += eNum;
  errorMsg += " (";
  errorMsg += sysMsg;
  errorMsg += ");";
}
