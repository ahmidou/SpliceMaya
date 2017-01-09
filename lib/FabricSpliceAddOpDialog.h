//
// Copyright (c) 2010-2017 Fabric Software Inc. All rights reserved.
//

#pragma once

#include <QLineEdit>
#include "FabricSpliceBaseDialog.h"

class FabricSpliceAddOpDialog : public FabricSpliceBaseDialog {
public:

  FabricSpliceAddOpDialog(QWidget * parent);
  virtual FabricCore::Variant getValue(const std::string & name);

private:
  QLineEdit * mName;
};
