//
// Copyright (c) 2010-2017 Fabric Software Inc. All rights reserved.
//

#pragma once

#include "FabricSpliceBaseDialog.h"

class FabricSpliceConfirmDialog : public FabricSpliceBaseDialog {
public:

  FabricSpliceConfirmDialog(QWidget * parent);
  virtual FabricCore::Variant getValue(const std::string & name);
};
