//
// Copyright (c) 2015, Microsoft Corporation
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
// SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
// IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
//

#include "pch.h"
#include "../BridgeCommon.h"
#include "WinFileAccess.h"


using namespace BridgePAL;

// Windows versions of file access methods
// Not really used at this time

WinFileAccess::WinFileAccess()
{
}

WinFileAccess::~WinFileAccess()
{
}

std::string WinFileAccess::GetConfigurationFolderPath()
{
#ifdef WINCX
	std::shared_ptr<StorageFolder> tempFolder = ApplicationData::Current->LocalFolder;
	std::string tempFilePath = tempFolder->Path;
#else
	// TODO: replace with real code
	std::string tempFilePath = "c:/config";
#endif
	return tempFilePath;
}
std::string WinFileAccess::GetPackageInstallFolderPath()
{
#ifdef WINCX
	std::shared_ptr<StorageFolder> tempFolder = Package::Current->InstalledLocation;
	std::string tempFilePath = tempFolder->Path;
#else
	// TODO: replace with real code
	std::string tempFilePath = "c:/config";
#endif

	return tempFilePath;
}

std::string WinFileAccess::AppendPath(std::string base, std::string path)
{
	// TODO: rewrite to use proper path APIs
	std::string combined = base;
	if (path[0] != '/' && path[0]!= '\\' && base[base.length() - 1] != '/' && base[base.length() - 1] != '\\')
		combined += "/";
	combined += path;
	return combined;
}

bool WinFileAccess::IsFilePresent(std::string existing)
{
	WIN32_FILE_ATTRIBUTE_DATA fileAttribute = { 0 };

	std::wstring wexist = StringUtils::ToWstring(existing);
	if (GetFileAttributesEx(wexist.c_str(), GetFileExInfoStandard, &fileAttribute))
	{
		return true;
	}
	return false;
}

QStatus WinFileAccess::CopyFile(std::string existing, std::string newfile)
{
	std::wstring wexist = StringUtils::ToWstring(existing);
	std::wstring wnewFile = StringUtils::ToWstring(newfile);
	return CopyFile2(wexist.c_str(), wnewFile.c_str(), nullptr) == S_OK ? ER_OK : ER_FAIL;
}

bool WinFileAccess::DelFile(std::string existing)
{
	std::wstring wexist = StringUtils::ToWstring(existing);
	return DeleteFile(wexist.c_str()) !=0;
}

// return -1 if no file
int64_t WinFileAccess::GetFileSize(std::string existing)
{
	WIN32_FILE_ATTRIBUTE_DATA fileAttribute = { 0 };

	std::wstring wexist = StringUtils::ToWstring(existing);
	if (GetFileAttributesEx(wexist.c_str(), GetFileExInfoStandard, &fileAttribute))
	{
		return (int64_t)fileAttribute.nFileSizeLow + ((int64_t)fileAttribute.nFileSizeHigh << 32);
	}
	return -1L;
}

HANDLE WinFileAccess::OpenFileForRead(std::string existing)
{
	std::wstring wexist = StringUtils::ToWstring(existing);
	return CreateFile2(wexist.c_str(), GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, nullptr);
}

HANDLE WinFileAccess::OpenFileForWrite(std::string newfile, uint32_t disposition)
{
	std::wstring wnewfile = StringUtils::ToWstring(newfile);
	return CreateFile2(wnewfile.c_str(), GENERIC_READ | GENERIC_WRITE, 0, disposition, nullptr);
}

bool WinFileAccess::ReadFromFile(HANDLE hfile, void* buffer, size_t bytes, uint32_t* bytesRead)
{
    // TODO: Should use correctly sized types
	return ReadFile(hfile, buffer, static_cast<DWORD>(bytes), (LPDWORD)bytesRead, nullptr) != 0;
}

bool WinFileAccess::CloseFile(HANDLE hfile)
{
	return CloseHandle(hfile) != 0;
}

bool WinFileAccess::WriteToFile(HANDLE hfile, void* buffer, size_t bytes, uint32_t* bytesWritten)
{
    // TODO: Should use correctly sized types
	return WriteFile(hfile, buffer, static_cast<DWORD>(bytes), (LPDWORD)bytesWritten, nullptr) != 0;
}

bool WinFileAccess::SetFilePointer(HANDLE hfile, int64_t position, uint32_t moveMethod)
{
	LARGE_INTEGER pos;
	pos.QuadPart = (LONGLONG)position;
	return SetFilePointerEx(hfile, pos, nullptr, moveMethod) != 0;
}

std::string WinFileAccess::GetHostName()
{
#ifdef WINCX
	std::string name = Windows::Networking::Proximity::PeerFinder::DisplayName->Data();
#else
	// TODO: replace with real code
	std::string name = "thiscomputer";
#endif

	return name;

}
