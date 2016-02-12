
#ifndef _CREATIONSPLICEBASEDIALOG_H_
#define _CREATIONSPLICEBASEDIALOG_H_

#include <QtGui/QWidget>
#include <QtGui/QDialog>
#include <QtGui/QLayout>

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

#endif 
