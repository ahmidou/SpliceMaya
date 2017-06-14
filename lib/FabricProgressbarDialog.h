//
// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
//

#pragma once

#include "Foundation.h"
#include <QDialog>
#include <QProgressBar>

class FabricProgressbarDialog : public QDialog
{
  Q_OBJECT
  
public:
  FabricProgressbarDialog(QWidget * parent, QString title, int count);
  virtual ~FabricProgressbarDialog();

  void increment();
  void setCount(int count);
  bool wasCancelPressed() const;

private slots:
  void onCancelPressed();

signals:
  void cancelPressed();

private:

  bool m_cancelPressed;
  QProgressBar * m_progressBar;
};
