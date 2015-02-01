Xbox One Controller Driver
==========================

This project packages an Xbox One controller driver for Mac OS X built on top
of the `IOKit` framework. It is a true kernel mode driver, meaning that programs
built with the user-facing `IOKit` API will recognize the controller (but also
meaning that the driver needs wired memory for its code, and that a failure
will bring down the whole operating system).

It is based on the documentation found on kylelemons's [xbox][1] and bkase's
[xbox-one-fake-driver][3] repositories, though it shares no common code.

The Xbox One controller is not HID-compliant, and this is the reason that
generic drivers aren't sufficient. However, it's not *too far* from a compliant
device. The three main differences are that:

* the device needs to be "turned on" by the computer (which is done by sending
	it `05 20`);
* the device sends more than just HID reports on the interrupt pipe;
* the device does not expose a report descriptor.

This driver attempts to bridge these three shortcomings.

Status
------

In my book, this driver is ready for production. This means that it is useful
for actual gaming and that people are unlikely to have problems with it.

The driver only works when the controller is connected with a USB cable to the
Mac. I believe that the controller does not use Bluetooth, which makes it
unlikely to ever work unless Microsoft releases some USB dongle.

Technicalities
--------------

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

The driver does not expose rumble motors. From badgio's [XboxOneController][4]
project, it appears that the magic byte sequence to send to the controller is:

    09 00 00 09 00 0f .w .x .y .z ff 00 00

where `.w` and `.x` are values for the left and right triggers, and `.y` and
`.z` are values for the left and right handles, respectively. So on the
controller's side, force feedback is fairly simple. The problem is that from the
only [example implementation of force feedback][5] I could find, it appears that
the work required on the software side is [non-trivial][6]. Not to mention that
I know of [just one application][7] that takes advantage of it.


  [1]: https://github.com/kylelemons/xbox/
  [2]: http://en.wikipedia.org/wiki/Magic_smoke
  [3]: https://github.com/bkase/xbox-one-fake-driver
  [4]: https://github.com/badgio/XboxOneController
  [5]: http://tattiebogle.net/index.php/ProjectRoot/Xbox360Controller/OsxDriver
  [6]: https://developer.apple.com/library/mac/documentation/ForceFeedback/Reference/IOForceFeedbackLib_header_reference/Reference/reference.html#//apple_ref/doc/uid/TP40011602
  [7]: http://sixtyforce.com/help/controllers.html
