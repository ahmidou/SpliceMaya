
#ifndef _CREATIONSPLICEBUTTONWIDGET_H_
#define _CREATIONSPLICEBUTTONWIDGET_H_

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

#endif 