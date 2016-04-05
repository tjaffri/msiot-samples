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
#include "IAdapter.h"


namespace Bridge
{
    class ControlPanel;

    // To handle a device's change of value signal, a class must be registered with the Adapter.
    class WidgetSignalHandler final : public IAdapterSignalListener
    {
    public:
        // Signal Handler Called by Signal Dispatcher.
        void AdapterSignalHandler(_In_ std::shared_ptr<IAdapter> sender, _In_ std::shared_ptr<IAdapterSignal> Signal, _In_opt_ intptr_t Context) override;

    //internal:
        // Constructor
        WidgetSignalHandler(ControlPanel* pControlPanel);

        // Control Panel to forward Signal Change to
        ControlPanel* m_pControlPanel;
    };
}
