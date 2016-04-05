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

#include "IBridgeJs.h"
#include "IJsAdapter.h"

namespace BridgeRT
{
	public ref class BridgeJs sealed : public IBridgeJs
	{
	public:
		BridgeJs();

		virtual int32_t Initialize();
		virtual void AddDevice(_In_ DeviceInfo ^device, _In_ Platform::String^ baseTypeXml, _In_ Platform::String^ jsScript, _In_ Platform::String^ modulesPath);
		virtual void Shutdown();
		event Windows::Foundation::EventHandler<Platform::String^>^ Error;
		void RaiseEvent(Platform::String^ msg);

    private:
        class AdapterError : public AdapterLib::IJsAdapterError
        {
        public:
            void ReportError(_In_ const std::string& msg) override;
        };
	};
}
