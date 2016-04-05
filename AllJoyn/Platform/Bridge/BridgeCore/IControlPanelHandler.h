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
#include "AdapterDefinitions.h"

namespace Bridge
{

    // *****************************************************************************************************************************************************
    //
    // Control Panel Handler Base Class.  Derived handlers primarily handle the details necessary to exchange data between the AllJoyn Control Panel
    // and the adapted device type.  To change the layout and features of a Control Panel, changes must be made in the DsbBridge and an associated Control
    // Panel Handler must be derived from here.
    //
    // *****************************************************************************************************************************************************
    class IControlPanelHandler
    {
		virtual std::shared_ptr<IAdapterSignal> ChangeOfValueSignal() = 0;
	};


    // *****************************************************************************************************************************************************
    //
    //  Simple Control Panel Handler provides data for the Simple Control Panel.  A Simple Control Panel exposes a vertically oriented Labeled Switch, Labeled Property and a Labeled Text
    //  Entry box with Labeled Action Button control.  An exmaple output may look similar to the following, but varies depending on the Control Panel Renderer
    //
    //  [Switch Label]
    //  [Boolean Switch Control]
    //
    //  [Property Label]
    //  [Property Value]
    //
    //  [Text Entry Box Label] : [ Text Entry Box Control ]
    //  [ Labeled Button Control ]
    //
    //  The Simple Control Panel Handler provides the data that the control panel displays for the aforementioned control panel features.  For example,
    //  a switch could graphically be displayed like this:
    //
    //  Light Switch
    //  [on]   off
    //
    // *****************************************************************************************************************************************************
    class  IControlPanelHandlerSimple : public IControlPanelHandler
    {
    public:

        virtual std::shared_ptr<IAdapterSignal> ChangeOfValueSignal() = 0;

        // *****************************************************************************************************************************************************
        //
        //  Read-Only Output Label.
        //
        // *****************************************************************************************************************************************************

        // Gets the label name for the output value
        // This could be the OutputValue->Name, Property->Name or some custom name
        virtual std::string OutputValueLabel() = 0;

        // Gets the Output Value.  This is the current data value content displayed to the user
        // Set to nullptr if not supporting an output value
        virtual std::shared_ptr<IAdapterValue> OutputValue() = 0;

        // *****************************************************************************************************************************************************
        //
        //  Boolean Switch Control
        //
        // *****************************************************************************************************************************************************

        // Gets the label name for the switch control
        virtual std::string SwitchLabel() = 0;

        // Gets the Switch property.  This is the current switch property bag.  This is neeed to trigger value updates
        // when the specific SwitchValue member of the property bag's state changes)
        virtual std::shared_ptr<IAdapterProperty> SwitchProperty() = 0;


        // Gets the Switch value.  This is the current switch data value.  The IAdapterValue must be a boolean data type or null if not supported
        // For non-boolean data types a value of true will always be displayed on the control panel
        virtual std::shared_ptr<IAdapterValue> SwitchValue() = 0;

        // *****************************************************************************************************************************************************
        //
        //  Run Button Handler.
        //
        // *****************************************************************************************************************************************************

        // Gets the Run Button Label Value, or nullptr if no run button is handled by this control
        // (For example: "Go", "Run", "Execute"...)
        virtual std::string RunButtonLabel() = 0;

        // Gets the Label for the Text Entry Box that will be filled in with a string value on the Remote Control, or null
        // (For example: "Enter A Command", "Enter Dollar Amount", etc).  If the Entry Box Label is null, no entry box is displayed,
        virtual std::string RunEntryBoxLabel() = 0;

        // The method to call when the Run Button is pressed
        virtual void Run(std::string runArgument) = 0;

    };

    enum class ControlType
    {
        Switch,     /* A Labeled Boolean Switch.  For example: Light Switch */
        Sensor,     /* A Labeled Data Value.  For example:  Current Temperature */
    };

    class  IControlPanelHandlerUniversal : public IControlPanelHandler
    {
    public:

        // *****************************************************************************************************************************************************
        //
        //  The Change of Value signal for this device.  Raised by a device when one of its properties changes
        //
        // *****************************************************************************************************************************************************
        virtual std::shared_ptr<IAdapterSignal> ChangeOfValueSignal() = 0;

        // *****************************************************************************************************************************************************
        //
        //  Get the Label to display for the specified property.
        //
        // *****************************************************************************************************************************************************
        virtual std::string GetLabel(std::shared_ptr<IAdapterProperty> prop) = 0;

        // *****************************************************************************************************************************************************
        //
        //  Get the Value that the control panel will operate/monitor for the specified property.
        //
        // *****************************************************************************************************************************************************
        virtual std::shared_ptr<IAdapterValue> GetValue(std::shared_ptr<IAdapterProperty> prop) = 0;

        // *****************************************************************************************************************************************************
        //
        //  Get the ControlType for this property (how should this property be rendered on the control panel)
        //
        // *****************************************************************************************************************************************************
        virtual ControlType GetType(std::shared_ptr<IAdapterProperty> prop) = 0;

        // *****************************************************************************************************************************************************
        //
        //  Set of all properties to activate/render on a device's control panel
        //
        // *****************************************************************************************************************************************************
        virtual AdapterPropertyVector ControlledProperties() = 0;
    };
}
