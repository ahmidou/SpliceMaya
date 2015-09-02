
#ifndef _FABRICDFGWIDGET_H_
#define _FABRICDFGWIDGET_H_

#include "Foundation.h"
#include "DFGUICmdHandler_Maya.h"
#include <QtGui/QWidget>
#include <QtGui/QSplitter>

#include <map>

#include <maya/MStatus.h>
#include <maya/MString.h>

#include <DFG/DFGCombinedWidget.h>
#include <DFG/Dialogs/DFGBaseDialog.h>

#include <FabricSplice.h>

using namespace FabricServices;
using namespace FabricUI;

class FabricDFGBaseInterface;

class FabricDFGWidget : public DFG::DFGCombinedWidget {

  Q_OBJECT
  
public:
  FabricDFGWidget(QWidget * parent);
  ~FabricDFGWidget();

  static QWidget * creator(QWidget * parent, const QString & name);
 
  static FabricDFGWidget *Instance();
  static void Destroy();

  static void SetCurrentUINodeName(const char * node);

  FabricCore::Client &getCoreClient()
  {
    return m_coreClient;
  }

  FabricCore::DFGHost &getDFGHost()
  {
    return m_dfgHost;
  }

public slots:
  virtual void onUndo();
  virtual void onRedo();
  virtual void onRecompilation();

private slots:
  void onPortEditDialogCreated(FabricUI::DFG::DFGBaseDialog * dialog);
  void onPortEditDialogInvoked(FabricUI::DFG::DFGBaseDialog * dialog);

protected:
  void setCurrentUINodeName(const char * node);

private:

  DFGUICmdHandler_Maya m_cmdHandler;
  std::string m_currentUINodeName;
  FabricCore::Client m_coreClient;
  FabricCore::DFGHost m_dfgHost;
  bool m_initialized;

  static FabricDFGWidget *s_widget;
};

#endif 
