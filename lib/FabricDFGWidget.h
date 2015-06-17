
#ifndef _FABRICDFGWIDGET_H_
#define _FABRICDFGWIDGET_H_

#include <QtGui/QWidget>
#include <QtGui/QSplitter>

#include <map>

#include <maya/MStatus.h>
#include <maya/MString.h>

#include <DFG/DFGCombinedWidget.h>
#include <DFG/Dialogs/DFGBaseDialog.h>

#include <FabricSplice.h>

#include "FabricDFGCommandStack.h"

using namespace FabricServices;
using namespace FabricUI;

class FabricDFGBaseInterface;

class FabricDFGWidget : public DFG::DFGCombinedWidget {

  Q_OBJECT
  
public:
  FabricDFGWidget(QWidget * parent);
  ~FabricDFGWidget();

  static QWidget * creator(QWidget * parent, const QString & name);
  
  static void setCurrentUINodeName(const char * node);
  static void mayaLog(const char * message);
  static void closeWidgetsForBaseInterface(FabricDFGBaseInterface * interf);

public slots:
  virtual void onRecompilation();

private slots:
  void onPortEditDialogCreated(FabricUI::DFG::DFGBaseDialog * dialog);
  void onPortEditDialogInvoked(FabricUI::DFG::DFGBaseDialog * dialog);

private:

  FabricCore::Client m_mayaClient;
  std::string m_baseInterfaceName;
  static std::string s_currentUINodeName;
  static std::map<FabricDFGWidget*, FabricDFGBaseInterface*> s_widgets;
};

#endif 
