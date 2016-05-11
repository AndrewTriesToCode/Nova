# Nova

![Screenshot](https://github.com/achandlerwhite/Nova/blob/master/nova_screenshot.PNG "Screenshot")

Nova is a simple 3D renderer written as a learning exercise with the following features:

* Portable and idiomatic C code (C99)

* All 3d math and rendering done in software (i.e. no outside libraries used)

* Matrix and vector data structures with support for rotations, translations, perspective projections

* Adjustable render size and field-of-view

* Perspective correct texture mapping

* Vertex based lighting

* Barymetric based traiangler rasterization

* Loading of .obj, .mtl, and .bmp files

* Project files for Visual Studio 2015 and XCode 7
  * Windows app features-
    * Basic Win32 functionality
    * Direct2D used to render raw pixel buffer to window
    * FPS calculation
    
  * OS X app features-
    * Written in Objective-C
    * Basic Cocoa functionality
    * CoreGraphics used to render raw pixel buffer to window
    * Time independent animation (unlike the windows app, this version rotates the model at the same rate no matter the FPS)
