
#ifndef _FABRICDFGWIDGET_H_
#define _FABRICDFGWIDGET_H_

#include <QtGui/QWidget>
#include <QtGui/QSplitter>

#include <map>

#include <maya/MStatus.h>
#include <maya/MString.h>

#include <DFG/DFGCombinedWidget.h>

#include <FabricSplice.h>

#include "FabricDFGCommandStack.h"

using namespace FabricServices;
using namespace FabricUI;

class FabricDFGWidget : public DFG::DFGCombinedWidget {

  Q_OBJECT
  
public:
  FabricDFGWidget(QWidget * parent);
  ~FabricDFGWidget();

  static QWidget * creator(QWidget * parent, const QString & name);
  
  static void setCurrentUINodeName(const char * node);
  static void mayaLog(const char * message);

public slots:
  virtual void onRecompilation();
  virtual void onPortRenamed(QString path, QString newName);

private:

  FabricCore::Client m_mayaClient;
  std::string m_baseInterfaceName;
  static std::string s_currentUINodeName;
};

#endif 
