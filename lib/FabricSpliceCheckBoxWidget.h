//
// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
//

#pragma once

#include <QtWidgets/QWidget>
#include <QtWidgets/QCheckBox>

class FabricSpliceCheckBoxWidget : public QCheckBox {
public:

  FabricSpliceCheckBoxWidget(QString label, QWidget * parent);

  virtual void checkStateSet();

  typedef void (*callback) (void * userData);
  void setCallback(callback func, void * userData);

private:
  callback mCallback;
  void * mCallbackUD;
};
