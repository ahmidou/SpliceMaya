//
// Copyright (c) 2010-2017 Fabric Software Inc. All rights reserved.
//

#pragma once

#include <QLineEdit>
#include <QCheckBox>
#include "FabricSpliceCheckBoxWidget.h"
#include "FabricSpliceComboWidget.h"
#include "FabricSpliceBaseDialog.h"

class FabricSplicePortDialog : public FabricSpliceBaseDialog {
public:

  FabricSplicePortDialog(QWidget * parent);
  virtual FabricCore::Variant getValue(const std::string & name);

private:
  QLineEdit * mName;
  FabricSpliceComboWidget * mMode;
  FabricSpliceComboWidget * mDataType;
  QLabel * mCustomRTLabel;
  QLineEdit * mCustomRT;
  QLabel * mExtensionLabel;
  QLineEdit * mExtension;
  FabricSpliceComboWidget * mArrayType;
  FabricSpliceCheckBoxWidget * mAttrCheckBox;
  bool mAttrCheckBoxChecked;
  FabricSpliceCheckBoxWidget * mAttrSpliceMayaDataCheckBox;
  FabricSpliceComboWidget * mPersistenceType;

  static void onDataTypeChanged(void * userData);
};
