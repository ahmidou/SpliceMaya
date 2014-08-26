
#ifndef _CREATIONSPLICELISTWIDGET_H_
#define _CREATIONSPLICELISTWIDGET_H_

#include <QtGui/QWidget>
#include <QtGui/QListWidget>

class FabricSpliceListWidget : public QListWidget {
public:

  FabricSpliceListWidget(QWidget * parent);
  std::string getStdText();
};

#endif 