//
// Copyright (c) 2010-2017 Fabric Software Inc. All rights reserved.
//

#include "FabricSpliceButtonWidget.h"

FabricSpliceButtonWidget::FabricSpliceButtonWidget(QString label, QWidget * parent)
: QPushButton(label, parent)
{
  mCallback = NULL;
  mCallbackUD = NULL;
}

void FabricSpliceButtonWidget::mouseReleaseEvent (QMouseEvent  * e)
{
  QPushButton::mouseReleaseEvent(e);
  if(mCallback)
    (*mCallback)(mCallbackUD);
}

void FabricSpliceButtonWidget::setCallback(FabricSpliceButtonWidget::callback func, void * userData)
{
  mCallback = func;
  mCallbackUD = userData;
}
