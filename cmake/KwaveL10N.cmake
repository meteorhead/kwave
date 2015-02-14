#############################################################################
##    Kwave                - cmake/KwaveL10N.cmake l10n support
##                           -------------------
##    begin                : Sat Sep 13 2008
##    copyright            : (C) 2008 by Thomas Eschenbacher
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

FIND_PACKAGE(RequiredProgram REQUIRED)

FIND_REQUIRED_PROGRAM(FIND_EXECUTABLE find)
FIND_REQUIRED_PROGRAM(MSGCAT_EXECUTABLE msgcat)
FIND_REQUIRED_PROGRAM(XGETTEXT_EXECUTABLE xgettext)
FIND_REQUIRED_PROGRAM(MSGMERGE_EXECUTABLE msgmerge)
FIND_REQUIRED_PROGRAM(MSGFMT_EXECUTABLE msgfmt)
FIND_REQUIRED_PROGRAM(RM_EXECUTABLE rm)

SET(EXTRACT_MESSAGES ${CMAKE_SOURCE_DIR}/l10n-kde4/scripts/extract-messages.sh)

SET(PO_DIR "${CMAKE_SOURCE_DIR}/po")
SET(POT_FILE "${PO_DIR}/kwave.pot")

#############################################################################
### get environment variable LINGUAS, default to all languages            ###

SET(LINGUAS "$ENV{LINGUAS}")
STRING(REGEX REPLACE "[ \t]+" \; OUTPUT "${LINGUAS}")
SEPARATE_ARGUMENTS(LINGUAS)
IF ("${LINGUAS}" STREQUAL "")
    SET(LINGUAS "*")
ENDIF ("${LINGUAS}" STREQUAL "")

#############################################################################
### find out which po files exist                                        ###

FILE(GLOB _existing_po_files "${PO_DIR}/*.po")
FOREACH(_po_file ${_existing_po_files})
    GET_FILENAME_COMPONENT(_lang "${_po_file}" NAME_WE)

    # take only languages that have been requested
    SET(_take_it TRUE)
    IF (NOT "${LINGUAS}" STREQUAL "*")
	LIST(FIND LINGUAS "${_lang}" _found)
	IF (_found LESS 0)
	    SET(_take_it FALSE)
	ENDIF (_found LESS 0)
    ENDIF (NOT "${LINGUAS}" STREQUAL "*")

    IF (_take_it)
	SET(KWAVE_BUILD_LINGUAS_STRING "${KWAVE_BUILD_LINGUAS_STRING} ${_lang}")
	LIST(APPEND KWAVE_BUILD_LINGUAS "${_lang}")
	LIST(APPEND _po_files "${_po_file}")
	MESSAGE(STATUS "Enabled GUI translation for ${_lang}")

	# handle generation and installation of the message catalog (gmo)
	SET(_gmo_file ${PO_DIR}/${_lang}.gmo)

	ADD_CUSTOM_COMMAND(
	    OUTPUT ${_gmo_file}
	    COMMAND ${CMAKE_COMMAND} -E make_directory ${PO_DIR}
	    COMMAND ${MSGMERGE_EXECUTABLE} --quiet --update --backup=none -s
	                ${_po_file} ${POT_FILE}
	    COMMAND ${MSGFMT_EXECUTABLE} -o ${_gmo_file} ${_po_file}
	    DEPENDS ${POT_FILE} ${_po_file}
	)

	INSTALL(
	    FILES ${_gmo_file}
	    DESTINATION ${KDE4_LOCALE_INSTALL_DIR}/${_lang}/LC_MESSAGES
	    RENAME kwave.mo
	)
	SET(_gmo_files ${_gmo_files} ${_gmo_file})

    ENDIF (_take_it)
ENDFOREACH(_po_file ${_existing_po_files})

#############################################################################
### update the local copy of the translations with files from kde svn     ###

ADD_CUSTOM_TARGET(update-translations
    COMMAND ${CMAKE_SOURCE_DIR}/bin/svn-update-l10n.sh "${CMAKE_SOURCE_DIR}"
)

#############################################################################
### processing of GUI translations if found                               ###

IF (NOT "${KWAVE_BUILD_LINGUAS}" STREQUAL "")

    FIND_PACKAGE(Gettext REQUIRED)

    IF ("${LINGUAS}" STREQUAL "*")
	MESSAGE(STATUS "LINGUAS not set, building for all supported languages")
    ENDIF ("${LINGUAS}" STREQUAL "*")

    # generate po/kwave.pot
    ADD_CUSTOM_COMMAND(OUTPUT ${POT_FILE}
	COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/po
	COMMAND ${RM_EXECUTABLE} -f ${CMAKE_BINARY_DIR}/po/*.gmo
	COMMAND cd ${CMAKE_SOURCE_DIR} && ${EXTRACT_MESSAGES}
	DEPENDS ${EXTRACT_MESSAGES}
	COMMAND ${RM_EXECUTABLE} -f ${CMAKE_SOURCE_DIR}/messages.mo
    )

    # build target "package-messages"
    ADD_CUSTOM_TARGET(package-messages ALL
	DEPENDS ${_gmo_files}
    )

    SET_DIRECTORY_PROPERTIES(PROPERTIES
	ADDITIONAL_MAKE_CLEAN_FILES
	"${CMAKE_BINARY_DIR}/po"
    )

ELSE (NOT "${KWAVE_BUILD_LINGUAS}" STREQUAL "")

    MESSAGE(STATUS "Found no suitable language to build for")

ENDIF (NOT "${KWAVE_BUILD_LINGUAS}" STREQUAL "")

#############################################################################
#############################################################################
