Xbox One Controller Driver
==========================

This project packages an Xbox One controller driver for Mac OS X built on top
of the `IOKit` framework. It is a true kernel mode driver, meaning that programs
built with the user-facing `IOKit` API will recognize the controller (but also
meaning that the driver needs wired memory for its code, and that a failure
will bring down the whole operating system).

It is based on the documentation found on kylelemons's [xbox][1] and bkase's
[xbox-one-fake-driver][3] repositories (though it shares no common code).

The Xbox One controller is not HID-compliant, and this is the reason that
generic drivers aren't sufficient. However, it's not *too far* from a compliant
device. The three main differences are that:

* the device needs to be activated by the controller (which is done by sending
	it `05 20`);
* the device sends more than just HID reports on the interrupt pipe;
* the device does not have a report descriptor.

This driver attempts to bridge these three shortcomings.

Being fairly new to OS X driver development and USB devices, I hope that I did
it right. If not, please don't blame me for bugs, character deaths from
incorrect input translation, data loss from system crashes or [magic smoke][2]
escaping your controller or expensive computer.

Status
------

The driver only works when the controller is connected with a USB cable to the
Mac. I haven't figured out wireless. If it doesn't use Bluetooth, it might not
even be possible.

The driver exposes the controller as a HID gamepad whose name is "Controller"
(that's Microsoft's controller guys' fault, not mine, though it wouldn't be very
hard to change). This gamepad has 16 buttons and six axes:

* **Button 1**: Sync 
* **Button 2**: ?
* **Button 3**: Menu ("start")
* **Button 4**: View ("select")
* **Button 5**: A
* **Button 6**: B
* **Button 7**: X
* **Button 8**: Y
* **Button 9**: D-Up
* **Button 10**: D-Down
* **Button 11**: D-Left
* **Button 12**: D-Right
* **Button 13**: Left Bumper
* **Button 14**: Right Bumper
* **Button 15**: Left Thumb Stick Press
* **Button 16**: Right Thumb Stick Press
* **Axes X, Y**: Left Thumb Stick
* **Axes Rx, Ry**: Right Thumb Stick
* **Axis Z**: Left Trigger
* **Axis Rz**: Right Trigger

You'll notice that the triggers are considered axes. This is because the Xbox
One controller has analog triggers with values varying between 0 and 1023.
These are not buttons (in the sense that buttons can only be either on or off).

The driver does not expose rumble motors, but that would be neat. If anyone has
any info on the magic words to send, that would be awesome.

The driver does not expose force feedback on the triggers either, but I don't
think that any non-Xbox game uses that.

  [1]: https://github.com/kylelemons/xbox/
  [2]: http://en.wikipedia.org/wiki/Magic_smoke
  [3]: https://github.com/bkase/xbox-one-fake-driver