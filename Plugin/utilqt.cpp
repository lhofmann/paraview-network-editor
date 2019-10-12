#include "utilqt.h"

#include <QWidget>
#include <cmath>

int utilqt::emToPx(const QWidget* w, double em) {
  w->ensurePolished();
  return emToPx(w->fontMetrics(), em);
}

int utilqt::emToPx(const QFontMetrics& m, double em) {
  const auto pxPerEm = m.boundingRect(QString(100, 'M')).width() / 100.0;
  return static_cast<int>(std::round(pxPerEm * em));
}
