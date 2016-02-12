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
