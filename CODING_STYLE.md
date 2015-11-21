
Firmware commands name rules
==================
Commands name rules are used by [tokeline](https://github.com/biot/tokenline) "t_token_dict" like for example in https://github.com/bvernoux/hydrafw/blob/master/hydrabus/commands.c

basic rules: no capital letters or _ in keywords please, this keeps it consistent use - for space instead, like write-mode or whatever


C code conventions
==================
When writing new C code, please adhere to the following conventions.

Coding Style of the project: Linux Kernel coding style see: https://www.kernel.org/doc/Documentation/CodingStyle

#Configuration of Linux Kernel coding style with AStyle
Can be also done automatically with open source tool AStyle: http://astyle.sourceforge.net/
With following syntax: AStyle.exe -t --style=linux --lineend=linux my_c_file.c

Syntax tested with AStyle_2.05.1

#Configuration of Linux Kernel coding style with [EmBitz](http://www.emblocks.org/web/downloads-main)
##Menu Settings -> Editor -> General settings:
![General settings](http://hydrabus.com/EmBlocks_CodingStyle_GeneralSettings.png)

##Menu Settings -> Editor -> Source fomatter Tab Style:
![Source fomatter Tab Style](http://hydrabus.com/EmBlocks_CodingStyle_SourceFormatter_Style.png)

##Menu Settings -> Editor -> General settings Tab Indentation:
![Source fomatter Tab Indentation](http://hydrabus.com/EmBlocks_CodingStyle_SourceFormatter_Indentation.png)
