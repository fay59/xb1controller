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

OSDefineMetaClassAndStructors(com_felixcloutier_driver_XboxOneController, IOHIDDevice)

#if DEBUG
// Do not use this macro with numbered arguments.
# define IO_LOG_DEBUG(fmt, ...) IOLog("%s: " fmt "\n", __func__, ## __VA_ARGS__)
#else
# define IO_LOG_DEBUG(...)
#endif

// The convention is to #define 'super' to the superclass's name, Java-style, and then use that
// through the code.
#define super IOHIDDevice

// "com_felixcloutier_driver_XboxOneController" is a heck of a long name. This should make it simpler.
#define XboxOneController com_felixcloutier_driver_XboxOneController

// Magic words to make the controller work.
constexpr UInt8 XboxOneControllerHelloMessage[] = {0x05, 0x20};

// Report descriptor. I made it myself.
// This guy (http://eleccelerator.com/tutorial-about-usb-hid-report-descriptors/) has a nice tutorial.
// See http://www.usb.org/developers/devclass_docs/Hut1_12v2.pdf for usage page and usage.
// See http://www.usb.org/developers/hidpage#HID%20Descriptor%20Tool for Windows tool to create/parse HID report
// descriptors.
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

			// hat prefixes
			0x16, 0x00, 0x80,	// LOGICAL_MINIMUM (-32768)
			0x26, 0xff, 0x7f,	// LOGICAL_MAXIMUM (32767)
			0x36, 0x00, 0x80,	// PHYSICAL MINIMUM (-32768)
			0x46, 0xff, 0x7f,	// PHYSICAL_MAXIMUM (32767)
			0x95, 0x02,			// REPORT_COUNT (2)
			0x75, 0x10,			// REPORT_SIZE (16)
			0x05, 0x01,			// USAGE_PAGE (Generic Desktop)

			// left hat
			0x09, 0x01,			// USAGE (Pointer)
			0xa1, 0x00,			// COLLECTION (Physical)
				0x09, 0x30,		// USAGE (X)
				0x09, 0x31,		// USAGE (Y)
				0x81, 0x02,		// INPUT (Data,Var,Abs)
			0xc0,				// END COLLECTION

			// right hat
			0x09, 0x01,			// USAGE (Pointer)
			0xa1, 0x00,			// COLLECTION (Physical)
				0x09, 0x33,		// USAGE (Rx)
				0x09, 0x34,		// USAGE (Ry)
				0x81, 0x02,		// INPUT (Data,Var,Abs)
			0xc0,				// END COLLECTION

		0xc0,					// END COLLECTION
	0xc0,						// END COLLECTION
};

bool XboxOneController::init(OSDictionary* dictionary)
{
	if (!super::init(dictionary))
	{
		return false;
	}
	
	_controller = nullptr;
	_controllerInterface = nullptr;
	_pipeFromController = nullptr;
	_pipeToController = nullptr;
	_readBuffer = nullptr;
	return true;
}

IOReturn XboxOneController::newReportDescriptor(IOMemoryDescriptor **descriptor) const
{
	if (descriptor == nullptr)
	{
		return kIOReturnBadArgument;
	}
	
	IOMemoryDescriptor* buffer = IOBufferMemoryDescriptor::inTaskWithOptions(kernel_task, 0, sizeof XboxOneControllerReportDescriptor);
	if (buffer == nullptr)
	{
		return kIOReturnNoMemory;
	}
	
	IOByteCount written = buffer->writeBytes(0, XboxOneControllerReportDescriptor, sizeof XboxOneControllerReportDescriptor);
	if (written != sizeof XboxOneControllerReportDescriptor) // paranoid check
	{
		return kIOReturnNoSpace;
	}
	
	*descriptor = buffer;
	return kIOReturnSuccess;
}

OSString* XboxOneController::newManufacturerString() const
{
	return OSString::withCString("Microsoft (UNOFFICIAL DRIVER)");
}

OSString* XboxOneController::newProductString() const
{
	return OSString::withCString("Xbox One Controller");
}

OSString* XboxOneController::newTransportString() const
{
	return OSString::withCString("USB Proprietary Class");
}

OSNumber* XboxOneController::newPrimaryUsageNumber() const
{
	// We already give this information in the report descriptor, so take it from there.
	// Just make a paranoid check that we're returning the correct thing. This can be
	// verified at compile-time, so there's no good reason to not do it.
	constexpr size_t usagePageOffset = 3;
	static_assert(XboxOneControllerReportDescriptor[usagePageOffset - 1] == 0x09,
				  "Constant offset into XboxOneControllerReportDescriptor does not point to the usage number");
	return OSNumber::withNumber(XboxOneControllerReportDescriptor[usagePageOffset], 8);
}

OSNumber* XboxOneController::newPrimaryUsagePageNumber() const
{
	// See newPrimaryUsageNumber for explanation of static_assert.
	constexpr size_t usagePageOffset = 1;
	static_assert(XboxOneControllerReportDescriptor[usagePageOffset - 1] == 0x05,
		"Constant offset into XboxOneControllerReportDescriptor does not point to the usage page number");
	return OSNumber::withNumber(XboxOneControllerReportDescriptor[usagePageOffset], 8);
}

OSNumber* XboxOneController::newLocationIDNumber() const
{
	return OSNumber::withNumber(_controller->GetLocationID(), 32);
}

bool XboxOneController::handleStart(IOService *provider)
{
	// The `handleStart` method is responsible for:
	//
	//  1. verifying that we got the expected kind of object;
	//  2. keep a reference to the provider for later use, and try to open it;
	//  3. set the device configuration;
	//  4. find the interfaces that we want to use;
	//  5. allocate the necessary resources;
	//  6. return true on success, or false on failure.
	//
	// For implementation reasons, we're going to mix steps 2 and 3 together. This is not necessary.
	
	// Apple says to call super::handleStart at the *beginning* of the method.
	if (!super::handleStart(provider))
	{
		return false;
	}
	
	// The advantage of a `do ... while (false)` block over labels and `goto`s is that
	// you do not have "protected scope" problems when you break to skip over everything else.
	// With a `goto`, you cannot skip over variable declarations with non-trivial initialization.
	do
	{
		// -- (1) do we have the correct kind of object?
		IOUSBDevice* device = OSDynamicCast(IOUSBDevice, provider);
		if (device == nullptr)
		{
			IO_LOG_DEBUG("Provider is not an IOUSBDevice.");
			break;
		}
		
		// At this point, because of the matching dictionary, we can assume that the device is an Xbox One controler.
		
		// -- (2-3) set device configuration. Drivers are expected to do that, so even if we fail there is no
		// reason to revert back: the next driver on the chain will do that for us.
		
		// From this point on, we need to open the controller device. We want to keep a reference to it only
		// if this works.
		if (!device->open(this))
		{
			IO_LOG_DEBUG("Couldn't open controller.");
			break;
		}
		
		// keep the reference around.
		_controller = device;
		_controller->retain();
		
		// Get configuration #1 and set it as active.
		if (_controller->GetNumConfigurations() != 1)
		{
			IO_LOG_DEBUG("Controller does not hve exactly one configuration.");
			break;
		}
		
		const IOUSBConfigurationDescriptor* configuration = _controller->GetFullConfigurationDescriptor(0);
		if (configuration == nullptr)
		{
			IO_LOG_DEBUG("Couldn't get configuration from controller.");
			break;
		}
		
		// Set configuration now that we 'own' the device.
		IOReturn ior = device->SetConfiguration(this, configuration->bConfigurationValue, true);
		
		// Fun story: IOReturn is actually formatted as an HRESULT. Yes, a bona fide, Microsoft COM-style HRESULT.
		// If we wanted to, we could even use the SUCCEEDED macro here. It turns out that Apple reimplemented some
		// of this stuff.
		// I had enough COM this summer for an entire lifetime, so I'll just compare this to kIOReturnSuccess (0),
		// but don't be surprised if you stumble upon that.
		if (ior != kIOReturnSuccess)
		{
			IO_LOG_DEBUG("Couldn't configure device: %08x\n", ior);
			break;
		}
		
		// -- (4) find the correct interface. The Xbox One controller exposes 3 interfaces of class 0xff ("Vendor
		// specific"), which means that all bets are off with the meaning of the subclass and protocol. My first-gen
		// Xbox One controller reports three interfaces of subclass 0x47 and protocol 0xd0, and I know from other
		// smart people's work (https://github.com/jedi22/PoorMansXboxOneControllerDriver/blob/master/first.c) that
		// the interface on which the controller talks is the first one. We need to ask IOKit to enumerate the
		// interfaces that match this info.
		IOUSBFindInterfaceRequest findRequest = {
			.bInterfaceClass = 0xff,
			.bInterfaceSubClass = 0x47,
			.bInterfaceProtocol = 0xd0,
			.bAlternateSetting = kIOUSBFindInterfaceDontCare
		};
		
		_controllerInterface = _controller->FindNextInterface(nullptr, &findRequest);
		if (_controllerInterface == nullptr)
		{
			IO_LOG_DEBUG("No interface matching class=%02x, subclass=%02x, protocol=%02x found.",
				findRequest.bInterfaceClass, findRequest.bInterfaceSubClass, findRequest.bInterfaceProtocol);
			break;
		}
		
		_controllerInterface->retain();
		if (!_controllerInterface->open(this))
		{
			IO_LOG_DEBUG("Couldn't open interface.");
			break;
		}
		
		// The controller has three pipes:
		// * the control pipe;
		// * the input pipe;
		// * the output pipe.
		//
		// We're not particularly interested in the control pipe, so we don't keep a reference to it. On the other
		// hand, we very much want a reference to the two other ones.
		IOUSBFindEndpointRequest pipeRequest = {
			.type = kUSBInterrupt,
		};
		
		// "Input" and "output" are also sort of ambiguous, because they depend on your perspective. From the computer
		// standpoint, the input pipe is the controller's output pipe. To avoid confusion, we'll use "pipe from
		// controller" and "pipe to controller".
		pipeRequest.direction = kUSBIn;
		_pipeFromController = _controllerInterface->FindNextPipe(nullptr, &pipeRequest);
		if (_pipeFromController == nullptr)
		{
			IO_LOG_DEBUG("Cannot find pipe for data from controller.");
			break;
		}
		_pipeFromController->retain();
		
		pipeRequest.direction = kUSBOut;
		_pipeToController = _controllerInterface->FindNextPipe(nullptr, &pipeRequest);
		if (_pipeToController == nullptr)
		{
			IO_LOG_DEBUG("Cannot find pipe to controller.");
			break;
		}
		_pipeToController->retain();
		
		// -- (5) allocate resources for the driver.
		const IOUSBEndpointDescriptor* pipeDescriptor = _pipeFromController->GetEndpointDescriptor();
		if (pipeDescriptor == nullptr)
		{
			IO_LOG_DEBUG("Cannot get read buffer size for pipe from controller.");
			break;
		}
		
		_readBuffer = IOBufferMemoryDescriptor::inTaskWithOptions(kernel_task, 0, pipeDescriptor->wMaxPacketSize);
		if (_readBuffer == nullptr)
		{
			IO_LOG_DEBUG("Cannot allocate read buffer.");
			break;
		}
		
		// -- (3 again) Everything's in order now. Tell the controller that it can start working.
		// This is the only place in the driver, currently, where we write to a pipe. Normally you would have
		// some ceremony around that and we would allocate just one buffer (since IOMemoryBuffers are rather
		// heavyweight), but since we're doing this just once, it's okay to do everything here.
		IOMemoryDescriptor* hello = IOBufferMemoryDescriptor::inTaskWithOptions(kernel_task, 0, 2);
		if (hello == nullptr)
		{
			IO_LOG_DEBUG("Could not allocate buffer for hello message.");
			break;
		}
		hello->writeBytes(0, XboxOneControllerHelloMessage, sizeof XboxOneControllerHelloMessage);
		
		// The Write method will error out (kIOReturnBadArgument) for intrrupt pipes if you pass it non-zero
		// timeout parameters.
		ior = _pipeToController->Write(hello, 0, 0, hello->getLength());
		hello->release();
		if (ior != kIOReturnSuccess)
		{
			IO_LOG_DEBUG("Could not send hello message to controller: %08x", ior);
			break;
		}
		
		// From this point on, the controller will send messages to the computer. We need to read them!
		ior = beginAsyncRead();
		if (ior != kIOReturnSuccess)
		{
			IO_LOG_DEBUG("Could not start async read operation: %08x", ior);
			break;
		}
		
		// Well, that's all for initialization. Thanks folks!
		return true;
	}
	while (false);
	
	// Something failed, so release everything and return `false`. `stop` will not be called, so everything
	// has to be cleaned up.
	releaseEverything();
	return false;
}

void XboxOneController::handleStop(IOService *provider)
{
	// TODO what's the message to stop a controller?
	// Apple says we should call `super::handleStop` at the **end** of the method.
	super::handleStop(provider);
}

void XboxOneController::free()
{
	releaseEverything();
	
	// Apple says that you should call `super::free` **after** you've deallocated your stuff.
	super::free();
}

void XboxOneController::releaseEverything()
{
	if (_controller != nullptr)
	{
		_controller->close(this);
		_controller->release();
		_controller = nullptr;
	}
	
	if (_controllerInterface != nullptr)
	{
		_controllerInterface->close(this);
		_controllerInterface->release();
		_controllerInterface = nullptr;
	}
	
	if (_pipeFromController != nullptr)
	{
		_pipeFromController->release();
		_pipeFromController = nullptr;
	}
	
	if (_pipeToController != nullptr)
	{
		_pipeToController->release();
		_pipeToController = nullptr;
	}
	
	if (_readBuffer != nullptr)
	{
		_readBuffer->release();
		_readBuffer = nullptr;
	}
}

IOReturn XboxOneController::beginAsyncRead()
{
	IOUSBCompletion callback = {
		.target = this,
		.parameter = nullptr,
		.action = &finishedAsyncRead
	};
	
	// A timeout of zero means "no timeout".
	return _pipeFromController->Read(_readBuffer, 0, 0, _readBuffer->getLength(), &callback);
}

void XboxOneController::finishedAsyncRead(void *target, void *parameter, IOReturn status, UInt32 bufferRemaining)
{
	XboxOneController* that = static_cast<XboxOneController*>(target);
	if (status == kIOReturnOverrun)
	{
		IOReturn ior = that->_pipeFromController->ClearStall();
		if (ior == kIOReturnSuccess)
		{
			status = kIOReturnSuccess;
		}
		else
		{
			IO_LOG_DEBUG("Pipe stalled but couldn't be cleared: %08x", ior);
			return;
		}
	}
	
	if (status == kIOReturnNotResponding)
	{
		// Device unplugged
		IO_LOG_DEBUG("Device not responding");
		return;
	}
	
	if (status != kIOReturnSuccess)
	{
		IO_LOG_DEBUG("Async read failed: %08x", status);
		return;
	}
	
	// Start a new async read.
	IOReturn ior = that->beginAsyncRead();
	if (ior != kIOReturnSuccess)
	{
		IO_LOG_DEBUG("Couldn't start async read: %08x. Driver will stop functioning.", ior);
		return;
	}
	
	// Is it an update packet?
	const UInt8* buffer = static_cast<const UInt8*>(that->_readBuffer->getBytesNoCopy());
	if (buffer == nullptr || that->_readBuffer->getLength() < 1)
	{
		IO_LOG_DEBUG("No buffer?!");
		return;
	}
	
	// If so, process it.
	if (buffer[0] == 0x20)
	{
		// Process packet.
		IOReturn ior = that->handleReport(that->_readBuffer);
		if (ior != kIOReturnSuccess)
		{
			IO_LOG_DEBUG("Couldn't correctly handle controller report: %08x", ior);
		}
	}
}
