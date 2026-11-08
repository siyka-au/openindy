#ifndef UIUTIL_H
#define UIUTIL_H

#include <QFont>
#include <QFontMetrics>
#include <QMap>
#include <QString>
#include <QStringList>

#include "types.h"

namespace oi {

const int Q_DECL_EXPORT getDropDownMenuSize(QStringList list,
                                            const int menuSize);

}

#endif  // UIUTIL_H
