http://lucidfusionlabs.com/svn/lfl/new_app_template/README.txt
==============================================================

OVERVIEW
--------

Run: lfl/new_app_template/clone.sh <organization name> <package name> <binary name>

Eg: ./new_app_template/clone.sh com.lucidfusionlabs SpaceballFuture spaceball
    Creates ./spaceball/spaceball.cpp, ./spaceball/spaceball-android, ./spaceball/spaceball-iphone, etc

Append "add_subdiretory(spaceball)" to lfl/CMakeLists.txt.


MANIFEST
--------

- new_app_template.cpp:                   App source code
- assets/:                                All models, multimedia, databases, etc here  

- assets/icon.ico:                        Windows icon file
- assets/icon.icns:                       Mac icon file
- assets/icon.bmp:                        32x32 Bitmap icon file

        Icon converter: http://iconverticons.com/

- *-android/res/drawable-hdpi/icon.png:   72x72 Android icon
- *-android/res/drawable-mdpi/icon.png:   48x48 Android icon
- *-android/res/drawable-ldpi/icon.png:   36x36 Android icon
- *-iphone/Images/Icon.png:               57x57 iPhone icon

        Png resizer http://images.my-addr.com/resize_png_online_tool-free_png_resizer_for_web.php

- *-iphone/Images/Default.png:            320x480 iPhone splash screen
- *-iphone/Images/Default@2x.png:         640x960 iPad splash screen

- Android splash screens:

        cp ../spaceball-iphone/Images/Default\@2x.png res/drawable-hdpi/splash.png
        cp ../spaceball-iphone/Images/Default\@2x.png res/drawable-xhdpi/splash.png
        cp ../spaceball-iphone/Images/Default.png res/drawable-ldpi/splash.png
        cp ../spaceball-iphone/Images/Default.png res/drawable-mdpi/splash.png

- CMakeLists.txt:                         Build rules

- resource.h:                             
- new_app_template.nsi:                   Windows Nullsoft Installer config

- pkg/lin.sh:                             Linux package directory prepare
                                       
- pkg/macprep.sh:                         OSX package directory prepare
- pkg/macpkg.sh:                          OSX installer builder
- pkg/mac-Info.plist:                     OSX properties list template

- pkg/iphoneprep.sh:                      iPhone package directory prepare
- pkg/iphonpkg.sh:                        iPhone installer builder
- pkg/mac-Info.plist:                     iPhone properties list template

- new_app_template-iphone/*:              iPhone XCode binary signer config

- new_app_template-android/*:             Android Eclipse linker and installer config
- Android.mk:                             

