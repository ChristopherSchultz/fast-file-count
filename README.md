# fast-file-count
A program to quickly count files in a subdirectory. This is mostly a programming demonstration that arose from some sample code poasted to StackOverflow here: https://stackoverflow.com/questions/1427032/fast-linux-file-count-for-a-large-number-of-files/28368788#28368788

Contributions welcome!

Compiling
---------

Using GCC or LLVM, the code complies cleanly using this command:

$ cc -Wall -pedantic -o dircnt dircnt.c

Usage
-----

$ dircnt [dir]

Where 'dir' is the directory you want to scan for files. The default dir is "." (current directory).
