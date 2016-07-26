//
// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
//

#pragma once

#include <QtWidgets/QWidget>
#include <QtWidgets/QDialog>
#include <QtWidgets/QLayout>

#include <FabricCore.h>

class FabricSpliceBaseDialog : public QDialog {
public:

  FabricSpliceBaseDialog(QWidget * parent);
  void setupButtons();
  virtual FabricCore::Variant getValue(const std::string & name) = 0;
  bool succeeded() { return mSucceeded; }

  typedef void (*callback) (void * userData);
  void setCallback(callback func, void * userData);

private:
  static void onOkPressed(void * userData);
  static void onCancelPressed(void * userData);
  callback mCallback;
  void * mCallbackUD;
  bool mSucceeded;
protected:
  std::string getStdStringFromQString(QString input);  
};
