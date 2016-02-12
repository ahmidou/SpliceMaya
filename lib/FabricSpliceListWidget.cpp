//
// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
//

#include "FabricSpliceListWidget.h"

FabricSpliceListWidget::FabricSpliceListWidget(QWidget * parent)
: QListWidget(parent)
{
}

std::string FabricSpliceListWidget::getStdText()
{
  QString result = "";
  QListWidgetItem * item = currentItem();
  if(item != NULL)
    result = item->text();
  #ifdef _WIN32
    return result.toLocal8Bit().constData();
  #else
    return result.toUtf8().constData();
  #endif
}
