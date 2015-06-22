
#ifndef _CREATIONSPLICECONFIRMDIALOG_H_
#define _CREATIONSPLICECONFIRMDIALOG_H_

#include "FabricSpliceBaseDialog.h"

class FabricSpliceConfirmDialog : public FabricSpliceBaseDialog {
public:

  FabricSpliceConfirmDialog(QWidget * parent);
  virtual FabricCore::Variant getValue(const std::string & name);
};

#endif 

