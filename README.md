HydraFW official firmware for HydraBus/HydraNFC
========

HydraFW is a native C (and asm) open source firmware for HydraBus board with support of HydraNFC Shield.

You can Buy HydraBus/HydraNFC Online in Seeed Studio Online Shop:
http://www.seeedstudio.com/depot/HydraBus-m-132.html

![HydraBus+HydraNFC board](HydraBus_HydraNFC_board.jpg)

![HydraFW default pin assignment](http://hydrabus.com/HydraBus_1_0_HydraFW_Default_PinAssignment_A4.jpg)

For more details on HydraBus Hardware and Firmware see also: https://github.com/bvernoux/hydrabus

For more details on HydraNFC Hardware see: https://github.com/bvernoux/hydranfc

For HydraFW information see Wiki: https://github.com/bvernoux/hydrafw/wiki

For hydrafw usage with VT100 Terminal see wiki
https://github.com/bvernoux/hydrafw/wiki/HydraFW-commands

If you want to help on this project see the [Coding Styles](https://github.com/bvernoux/hydrafw/blob/master/CODING_STYLE.md), [Wiki](https://github.com/bvernoux/hydrafw/wiki) & [Wiki Task List](https://github.com/bvernoux/hydrafw/wiki/Task-List) 

##How to build, flash and use hydrafw on Windows:

###Required dependencies:

    Custom chibios (based on ChibiOS 3.0) included as Submodule.
    Custom microrl included as Submodule.
    
###Prerequisites for Windows:
Install git from http://msysgit.github.io

Install Python 2.7.x see https://www.python.org/downloads/windows
    
Open a Command Prompt window(cmd.exe) and type following commands:

    git clone https://github.com/bvernoux/hydrafw.git hydrafw
    cd hydrafw/
    git submodule init
    git submodule update
    scripts/Update_python_libraries.bat

###To build hydrafw firmware (with mingw or cygwin):

MinGW (http://www.mingw.org) is required (or Cygwin) and shall include make.
The firmware is set up for compilation with the GCC toolchain GNU_ARM_4_7_2013q3.

* Install GCC toolchain GNU_ARM_4_7_2013q3 from https://launchpad.net/gcc-arm-embedded/+milestone/4.7-2013-q3-update
* Install MinGW from http://sourceforge.net/projects/mingw/files/latest/download?source=files
    * `Default install: C:\MinGW\msys\1.0`
    * During MinGW install choose `msys-base` (it includes minimal tools and make 3.81)
    * Launch msys shell from `Default Install: C:\MinGW\msys\1.0\msys.bat`
```
    cd in root directory(which contains directories common, fatfs, hydrabus, hydranfc ...)
    make clean
    make
```

###To build hydrafw firmware (with Em::Blocks):
Install Em::Blocks (tested with Em::Blocks 2.30) from http://www.emblocks.org/web/downloads-main

Launch Em::Blocks and choose from Menu `File => Open... Ctrl-O` with file `hydrafwEm.ebp` which is in root of hydrafw/

Build the code with Menu `Build => Rebuild all target files`

###Flash and use hydrafw on Windows:
See the wiki https://github.com/bvernoux/hydrafw/wiki/Getting-Started-with-HydraBus

##How to build, flash and use hydrafw on Linux (Debian/Ubuntu):

###Prerequisites for Linux:

    cd ~
    sudo apt-get install git dfu-util python putty
    wget http://www.bialix.com/intelhex/intelhex-1.4.zip
    unzip intelhex-1.4.zip
    cd intelhex-1.4
    python setup.py install --user
    wget https://launchpad.net/gcc-arm-embedded/4.7/4.7-2013-q3-update/+download/gcc-arm-none-eabi-4_7-2013q3-20130916-linux.tar.bz2
    tar xjf gcc-arm-none-eabi-4_7-2013q3-20130916-linux.tar.bz2
    echo 'PATH=$PATH:~/gcc-arm-none-eabi-4_7-2013q3/bin' >> ~/.bashrc
    git clone https://github.com/bvernoux/hydrafw.git hydrafw
    cd ~/hydrafw
    git submodule init
    git submodule update
    cd ./scripts
    python get-pip.py
    python -m pip install GitPython
    python -m pip install intelhex --allow-external intelhex --allow-unverified intelhex
    cd ..

###Build hydrafw on Linux:

    cd ~/hydrafw
    make clean
    make

###Flash and use hydrafw on Linux:
See the wiki https://github.com/bvernoux/hydrafw/wiki/Getting-Started-with-HydraBus

