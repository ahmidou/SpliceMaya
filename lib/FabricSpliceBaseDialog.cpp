//
// Copyright (c) 2010-2017 Fabric Software Inc. All rights reserved.
//

#include "FabricSpliceBaseDialog.h"
#include "FabricSpliceButtonWidget.h"

FabricSpliceBaseDialog::FabricSpliceBaseDialog(QWidget * parent)
: QDialog(parent)
{
  mCallback = NULL;
  mCallbackUD = NULL;
  mSucceeded = false;

  setLayout(new QVBoxLayout());
  setModal(true);
}

void FabricSpliceBaseDialog::setupButtons()
{
  QWidget * buttons = new QWidget(this);
  layout()->addWidget(buttons);

  buttons->setLayout(new QHBoxLayout());
  buttons->layout()->setContentsMargins(0, 0, 0, 0);

  FabricSpliceButtonWidget * cancelButton = new FabricSpliceButtonWidget("Cancel", buttons);
  buttons->layout()->addWidget(cancelButton);
  FabricSpliceButtonWidget * okButton = new FabricSpliceButtonWidget("OK", buttons);
  buttons->layout()->addWidget(okButton);

  // callbacks
  okButton->setCallback(onOkPressed, this);
  cancelButton->setCallback(onCancelPressed, this);
  okButton->setDefault(true);
}

void FabricSpliceBaseDialog::onOkPressed(void * userData)
{
  FabricSpliceBaseDialog * dialog = (FabricSpliceBaseDialog *)userData;
  dialog->mSucceeded = true;
  if(dialog->mCallback != NULL)
    (*dialog->mCallback)(dialog->mCallbackUD);
  dialog->close();
}

void FabricSpliceBaseDialog::onCancelPressed(void * userData)
{
  FabricSpliceBaseDialog * dialog = (FabricSpliceBaseDialog *)userData;
  dialog->close();
}

void FabricSpliceBaseDialog::setCallback(FabricSpliceBaseDialog::callback func, void * userData)
{
  mCallback = func;
  mCallbackUD = userData;
}

std::string FabricSpliceBaseDialog::getStdStringFromQString(QString input)
{
  #ifdef _WIN32
    return input.toLocal8Bit().constData();
  #else
    return input.toUtf8().constData();
  #endif
}
