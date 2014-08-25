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

  [1]: https://github.com/kylelemons/xbox/
  [2]: http://en.wikipedia.org/wiki/Magic_smoke
  [3]: https://github.com/bkase/xbox-one-fake-driver