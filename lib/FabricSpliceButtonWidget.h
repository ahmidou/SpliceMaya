//
// Copyright (c) 2010-2017 Fabric Software Inc. All rights reserved.
//

#pragma once

#include <QWidget>
#include <QPushButton>
#include <QMouseEvent>

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
