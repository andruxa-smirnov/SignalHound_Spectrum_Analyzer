Spike Spectrum Analysis Software
=====

This is the official Github page for Signal Hound's spectrum analyzer software Spike. 
See *www.signalhound.com* for more information, and for the users manual for this software.

Note: The original BBApp has been renamed Spike. 
Until the project is fully renamed, you will see references to "BBApp".

This is the complete source tree for Spike. 
This project is a great place to start if you need to understand how the software works or extend our application for your custom needs.

Spike is built using the 64-bit Qt 5.2.1 Desktop OpenGL libraries. 
Spike has also been built using the 32-bit Qt 5.3.0 Desktop OpenGL libraries.
Other versions are likely to be compatible *(OpenGL only)* but untested. 
VS2012 or a later compiler is required, for the use of C++11 features. 
The application is for Windows only as it relies on the Signal Hound product APIs, which are C++ DLLs for operating Signal Hound's spectrum analyzers.

For users new to Qt 
- Visit the Qt Project website.
- Download and install the latest version of Qt Creator.
- Download and install the Qt 5.2.1 Desktop OpenGL 64-bit libraries.
- Ensure you have the Visual Studio 2012 compiler installed.
- Install the cdb debugger if you wish (optional).
- Clone/Download the project from Github.
- Open the BBApp.pro file in the project directory
- In Qt Creator you must create a kit in order to compile the project. 
  Select the project tab on the left toolbar, select manage kits, then add and configure a new kit to use the Qt 5.2.1 libraries you installed, the VS compiler and a debugger if you wish. 
  Save your kit *(press Apply)*, then back out in the projects tab, add and select the new kit you created. 
- Pick a build directory and place bb_api.lib, bb_api.dll, sa_api.lib, and sa_api.dll in the build directory. 
  These files can be found in the api folder after installing our software from the website.
- Build and run.

Contact aj@signalhound.com for additional information.
