#############################################################################
##    Kwave                - plugins/codec_flac/CMakeLists.txt
##                           -------------------
##    begin                : Mon May 28 2007
##    copyright            : (C) 2007 by Thomas Eschenbacher
##    email                : Thomas.Eschenbacher@gmx.de
#############################################################################
#
#############################################################################
##                                                                          #
##    This program is free software; you can redistribute it and/or modify  #
##    it under the terms of the GNU General Public License as published by  #
##    the Free Software Foundation; either version 2 of the License, or     #
##    (at your option) any later version.                                   #
##                                                                          #
#############################################################################

OPTION(WITH_FLAC "enable support for FLAC files [default=on]" ON)
IF (WITH_FLAC)

    INCLUDE(CheckLibraryExists)

    PKG_CHECK_MODULES(FLAC REQUIRED flac>=1.2.0)
    IF (NOT FLAC_FOUND)
	MESSAGE(FATAL_ERROR "FLAC not found")
    ENDIF(NOT FLAC_FOUND)

    MESSAGE(STATUS "Found FLAC version ${FLAC_VERSION}")

#############################################################################
### check for FLAC header files                                           ###

    CHECK_INCLUDE_FILES(FLAC/format.h HAVE_FLAC_HEADERS)
    IF (HAVE_FLAC_HEADERS)
        MESSAGE(STATUS "Found FLAC include files")
    ELSE (HAVE_FLAC_HEADERS)
        MESSAGE(FATAL_ERROR "libFLAC not found, please install libflac !")
    ENDIF (HAVE_FLAC_HEADERS)

#############################################################################
### check for FLAC++ header files                                         ###

    IF (HAVE_FLAC_HEADERS)
        CHECK_INCLUDE_FILES("FLAC++/decoder.h;FLAC++/metadata.h" HAVE_FLAC_HEADERS)
    ENDIF (HAVE_FLAC_HEADERS)

    IF (HAVE_FLAC_HEADERS)
        MESSAGE(STATUS "Found FLAC++ include files")
    ELSE (HAVE_FLAC_HEADERS)
        MESSAGE(FATAL_ERROR "libFLAC++ not found, please install libflac++ !")
    ENDIF (HAVE_FLAC_HEADERS)

    SET(HAVE_FLAC  ON CACHE BOOL "enable FLAC codec")
    SET(plugin_codec_flac_LIB_SRCS
        FlacCodecPlugin.cpp
        FlacDecoder.cpp
        FlacEncoder.cpp
    )

    SET(plugin_codec_flac_LIBS FLAC++ FLAC)

    KWAVE_PLUGIN(codec_flac)

ENDIF(WITH_FLAC)

#############################################################################
#############################################################################
