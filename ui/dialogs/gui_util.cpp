#include "gui_util.h"

namespace {
constexpr double kPopupMargin =
    0.1;  // extra popup width relative to the widest entry
}  // namespace

namespace oi {

/*!
 * \brief getDropDownMenuSize
 * \param list
 * \param menuSize
 * \return
 */
const int getDropDownMenuSize(QStringList list, const int menuSize) {
  // get largest string in list
  QString largestString = "";
  foreach (const QString& filter, list) {
    if (filter.length() > largestString.length()) {
      largestString = filter;
    }
  }

  // calculate width of popup dependend of list
  QFont font;
  QFontMetrics fm(font);
  int width = fm.horizontalAdvance(largestString);

  // get largest width
  if ((width + (kPopupMargin * width)) > menuSize) {
    return static_cast<int>(width + (kPopupMargin * width));
  } else {
    return menuSize;
  }
  return menuSize;
}

}  // namespace oi
