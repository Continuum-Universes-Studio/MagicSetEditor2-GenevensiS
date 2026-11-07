// Use the actual Qt application font for custom-drawn interface text.
// wxWidgets 3.3.3 wxQt's wxSYS_DEFAULT_GUI_FONT chooses generic Swiss/sans
// instead of returning QApplication::font(), which breaks KDE font parity.
#pragma once

#ifdef __WXQT__
#include <wx/font.h>
#include <QtGui/QFont>
#include <QtWidgets/QApplication>

inline wxFont genevensisQtGuiFont() {
  return wxFont(QApplication::font());
}
#endif
