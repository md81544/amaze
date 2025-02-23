#!/bin/bash

# tmplibs contains sfml libs I've recompiled to work around the fullscreen bug in my
# latest bug report to SFML. This just fixes the relative paths hardcoded into the dylibs
# so I could move them into the Amaze source tree and not have things break

DIR=`brew --prefix`

install_name_tool -change  "@rpath/../Frameworks/freetype.framework/Versions/A/freetype" "${DIR}/opt/freetype/lib/libfreetype.6.dylib" ./libsfml-graphics.2.6.2.dylib

install_name_tool -change  "@rpath/../Frameworks/OpenAL.framework/Versions/A/OpenAL" "/System/Library/Frameworks/OpenAL.framework/Versions/A/OpenAL" ./libsfml-audio.2.6.2.dylib
install_name_tool -change  "@rpath/../Frameworks/vorbisenc.framework/Versions/A/vorbisenc" "${DIR}/opt/libvorbis/lib/libvorbisenc.2.dylib" ./libsfml-audio.2.6.2.dylib
install_name_tool -change  "@rpath/../Frameworks/vorbisfile.framework/Versions/A/vorbisfile" "${DIR}/opt/libvorbis/lib/libvorbisfile.3.dylib" ./libsfml-audio.2.6.2.dylib
install_name_tool -change  "@rpath/../Frameworks/vorbis.framework/Versions/A/vorbis" "${DIR}/opt/libvorbis/lib/libvorbis.0.dylib" ./libsfml-audio.2.6.2.dylib
install_name_tool -change  "@rpath/../Frameworks/ogg.framework/Versions/A/ogg" "${DIR}/opt/libogg/lib/libogg.0.dylib" ./libsfml-audio.2.6.2.dylib
install_name_tool -change  "@rpath/../Frameworks/FLAC.framework/Versions/A/FLAC" "${DIR}/opt/flac/lib/libFLAC.12.dylib" ./libsfml-audio.2.6.2.dylib
# Just in case the above doesn't work, owing to having been changed before, we update to
# flac 14 from flac 12:
install_name_tool -change  "${DIR}/opt/flac/lib/libFLAC.12.dylib" "${DIR}/opt/flac/lib/libFLAC.14.dylib" ./libsfml-audio.2.6.2.dylib
