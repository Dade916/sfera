Sfera
=====

Sfera is a game, mostly written using OpenCL and OpenGL, using real-time path
tracing rendering. It includes a Bullet Physics based engine.

Check Sfera Web site and Wiki at http://code.google.com/p/sfera/ for more information.


Features
========

Sfera supports:

    * Multiple render engines: mono-thread, multi-thread, mono-GPU, multi-GPUs;
    * Multiple materials (all can emit light): matte, mirror, glass, metal, alloy;
    * Texture mapping;
    * Bump mapping;
    * Depth of field;
    * A text SDL (Scene Description Language) for levels;
    * Multi-platforms (i.e. Linux, MacOS, Windows, etc.);
    * A single awesome primitive supported: sphere;


Game Controls
=============

Puppet controls

    * 'w' to move forward;
    * 's' to stop;
    * 'a' to turn left;
    * 'd' to turn right;
    * space bar to jump.

Camera controls

    * press left mouse button and move the mouse (while still pressing left button) to turn the camera around the puppet;
    * press right mouse button and move the mouse up/down (while still pressing right button) to translate the camera near/far from the puppet;
    * press 'f' key, left mouse button and move the mouse (while still pressing 'f' and left button) to look around;

Others

    * 'ESC' to exit from level/game;


Authors
=======

See AUTHORS.txt file.

Sfera includes many classes from LuxRender and LuxRays (http://www.luxrender.net)


Credits
=======

A special thanks goes to:

    * http://www.bulletphysics.org for the physic engine;
    * http://freeimage.sourceforge.net for open source image library;
    * http://www.libsdl.org for cross-platform multimedia library;
    * http://www.hdrlabs.com/sibl/archive.html for HDR maps;
    * http://freebitmaps.blogspot.com for planet texture maps;

This software is released under GPL v3 License (see COPYING.txt file).
