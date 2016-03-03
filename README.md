libCppEvent
===========

## Overview

[libCppEvent](https://github.com/zhanggyb/libCppEvent) is a simple C++11 library
implementing event/delegate based on fast C++ delegates, inspired by:

- [Member Function Pointers and the Fastest Possible C++ Delegates](http://www.codeproject.com/Articles/7150/Member-Function-Pointers-and-the-Fastest-Possible)
by Don Clugston
- [The Impossibly Fast C++ Delegates](http://www.codeproject.com/Articles/11015/The-Impossibly-Fast-C-Delegates) by Sergey Ryazanov
- [Fast C++ Delegate: Boost.Function 'drop-in' replacement and multicast](http://www.codeproject.com/Articles/18389/Fast-C-Delegate-Boost-Function-drop-in-replacement) by JaeWook Choi
- [CppEvents](http://code.google.com/p/cpp-events/) by Nickolas V. Pohilets

Event/delegate has been proven very useful in object-oriented programming, especially GUI application. libCppEvent provides such implementation for communication between objects in a simple way as illustrated below:

<div  align="center">
<img src="https://github.com/zhanggyb/libCppEvent/blob/master/doc/graphics/example.png" width = "400" alt="Example" align=center />
</div>

When an instance of Subject has something changed, it may want its observer to be notified. It can fire events to invoke the connected member function in observers. The events can be connected or disconnected when needed. When a Subject or Observer is destroyed, all corresponding connections will be broken too.

For more information, see the [Wiki page](https://github.com/zhanggyb/libCppEvent/wiki).

## Download and Install

* Checkout this repository
* Copy *include/cppevent* and *src/\** into your C++ project

## Known Issue

This project currently does not support MSVC.(FIXME)

## License

This project is licensed under MIT License, which can be found in the LICENSE
file. The license is also included at the top of each source file.
