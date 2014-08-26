//
// Copyright (c) 2014 Félix Cloutier
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

// The purpose of the XboxOneControllerDriver class is to attach to the USB interface
// and provide actual HID driver functionality. Since the class derives from IOUSBHIDDriver,
// most of it is already implemented for us. We know that the controller behaves differently
// from a USB HID gamepad in three ways:
//
// * it doesn't have a report descriptor;
// * it needs to be sent a hello message;
// * it sends reports that aren't state reports.
//
// We override three methods to fix this: `newReportDescriptor` (to provide a correct report
// descriptor), `handleStart` (to send the hello message) and `handleReport` (to handle
// reports that aren't state reports).
//
// Additionally, we override `setPowerState` to wake up the controller after sleep.
//
// IOUSBHIDDriver already handles cases like disconnects, which is quite nifty.

#ifndef	XBOX_ONE_CONTROLLER_DRIVER_H
#define XBOX_ONE_CONTROLLER_DRIVER_H

#include <IOKit/usb/IOUSBHIDDriver.h>

// This class does not attach to the USB device: it attaches to its first interface.
// This is an important distinction, as the device starts in an unconfigured state.
// As the controller exposes no interface in an unconfigured state, we need another
// driver to configure it. AppleUSBComposite does just that, except that it won't match
// by default on devices with a vendor-specific class. Fortunately, nothing prevents us
// from giving it a new matching personality from our own Info.plist to make it match:
// no need to write an identical driver ourselves.
class com_felixcloutier_driver_XboxOneControllerDriver : public IOUSBHIDDriver
{
	OSDeclareDefaultStructors(com_felixcloutier_driver_XboxOneControllerDriver)
	
public:
	virtual bool init(OSDictionary* dictionary = nullptr) override;
	virtual bool didTerminate(IOService* provider, IOOptionBits options, bool* defer) override;
	
	virtual IOReturn newReportDescriptor(IOMemoryDescriptor** descriptor) const override;
	virtual IOReturn setPowerState(unsigned long powerStateOrdinal, IOService* device) override;
	
protected:
	virtual bool handleStart(IOService* provider) override;
	virtual IOReturn handleReport(IOMemoryDescriptor* descriptor, IOHIDReportType type, IOOptionBits options) override;
	
private:
	IOReturn sendHello();
	
	IOUSBPipe* _interruptPipe;
};

#endif