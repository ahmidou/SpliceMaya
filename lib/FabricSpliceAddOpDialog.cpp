//
// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
//

#include <QLabel>
#include <QRegExp>
#include <QRegExpValidator>

#include "FabricSpliceAddOpDialog.h"

FabricSpliceAddOpDialog::FabricSpliceAddOpDialog(QWidget * parent)
: FabricSpliceBaseDialog(parent)
{
  setWindowTitle("New Operator");

  QLabel * label = new QLabel("Name", this);
  layout()->addWidget(label);

  mName = new QLineEdit(this);
  layout()->addWidget(mName);

  QRegExp rx("[_a-zA-Z].[_a-zA-Z0-9]*");
  QValidator *validator = new QRegExpValidator(rx, mName);
  mName->setValidator(validator);

  setupButtons();
}

FabricCore::Variant FabricSpliceAddOpDialog::getValue(const std::string & name)
{
  FabricCore::Variant result;
  if(name == "name")
  {
    std::string opName = getStdStringFromQString(mName->text());
    if(opName.length() > 0)
      result = FabricCore::Variant::CreateString(opName.c_str());
  }
  return result;
}
