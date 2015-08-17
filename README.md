# XBeat

XBeat is a PC clone of the game Project Diva, created originally for the Sony&reg; PlayStation Portable&copy;.

## License
This software is licensed under the University of Illinois Open Source License.

See LICENSE.txt for details.

## Computer Requisites

  - Windows 7 or higher (tested on 8.1 and 10)
  - A DirectX 11 compatible graphics adapter, and it is recommended to use the latest available graphics driver, for best results.
  - A DirectAudio compatible sound card is also required.
  - A XInput compatible gamepad is recommended, but is not required (you may use a keyboard).


## Using
The source code distribution does not come with the data files, so you will need either to download them separatedly or create your own!

The following sections will describe how to create your own data files.

### Model files
Adding a new model is a really simple task!

All you have to do is drop its files into the Data/Models folder and the game will reload the cache automatically when either you enter the main menu or when you enter the model selection screen!

### Loading Screens
Every image file that can be loaded on Windows (i.e., when you click an image on Windows Explorer and it can be previewed or it has a thumbnail) may be used as a loading screen.
This means that the most common file types can be used, like PNG, JPEG, BMP and others not-so-common, like TGA.

To add a loading screen to the list of available screens, drop the desired images into the Data/Textures/Loading folder and the game will use them on the next loading screen!
Note that *it can be done even while the game is running*, and it might be used on the next loading screen!

### Main Menu
For the main menu, a random model will be used from the list of known models. Also, every few seconds a new idle animation will be used.

To add a new idle, just drop a VMD motion file into the Data/Motions/Idle folder.

## For Developers 

### Project Status

Currently, the project is under development and is not in a playable state yet.

The features that are implemented so far are:

  - PMX 2.0 and 2.1 model support, with a manager to list all available models
  - Morph types supported:
	- Bone
	- Flip (PMX 2.1 exclusive)
    - Group
    - Impulse (PMX 2.1 exclusive)
    - Material
    - Vertex
  - Physics support (PMX 2.1 soft bodies not yet supported)
  - OBJ for static meshes
  - PostProcessing effect (only for the entire scene, to be done for each model)
  - Bone deformation
  - Vertex skinning on GPU
  - VMD motion, including camera, bone transformations and morphs

### Compiling

In order to compile, you need a C++11 enabled compiler - Microsoft Visual Studio 2013 or 2015 is recommended and a solution for 2015 is available with the source code.

Also, there are a few external libraries. When cloning the repository, all of the following dependencies are cloned together:

  - Bullet Physics 3
  - DirectXTex
  - DirectXTK
  - FX11

There is also the need for the following libraries, not included on GIT:

  - Boost 1.55 or higher, available [here][1]
  
### Coding Standards

To make the source code more readable, we are (at least, trying to :P) adopting the LLVM coding standards, which can be seen [here][2].

When submitting a patch or doing a merge request, please follow these standards.
  
[1]: http://www.boost.org/
[2]: http://llvm.org/docs/CodingStandards.html
