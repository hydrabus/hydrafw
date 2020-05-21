
Firmware commands name rules
==================
Commands name rules are used by [tokeline](https://github.com/biot/tokenline) "t_token_dict" like for example in https://github.com/bvernoux/hydrafw/blob/master/hydrabus/commands.c

basic rules: no capital letters or _ in keywords please, this keeps it consistent use - for space instead, like write-mode or whatever

Do not use reserved keywords for files (like CON, PRN, AUX, NUL, COM1, COM2, COM3, COM4, COM5, COM6, COM7, COM8, COM9, LPT1, LPT2, LPT3, LPT4, LPT5, LPT6, LPT7, LPT8, and LPT9)
This is documented on:
https://docs.microsoft.com/en-us/windows/desktop/FileIO/naming-a-file


C code conventions
==================
When writing new C code, please adhere to the following conventions.

Coding Style of the project: Linux Kernel coding style see: https://www.kernel.org/doc/Documentation/CodingStyle

# Configuration of Linux Kernel coding style with AStyle
Can be also done automatically with open source tool AStyle: http://astyle.sourceforge.net/
With following syntax: AStyle.exe -t --style=linux --lineend=linux my_c_file.c

Syntax tested with AStyle_2.05.1

# Configuration of Linux Kernel coding style with Eclipse(Including Atollic TrueSTUDIO for STM32 or STM32CubeIDE)/Qt
See details here https://github.com/PanderMusubi/linux-kernel-code-style-formatters
