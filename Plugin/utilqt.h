#ifndef PARAVIEWNETWORKEDITOR_PLUGIN_UTILQT_H_
#define PARAVIEWNETWORKEDITOR_PLUGIN_UTILQT_H_

#include <QFontMetrics>

namespace utilqt{

constexpr int refEm() { return 11; }

int emToPx(const QWidget* w, double em);
int emToPx(const QFontMetrics& m, double em);

}

#endif //PARAVIEWNETWORKEDITOR_PLUGIN_UTILQT_H_
