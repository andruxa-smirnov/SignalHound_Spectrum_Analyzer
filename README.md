Spike Spectrum Analysis Software
=====

This is the code in Qt which I took from https://github.com/SignalHound/BBApp .

It is the official Github page for Signal Hound's spectrum analyzer software Spike. 
See *www.signalhound.com* for more information, and for the users manual for this software.

Spike is built using the 64-bit Qt 5.2.1 Desktop OpenGL libraries. 
Spike has also been built using the 32-bit Qt 5.3.0 Desktop OpenGL libraries.
Other versions are likely to be compatible *(OpenGL only)* but untested. 
VS2012 or a later compiler is required, for the use of C++11 features. 
The application is for Windows only as it relies on the Signal Hound product APIs, which are C++ DLLs for operating Signal Hound's spectrum analyzers.
