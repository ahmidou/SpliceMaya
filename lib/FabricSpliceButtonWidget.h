//
// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
//

#pragma once

#include <QtGui/QWidget>
#include <QtGui/QPushButton>
#include <QtGui/QMouseEvent>

class FabricSpliceButtonWidget : public QPushButton {
public:

  FabricSpliceButtonWidget(QString label, QWidget * parent);

  virtual void mouseReleaseEvent (QMouseEvent  * e);

  typedef void (*callback) (void * userData);
  void setCallback(callback func, void * userData);

private:
  callback mCallback;
  void * mCallbackUD;
};
