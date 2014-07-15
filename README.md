LEDSign
=======

Visual demos for 16x64 LED sign, mostly cobbled from other programs and Arduino examples, this code runs on Raspberry Pi

[![Plasma demo](http://img.youtube.com/vi/KiPLsGjKtl8/0.jpg)](http://www.youtube.com/watch?v=KiPLsGjKtl8)

[![Hackspace sign](http://img.youtube.com/vi/Se_DR5I9aaM/0.jpg)](http://www.youtube.com/watch?v=Se_DR5I9aaM)

Usage
-----
Clone repository into a directory on Raspberry Pi.

Run 'make' to build the program

Run 'sudo ./led-sign 0' for a rotating block demo

Run 'sudo ./led-sign 1 yhs-scroll.ppm' for a scrolling bitmap demo (build any bitmap in GIMP and export as PPM)

Run 'sudo ./led-sign 2' for a simple box

Run 'sudo ./led-sign 3' for a particle system demo

Run 'sudo ./led-sign 4' for a colour plasma demo

Run 'sudo ./led-sign 5' for a simple colour cycle

Run 'sudo python message.py' for a Python demo
