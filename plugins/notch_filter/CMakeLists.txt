#############################################################################
##    Kwave                - plugins/notch_filter/CMakeLists.txt
##                           -------------------
##    begin                : Sat Jun 02 2007
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

SET(plugin_notch_filter_LIB_SRCS
    NotchFilter.cpp
    NotchFilterDialog.cpp
    NotchFilterPlugin.cpp
)

SET(plugin_notch_filter_LIB_UI
    NotchFilterDlg.ui
)

KWAVE_PLUGIN(notch_filter)

#############################################################################
#############################################################################
