//
// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
//

#pragma once

#include <QtWidgets/QWidget>
#include <QtGui/QMouseEvent>
#include <QtGui/QPaintEvent>
#include <QtGui/QPixmap>

class FabricSpliceLogoWidget : public QWidget {
public:

  FabricSpliceLogoWidget(QString pngPath, QWidget * parent);

  virtual void mousePressEvent (QMouseEvent  * e);

  typedef void (*callback) (void * userData);
  void setCallback(callback func, void * userData);

  void paintEvent ( QPaintEvent * event );

private:
  callback mCallback;
  void * mCallbackUD;
  QPixmap mPixmap;
};
