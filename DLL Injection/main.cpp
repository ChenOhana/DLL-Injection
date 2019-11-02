#include <windows.h>
#include <iostream>
#include <string>
#include <sstream>

using std::cout;
using std::endl;

#define SIZE 4096
#define DLL_PATH "C:/Users/user/Documents/Magshimim 2/Linux/DLL Injection/dll.dll"

DWORD GetProcessId(std::string procName);
int GetIdFromString(std::string str);

int main()
{
	TCHAR  lpBuffer[SIZE];
	TCHAR** lpFilePart = { NULL };

	// Get LoadLibrary function address –
	// the address doesn't change at remote process
	PVOID addrLoadLibrary = (PVOID)GetProcAddress(GetModuleHandle(TEXT("Kernel32.dll")), "LoadLibraryA");
	
	// Open remote process
	HANDLE proc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, GetProcessId("notepad"));

	// Get a pointer to memory location in remote process,
	// big enough to store DLL path
	PVOID memAddr = (PVOID)VirtualAllocEx(proc, NULL, SIZE, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

	while (NULL == memAddr) //while process wasn't found
	{
		proc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, GetProcessId("notepad")); //try to open process
		memAddr = (PVOID)VirtualAllocEx(proc, NULL, SIZE, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE); //try to allocate memory
	}

	SIZE_T bytesWritten = 0;

	// Write DLL name to remote process memory
	BOOL check = WriteProcessMemory(proc, memAddr, DLL_PATH, SIZE, &bytesWritten);

	if (0 == check)
	{
		cout << GetLastError() << endl;
		system("pause");
		return 0;
	}

	// Open remote thread, while executing LoadLibrary
	// with parameter DLL name, will trigger DLLMain
	HANDLE hRemote = CreateRemoteThread(proc, NULL, 0, (LPTHREAD_START_ROUTINE)addrLoadLibrary, memAddr, NULL, NULL);
	
	if (NULL == hRemote)
	{
		cout << GetLastError() << endl;
		system("pause");
		return 0;
	}

	WaitForSingleObject(hRemote, INFINITE);
	check = CloseHandle(hRemote);

	return 0;
}

/*
Function will return process id.
Input: process name
Output: id
*/

DWORD GetProcessId(std::string procName)
{
	char buffer[SIZE];
	std::string output;
	FILE* p = _popen("PowerShell \"Get-Process notepad | Format-Table id\"", "r"); //get process id from PowerShell
	int size = fread(buffer, 1, sizeof(buffer), p);

	output = std::string(buffer, size);
	_pclose(p);

	return GetIdFromString(output); //get id from PowerShell output as int
}

/*
Function will parse PowerShell output in order to find the id number.
Input: str - PowerShell Output
Output: id as int
*/

int GetIdFromString(std::string str)
{
	int id;
	std::stringstream ss;
	std::string temp;

	ss << str;

	while (!ss.eof()) 
	{
		ss >> temp;
		std::stringstream(temp) >> id;
	}
	return id;
}