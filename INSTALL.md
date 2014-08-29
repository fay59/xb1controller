Building, Installing and Debugging XboxOneControllerDriver
===================================================================

If you are not a developer, just head to the [Releases][1] page, grab the
installer package and double-click it. You're done! No reboot required!

Developer Foreword
------------------

Please note that Mac OS X makes it harder to load unsigned kernel extension. If
you are not a member of the Mac Developer Program, you will not be able to sign
the extension and Mac OS X will give you all sorts of pain for it.

If you want it to be automatically loaded, you will need to place it in
`/System/Library/Extensions` instead of `/Library/Extensions` (which is bad form
because it's not part of the normal system distribution) and you will get a
warning each time you boot that the extension is not signed.

Your other option will be to load it yourself on demand before using your
controller (which won't absolve you of the warning).

Building
--------

Building a kext is the same as building a normal application. Launch Xcode, hit
Build, or use `xcodebuild` from the command-line.

Installing
----------

Once you got the built extension, you first need to **copy** it somewhere else
convenient, like `/tmp`. You need to change its permissions, and if you do it
on the copy from the build directory, you will have permission issues
rebuilding.

    $ sudo cp -R ~/Library/Developer/Xcode/DerivedData/XboxOneControllerDriver-*/Build/Products/Debug/XboxOneControllerDriver.kext /tmp
    $ sudo chown -R root:wheel /tmp/XboxOneControllerDriver.kext
    $ sudo chmod -R 0755 /tmp/XboxOneControllerDriver.kext

You can copy it to the `/Library/Extensions` directory if you want to install it
permanently (if it is signed) and load it.

    $ sudo mv /tmp/XboxOneControllerDriver.kext /Library/Extensions
    $ sudo touch /Library/Extensions

Do **NOT** do this if you made changes and want to test stability! These
extensions are loaded at boot time. You would need to start in [safe mode][2] to
remove it. See *Debugging* if you want to make changes and test them.

Debugging
---------

Since Mac OS X does not support local kernel debugging, it is advised that you
either get a second Mac (expensive) or a Mac OS X virtual machine (cheap) and do
remote debugging.

Apple has [official documentation][3] regarding kernel debugging, but it is
unfortunately very outdated. It dates to when Apple's developer website was
"connect.apple.com". `gdb` has been completely replaced with `lldb` on recent
systems and there is not a word about it. Either way, it is *not* possible to
debug kernel extensions from Xcode. You will need to use `lldb` from the
command-line, and this guide assumes that you're at least comfortable with that.
The [gdb to lldb command map][4] has just about all of the useful (normal)
commands in one place, and the in-program `help` command is quite decent too.

Parallels Desktop and VMware Fusion both support creating a Mac OS X guest by
starting up with your Mac's recovery partition. No idea about VirtualBox.

### Setting Up the Developer Environment

The developer machine is the one on which you build the extension and from which
you want to debug it. Go [download the Kernel Debug Kit][5] on the Apple
Developer website. Make sure that you get the one that matches your **target**
machine's operating system.

Mount the .dmg file and fire up lldb:

    xcrun lldb /Volumes/KernelDebugKit/mach_kernel

`Lldb` will warn you that `mach_kernel` comes with bundled debug scripts. You
want to import them, so be sure to run the command that it tells you to do. This
is **essential** to get kernel debugging commands.

### Setting Up the Target Machine

The target machine is the one you'll install the kext to (so the spare Mac or
the virtual machine). First, enable verbose boot and kernel debugging:

    $ sudo nvram boot-args="debug=0x144 -v"
    $ sudo shutdown -r now

Be sure to **write down the target machine's IP address** somewhere that you can
access it even if it is unresponsive.

Now copy over the kernel extension. The simplest way I've found is to enable
file sharing on the target machine, connect to it from the host machine and copy
the extension with simple drag-and-drop. Do the *Install* steps from the target
machine, and load the kernel extension.

### Breaking into kernel debugging

The normal way to break into kernel debugging is to [send a non-maskable
interrupt][6] to the target machine. We've already set the debug flags on the
kernel so you don't have any additional setup to do. It's super easy on a
physical machine but I haven't found a way to do it on a virtual machine (at
least with Parallels). This means that it may be impossible for you to break
into the kernel without actually crashing it.

When the kernel crashes and debugging is enabled, you get scary overlayed text
that describes the issue. The final lines will be something along the lines of
"waiting for debugger...". This is your cue that you can connect to the machine
with `lldb`.

    (lldb) kdp-remote xx.yy.zz.ww

Next you want to find the kernel extension's load address:

    (lldb) showallkmods

Look for it in the table that it prints and write down the `address` value. Next
we will tell lldb where it can find the symbols for the kext (where `$ADDRESS`
is the address that you found):

    (lldb) target modules load --file XboxOneControllerDriver --slide $ADDRESS

`Lldb` now knows about the kernel extension's symbols. Enjoy your debugging.

  [1]: https://github.com/zneak/xb1controller/releases
  [2]: http://support.apple.com/kb/HT1564
  [3]: https://developer.apple.com/librarY/mac/documentation/Darwin/Conceptual/KEXTConcept/KEXTConceptDebugger/debug_tutorial.html
  [4]: http://lldb.llvm.org/lldb-gdb.html
  [5]: https://developer.apple.com/downloads/index.action
  [6]: https://developer.apple.com/library/mac/qa/qa1264/_index.html