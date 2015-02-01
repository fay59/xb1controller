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

#include "XboxOneController.h"

OSDefineMetaClassAndStructors(com_felixcloutier_driver_XboxOneControllerDriver, IOUSBHIDDriver)

#if DEBUG
// Do not use this macro with numbered arguments.
# define IO_LOG_DEBUG(fmt, ...) IOLog("%s: " fmt "\n", __func__, ## __VA_ARGS__)
#else
# define IO_LOG_DEBUG(...)
#endif

// The convention is to #define 'super' to the superclass's name, Java-style, and then use that
// through the code.
#define super IOUSBHIDDriver

// "com_felixcloutier_driver_XboxOneControllerDriver" is a heck of a long name. This should make it prettier.
#define XboxOneControllerDriver com_felixcloutier_driver_XboxOneControllerDriver

// Magic words to make the controller work.
constexpr UInt8 XboxOneControllerHelloMessage[] = {0x05, 0x20};

// Report descriptor. I made it myself.
// This guy (http://eleccelerator.com/tutorial-about-usb-hid-report-descriptors/) has a nice tutorial.
// See http://www.usb.org/developers/devclass_docs/Hut1_12v2.pdf for usage page and usage.
// See http://www.usb.org/developers/hidpage#HID%20Descriptor%20Tool for Windows tool to create/parse HID report
// descriptors.
// http://sourceforge.net/projects/hidrdd/ was used to validate the descriptor by making sure that the emitted struct
// looks like what you would expect.
constexpr UInt8 XboxOneControllerReportDescriptor[] = {
	0x05, 0x01,					// USAGE_PAGE (Generic Desktop)
	0x09, 0x05,					// USAGE (Game pad)
	0xa1, 0x01,					// COLLECTION (Application)
		0xa1, 0x00,				// COLLECTION (Physical)
	
			// 20 00 ss EC (where ss is a sequence number)
			0x09, 0x3f,			// USAGE (Reserved)
			0x75, 0x20,			// REPORT_SIZE (16)
			0x95, 0x01,			// REPORT_COUNT (1)
			0x81, 0x02,			// INPUT (Data,Var,Abs)
	
			// buttons
			0x05, 0x09,			// USAGE_PAGE (Button)
			0x19, 0x01,			// USAGE_MINIMUM (Button 1)
			0x29, 0x10,			// USAGE_MAXIMUM (Button 16)
			0x15, 0x00,			// LOGICAL_MINIMUM (0)
			0x25, 0x01,			// LOGICAL_MAXIMUM (1)
			0x95, 0x10,			// REPORT_COUNT (16)
			0x75, 0x01,			// REPORT_SIZE (1)
			0x81, 0x02,			// INPUT (Data,Var,Abs)
	
			// triggers
			// Colin Munro's Xbox 360 controller driver uses Z and Rz instead of buttons.
			// OS X seems to dislike non-boolean buttons, so that's what I'll be doing too.
			0x05, 0x01,			// USAGE_PAGE (Generic Desktop)
			0x09, 0x32,			// USAGE (Z)
			0x09, 0x35,			// USAGE (Rz)
			0x15, 0x00,			// LOGICAL_MINIMUM (0)
			0x26, 0x00, 0x04,	// LOGICAL_MAXIMUM (1024)
			0x75, 0x10,			// REPORT_SIZE (16)
			0x95, 0x02,			// REPORT_COUNT (2)
			0x81, 0x02,			// INPUT (Data,Var,Abs)
	
			// sticks prefix
			0x16, 0x00, 0x80,	// LOGICAL_MINIMUM (-32768)
			0x26, 0xff, 0x7f,	// LOGICAL_MAXIMUM (32767)
			0x36, 0x00, 0x80,	// PHYSICAL MINIMUM (-32768)
			0x46, 0xff, 0x7f,	// PHYSICAL_MAXIMUM (32767)
			0x95, 0x02,			// REPORT_COUNT (2)
			0x75, 0x10,			// REPORT_SIZE (16)
			0x05, 0x01,			// USAGE_PAGE (Generic Desktop)
	
			// left stick
			0x09, 0x01,			// USAGE (Pointer)
			0xa1, 0x00,			// COLLECTION (Physical)
				0x09, 0x30,		// USAGE (X)
				0x09, 0x31,		// USAGE (Y)
				0x81, 0x02,		// INPUT (Data,Var,Abs)
			0xc0,				// END COLLECTION
	
			// right stick
			0x09, 0x01,			// USAGE (Pointer)
			0xa1, 0x00,			// COLLECTION (Physical)
				0x09, 0x33,		// USAGE (Rx)
				0x09, 0x34,		// USAGE (Ry)
				0x81, 0x02,		// INPUT (Data,Var,Abs)
			0xc0,				// END COLLECTION
	
		0xc0,					// END COLLECTION
	0xc0,						// END COLLECTION
};

bool XboxOneControllerDriver::init(OSDictionary* dict)
{
	// Just make sure that the driver is in a predictable state.
	if (!super::init(dict))
	{
		return false;
	}
	
	_interruptPipe = nullptr;
	return true;
}

bool XboxOneControllerDriver::didTerminate(IOService *provider, IOOptionBits options, bool *defer)
{
	// IOUSBHIDDriver releases its _interruptPipe reference from `didTerminate`, so it's probably the right
	// place to do this.
	
	if (_interruptPipe != nullptr)
	{
		_interruptPipe->release();
		_interruptPipe = nullptr;
	}
	
	return super::didTerminate(provider, options, defer);
}

IOReturn XboxOneControllerDriver::newReportDescriptor(IOMemoryDescriptor **descriptor) const
{
	if (descriptor == nullptr)
	{
		return kIOReturnBadArgument;
	}
	
	constexpr size_t descriptorSize = sizeof XboxOneControllerReportDescriptor;
	IOMemoryDescriptor* buffer = IOBufferMemoryDescriptor::inTaskWithOptions(kernel_task, 0, descriptorSize);
	if (buffer == nullptr)
	{
		return kIOReturnNoMemory;
	}
	
	IOByteCount written = buffer->writeBytes(0, XboxOneControllerReportDescriptor, descriptorSize);
	if (written != sizeof XboxOneControllerReportDescriptor) // paranoid check
	{
		buffer->release();
		return kIOReturnNoSpace;
	}
	
	*descriptor = buffer;
	return kIOReturnSuccess;
}

IOReturn XboxOneControllerDriver::setPowerState(unsigned long powerStateOrdinal, IOService *device)
{
	// For some reason, Apple is returning kIOPM constants as IOReturn codes even though
	// they do not follow the IOReturn convention. Don't check them with SUCCEEDED or FAILED!
	IOReturn ior = super::setPowerState(powerStateOrdinal, device);
	if (ior != kIOPMAckImplied)
	{
		IO_LOG_DEBUG("super::setPowerState failed");
		return ior;
	}
	
	// When we come back from sleep, the controller is off and needs to be acknowledged again.
	// This is the only case that we specifically want to handle.
	if (powerStateOrdinal == kUSBHIDPowerStateOn)
	{
		IOReturn ior = sendHello();
		if (ior != kIOReturnSuccess)
		{
			IO_LOG_DEBUG("Couldn't wake up because sendHello failed: %08x", ior);
			
			// Since we have to return one of the kIOPM constants, we can't return this IOResult
			// directly. Since there is no generic error return code, we would have to return
			// success even though we weren't successful.
		}
	}
	
	return kIOPMAckImplied;
}

bool XboxOneControllerDriver::handleStart(IOService *provider)
{
	IOUSBInterface* interface = nullptr;
	IOUSBPipe* interruptPipe = nullptr;
	IOReturn ior = kIOReturnSuccess;
	
	IOUSBFindEndpointRequest pipeRequest = {
		.type = kUSBInterrupt,
		.direction = kUSBOut,
	};
	
	// Apple says you should call super::handleStart at the *beginning* of the method.
	if (!super::handleStart(provider))
	{
		goto cleanup;
	}
	
	// Paranoid check: is it the correct kind of object?
	interface = OSDynamicCast(IOUSBInterface, provider);
	if (interface == nullptr)
	{
		IO_LOG_DEBUG("IOUSBHIDDriver is handling start on an object that's not a IOUSBInterface??");
		goto cleanup;
	}
	
	// Find the pipe through which we have to send the hello message.
	// This is redundant with work that IOUSBHIDDriver already did, but there are no accessors.
	interruptPipe = interface->FindNextPipe(nullptr, &pipeRequest);
	if (interruptPipe == nullptr)
	{
		IO_LOG_DEBUG("No interrupt pipe found on controller");
		goto cleanup;
	}
	
	// `sendHello` needs _interruptPipe to be set, but only retain it if it succeeds.
	_interruptPipe = interruptPipe;
	ior = sendHello();
	if (ior != kIOReturnSuccess)
	{
		IO_LOG_DEBUG("Couldn't send hello message: %08x", ior);
		_interruptPipe = nullptr;
		goto cleanup;
	}
	
	_interruptPipe->retain();
	
cleanup:
	return _interruptPipe != nullptr;
}

IOReturn XboxOneControllerDriver::handleReport(IOMemoryDescriptor *descriptor, IOHIDReportType type, IOOptionBits options)
{
	// The first byte of the report tells what kind of report it is.
	UInt8 opcode;
	IOByteCount read = descriptor->readBytes(0, &opcode, sizeof opcode);
	if (read != 1)
	{
		IO_LOG_DEBUG("Couldn't read a single byte from report descriptor!");
		return kIOReturnNoMemory;
	}
	
	// 0x20 is a button state report, anything else should be ignored (at least until we figure out what it is).
	if (opcode != 0x20)
	{
		return kIOReturnSuccess;
	}
	
	return super::handleReport(descriptor, type, options);
}

IOReturn XboxOneControllerDriver::sendHello()
{
	IOReturn ior = kIOReturnSuccess;
	IOMemoryDescriptor* hello = nullptr;
	IOByteCount bytesWritten = 0;
	constexpr size_t helloSize = sizeof XboxOneControllerHelloMessage;
	
	if (_interruptPipe == nullptr) // paranoid check
	{
		IO_LOG_DEBUG("_interruptPipe is null");
		ior = kIOReturnInternalError;
		goto cleanup;
	}
	
	// Create the hello message that we're about to send to the controller.
	hello = IOBufferMemoryDescriptor::inTaskWithOptions(kernel_task, 0, helloSize);
	if (hello == nullptr)
	{
		IO_LOG_DEBUG("Could not allocate buffer for hello message.");
		ior = kIOReturnNoMemory;
		goto cleanup;
	}
	
	bytesWritten = hello->writeBytes(0, XboxOneControllerHelloMessage, helloSize);
	if (bytesWritten != helloSize) // paranoid check
	{
		ior = kIOReturnOverrun;
		goto cleanup;
	}
	
	// Now send the message
	ior = _interruptPipe->Write(hello, 0, 0, hello->getLength());
	if (ior != kIOReturnSuccess)
	{
		IO_LOG_DEBUG("Couldn't send hello message to controller: %08x\n", ior);
		goto cleanup;
	}
	
cleanup:
	if (hello != nullptr)
	{
		hello->release();
	}
	
	return ior;
}
