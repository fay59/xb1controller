//
//  XboxOneControllerDriver.h
//  XboxOneControllerDriver
//
//  Created by Félix on 2014-08-20.
//  Copyright (c) 2014 Félix Cloutier. All rights reserved.
//

//
// The Xbox One controller exposes 3 USB interfaces, all of class 0xff, subclass
// 0x47 and protocol 0xd0. This means that generic USB HID drivers, which expect
// class 3 devices, will not recognize it. Since we cannot use most of the
// generic USB driver's functionality, we subclass from IOHIDDevice instead of
// IOUSBHIDDriver.
//
// The Xbox One controller does not report its state like a standard Human
// Interface Device through a Report descriptor. Instead, it sends a "command"
// that indicates button presses and hat directions. IOKit was not built with
// only USB in mind, but Apple considered that most HIDs would be HID-compliant,
// so the simplest way to implement functionality was to ask drivers to expose a
// HID-compliant interface, no matter what is the actual transport or protocol.
// This driver is therefore an adapter between the Xbox One controller's
// proprietary protocol and the standard USB HID interface.
//

#ifndef	XBOX_ONE_CONTROLLER_DRIVER_H
#define XBOX_ONE_CONTROLLER_DRIVER_H

#include <IOKit/hid/IOHIDDevice.h>
#include <IOKit/usb/IOUSBDevice.h>
#include <IOKit/usb/IOUSBInterface.h>

class com_felixcloutier_driver_XboxOneController : public IOHIDDevice
{
	OSDeclareDefaultStructors(com_felixcloutier_driver_XboxOneController)
	
public:
	virtual bool init(OSDictionary* dict = nullptr) override;
	virtual IOReturn newReportDescriptor(IOMemoryDescriptor** descriptor) const override;
	
	// The values returned by these methods end up in the properties dictionary of the driver.
	virtual OSString* newManufacturerString() const override;
	virtual OSString* newProductString() const override;
	virtual OSString* newTransportString() const override;
	virtual OSNumber* newPrimaryUsageNumber() const override;
	virtual OSNumber* newPrimaryUsagePageNumber() const override;
	virtual OSNumber* newLocationIDNumber() const override;
	
protected:
	virtual void free() override;
	virtual bool handleStart(IOService* provider) override;
	virtual void handleStop(IOService* provider) override;
	
private:
	IOUSBDevice* _controller;
	IOUSBInterface* _controllerInterface;
	IOUSBPipe* _pipeFromController;
	IOUSBPipe* _pipeToController;
	IOMemoryDescriptor* _readBuffer;
	
	void releaseEverything();
	
	IOReturn beginAsyncRead();
	static void finishedAsyncRead(void* target, void* parameter, IOReturn status, UInt32 bufferRemaining);
};

#endif
