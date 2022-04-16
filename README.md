# m8mouse RGB and DPI controller

This is a small CLI tool to replicate the *M8 Mouse* windows 
application to control *Zi You Lang* devices which go by many
different brands/labels (Red Wolf, Ziyulang).

The M8 Mouse software used to control this is not downloadable 
from official vendor/whitelabel sources, but instead passed around
on online drives, so it made sense to distill the main functionality
out into something more open source.

This was tested on a Zi You Lang *T60 model* on Linux, but could
be extended to Windows/MAC as the hidapi library is cross platform.

## Device Identification

The main mcu on the T60 model is a Sunplus Innovation Technology
SPCP19X chip, which of course doesn't have lots of datasheets out
there, so most of this work came from tracing what M8 Mouse does.

The VID/PID of the chip is not enough to identify it is the right device
so instead we rely on checking the handshake and memory buffer to confirm

        //1bcf:Sunplus Innovation Technology Inc.
        //08a0:Gaming mouse [Philips SPK9304]


## Compiling and Running

### Dependencies

Main dependency for this tool is **hidapi** with the libusb backend.
Install the library (usually preinstalled) and headers for it before
compiling.

On debian that should be (note the -libusb0 for binaries)

    apt install libhidapi-libusb0 libhidapi-dev

### Compiling

Then it's standard cmake build workflow:

    cmake -H. -Bbuild -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_INSTALL_PREFIX=/usr/local -DCMAKE_BUILD_TYPE=Debug 
    cmake --build build

### Installing

Install globally with cmake

    # this installs the binary and the udev.rules file
    sudo cmake --build build --target install

#### Installing without sudo

If you prefer to avoid installing globally, you can use user bin directory
for the binary if you have that included on your path already.
You would also need to edit or copy the udev.rules file directly to
allow user access to the device

    cp -v build/m8mouse $HOME/bin
    sudo cp -v 90-m8mouse.rules /etc/udev/rules.d/

### Running 

First reload your udev rules and reconnect the device (or restart your PC)

    # reload udev rules
    sudo udevadm control -R

Then run m8mouse
    
    # show usage
    m8mouse -h
    # List known modes and settings
    m8mouse -l
    # Query device state
    m8mouse
    # Update device state
    m8mouse -dpi 1 -led 2 -speed 4


## Issues

The query/update cycle is pretty slow as the mouse mcu needs ~14ms between each set/get call.
In total a query is about 1.5s and a update is about 3.2s

Other functions like key mappings are also present in the queried configuration
but I didn't need them changed. If there is interest out there for allowing those
feel free to raise an issue to discuss.

## License

MIT license, see LICENSE file.

Logging framework included is here (used for debugging mostly, not cli):
https://github.com/rxi/log.c/

