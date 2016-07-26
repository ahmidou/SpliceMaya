//
// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
//

#pragma once

#include <QtWidgets/QWidget>
#include <QtWidgets/QListWidget>

class FabricSpliceListWidget : public QListWidget {
public:

  FabricSpliceListWidget(QWidget * parent);
  std::string getStdText();
};
