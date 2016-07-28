//
// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
//

#include <QPainter>

#include "FabricSpliceLogoWidget.h"

FabricSpliceLogoWidget::FabricSpliceLogoWidget(QString pngPath, QWidget * parent)
: QWidget(parent)
{
  mCallback = NULL;
  mCallbackUD = NULL;

  QPixmap pixmapSrc;
  bool loaded = pixmapSrc.load(pngPath);
  if(loaded)
  {
    mPixmap = pixmapSrc;
    setMinimumWidth(mPixmap.width());
    setMaximumWidth(mPixmap.width());
    setMinimumHeight(mPixmap.height());
    setMaximumHeight(mPixmap.height());
  }
}

void FabricSpliceLogoWidget::mousePressEvent (QMouseEvent  * e)
{
  if(mCallback)
    (*mCallback)(mCallbackUD);
  QWidget::mousePressEvent(e);
}

void FabricSpliceLogoWidget::setCallback(FabricSpliceLogoWidget::callback func, void * userData)
{
  mCallback = func;
  mCallbackUD = userData;
}

void FabricSpliceLogoWidget::paintEvent ( QPaintEvent * event )
{
  QWidget::paintEvent(event);
  if(mPixmap.width() == 0)
    return;
  QPainter painter(this);
  painter.drawPixmap(0, 0, mPixmap);
}
