
#ifndef _CREATIONSPLICELICENSEDIALOG_H_
#define _CREATIONSPLICELICENSEDIALOG_H_

#include <QtGui/QWidget>
#include <QtGui/QDialog>
#include <QtGui/QLayout>
#include <QtGui/QTextEdit>
#include <QtGui/QLineEdit>

#include <FabricCore.h>

class FabricSpliceLicenseDialog : public QDialog {
public:

  FabricSpliceLicenseDialog(QWidget * parent);
  ~FabricSpliceLicenseDialog();

private:
  static void onOkPressed(void * userData);
  static void onCancelPressed(void * userData);
  static std::string getStdStringFromQString(QString input); 

  QLineEdit * mLicenseServer;
};

#endif 
