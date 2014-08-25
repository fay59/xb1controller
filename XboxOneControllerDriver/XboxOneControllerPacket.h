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

#ifndef XBOX_ONE_CONTROLLER_PACKET_H
#define XBOX_ONE_CONTROLLER_PACKET_H

#include <libkern/OSTypes.h>

// Layout taken from https://github.com/bkase/xbox-one-fake-driver/blob/master/xbox.go
// For reference:
// 0x20 0x00 ...
// _    = data[0] // sequence number
// _    = data[1] // unknown
// btn1 = data[2] // yxbaSM?N S=share, M=Menu, N=Sync
// _    = btn1
// btn2 = data[3] // rlrlRLDU r=R-Stick/Trigger, l=L-Stick/Trigger, R=D-Right, L=D-Left, D=D-Down, U=D-Up
// _    = btn2
//
// lt = binary.LittleEndian.Uint16(data[4:6]) // left trigger, 0..1024
// rt = binary.LittleEndian.Uint16(data[6:8]) // right trigger
//
// lx = int16(binary.LittleEndian.Uint16(data[8:10]))  // left stick X, +/- 32768 or so
// ly = int16(binary.LittleEndian.Uint16(data[10:12])) // left stick Y
// rx = int16(binary.LittleEndian.Uint16(data[12:14])) // right stick X
// ry = int16(binary.LittleEndian.Uint16(data[14:16])) // right stick Y

struct [[gnu::packed]] XboxOneControllerPacketHeader
{
	UInt8 sequenceNumber;
	UInt8 unknown;
};

union [[gnu::packed]] XboxOneControllerPacketButtons1
{
	UInt8 bits;
	struct
	{
		UInt8 y : 1;
		UInt8 x : 1;
		UInt8 b : 1;
		UInt8 a : 1;
		UInt8 share : 1;
		UInt8 menu : 1;
		UInt8 view : 1;
		UInt8 sync : 1;
	};
};

union [[gnu::packed]] XboxOneControllerPacketButtons2
{
	UInt8 bits;
	struct
	{
		UInt8 rightStick : 1;
		UInt8 leftStick : 1;
		UInt8 rightTrigger : 1;
		UInt8 leftTrigger : 1;
		UInt8 dRight : 1;
		UInt8 dLeft : 1;
		UInt8 dDown : 1;
		UInt8 dUp : 1;
	};
};

struct [[gnu::packed]] XboxOneControllerPacketHat
{
	UInt16 x;
	UInt16 y;
};

struct [[gnu::packed]] XboxOneControllerPacket
{
	XboxOneControllerPacketHeader header;
	XboxOneControllerPacketButtons1 buttons1;
	XboxOneControllerPacketButtons2 buttons2;
	UInt16 leftTrigger;
	UInt16 rightTrigger;
	XboxOneControllerPacketHat leftStick;
	XboxOneControllerPacketHat rightStick;
};

#endif
