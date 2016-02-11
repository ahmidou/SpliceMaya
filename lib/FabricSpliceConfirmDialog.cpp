//
// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
//

#include "FabricSpliceConfirmDialog.h"

FabricSpliceConfirmDialog::FabricSpliceConfirmDialog(QWidget * parent)
: FabricSpliceBaseDialog(parent)
{
  setWindowTitle("Are you sure?");
  setupButtons();
}

FabricCore::Variant FabricSpliceConfirmDialog::getValue(const std::string & name)
{
  return FabricCore::Variant();
}
