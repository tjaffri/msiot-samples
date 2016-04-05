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

#pragma once

#include "../FileAccess.h"

namespace BridgePAL
{
	class WinFileAccess :
		public IFileAccess
	{
	public:
		WinFileAccess();
		virtual ~WinFileAccess();

		virtual std::string GetConfigurationFolderPath();

		virtual std::string GetPackageInstallFolderPath();

		virtual std::string AppendPath(std::string base, std::string path);

		virtual bool IsFilePresent(std::string existing);

		virtual QStatus CopyFile(std::string existing, std::string newfile);

		virtual bool DelFile(std::string existing);

		virtual int64_t GetFileSize(std::string existing);

		virtual HANDLE OpenFileForRead(std::string existing);

		virtual HANDLE OpenFileForWrite(std::string newfile, uint32_t disposition);

		virtual bool ReadFromFile(HANDLE hfile, void* buffer, size_t bytes, uint32_t* bytesRead);
		
		virtual bool WriteToFile(HANDLE hfile, void* buffer, size_t bytes, uint32_t* bytesWritten);

		virtual bool CloseFile(HANDLE hfile);

		virtual bool SetFilePointer(HANDLE hfile, int64_t position, uint32_t moveMethod);

		virtual std::string GetHostName();
	};

}