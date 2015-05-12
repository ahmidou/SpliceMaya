
#ifndef _CREATIONSPLICEADDOPDIALOG_H_
#define _CREATIONSPLICEADDOPDIALOG_H_

#include <QtGui/QLineEdit>
#include "FabricSpliceBaseDialog.h"

class FabricSpliceAddOpDialog : public FabricSpliceBaseDialog {
public:

  FabricSpliceAddOpDialog(QWidget * parent);
  virtual FabricCore::Variant getValue(const std::string & name);

private:
  QLineEdit * mName;
};

#endif 
