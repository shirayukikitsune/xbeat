XBeat
=====

XBeat is a PC clone of the game Project Diva, created originally for the Sony(r) PlayStation Portable(c).

Computer Requisites
-------------------

  - To be able to run, it is required a DirectX 11 compatible graphics adapter, and it is recommended to use the latest available graphics driver, for best results.
  - A DirectAudio compatible sound card is also required.
  - A XInput compatible gamepad is recommended, but is not required (you may use a keyboard).

Project Status
--------------

Currently, the project is under development and is not in a playable state yet.

The features that are implemented so far are:

  - PMX 2.0 and 2.1 model support, with a manager to list all available mods
  - Physics support (PMX 2.1 soft bodies not yet supported)
  - OBJ for static meshes
  - PostProcessing effect (only for the entire scene, to be done for each model)
  - Bone deformation
  - Vertex skinning on GPU

Compiling
---------

In order to compile, you need a C++11 enabled compiler - Microsoft Visual Studio 2013 is recommended and its solution is available with the source code.

There are some dependencies included with the source code, but it is also needed some external libraries:

  - Boost 1.55 or higher, available [here][1]
  - Bullet Physics 2.82 or higher, available [here][2]
  
[1]: http://www.boost.org/
[2]: http://bulletphysics.org/
