#!/bin/sh
cd "$(dirname "$0")"
cd ..

cp ./local_addons/ofxNDI/libs/libndi/lib/osx_x64/libndi.3.dylib ./bin/led-animation-toolkit.app/Contents/MacOS/
install_name_tool -change @rpath/libndi.3.dylib @executable_path/libndi.3.dylib ./bin/led-animation-toolkit.app/Contents/MacOS/led-animation-toolkit
