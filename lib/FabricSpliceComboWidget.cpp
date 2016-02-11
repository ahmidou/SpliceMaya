//
// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
//

#include "FabricSpliceComboWidget.h"

FabricSpliceComboWidget::FabricSpliceComboWidget(QWidget * parent)
: QComboBox(parent)
{
  mCallback = NULL;
  mCallbackUD = NULL;
  mLastText = "";
}

void FabricSpliceComboWidget::clear()
{
  QComboBox::clear();
}

std::string FabricSpliceComboWidget::getStdText()
{
  QString result = currentText();
  #ifdef _WIN32
    return result.toLocal8Bit().constData();
  #else
    return result.toUtf8().constData();
  #endif
}

std::string FabricSpliceComboWidget::itemStdText(unsigned int index)
{
  QString result = itemText(index);
  #ifdef _WIN32
    return result.toLocal8Bit().constData();
  #else
    return result.toUtf8().constData();
  #endif
}

bool FabricSpliceComboWidget::event(QEvent * e)
{
  std::string text(getStdText());
  if(text != mLastText)
  {
    if(mCallback)
      (*mCallback)(mCallbackUD);
    mLastText = text;
  }
  return QComboBox::event(e);
}

void FabricSpliceComboWidget::setCallback(FabricSpliceComboWidget::callback func, void * userData)
{
  mCallback = func;
  mCallbackUD = userData;
}
