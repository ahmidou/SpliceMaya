
#ifndef _FABRICDFGWIDGET_H_
#define _FABRICDFGWIDGET_H_

#include <QtGui/QWidget>
#include <QtGui/QSplitter>

#include <map>

#include <maya/MStatus.h>
#include <maya/MString.h>

#include <DFG/DFGUI.h>
#include <DFG/DFGValueEditor.h>
#include <Commands/CommandStack.h>
#include <DFGWrapper/DFGWrapper.h>

#include <FabricSplice.h>

#include "FabricDFGCommandStack.h"

using namespace FabricServices;
using namespace FabricUI;

class FabricDFGWidget : public QSplitter {

  Q_OBJECT
  
public:
  FabricDFGWidget(QWidget * parent);
  ~FabricDFGWidget();

  static QWidget * creator(QWidget * parent, const QString & name);
  
  static void setCurrentUINodeName(const char * node);
  static void log(const char * message);

public slots:
  void onValueChanged();
  void onStructureChanged();
  void onRecompilation();
  void onPortRenamed(QString path, QString newName);
  void hotkeyPressed(Qt::Key, Qt::KeyboardModifier, QString);
  void onNodeDoubleClicked(FabricUI::GraphView::Node * node);

private:

  FabricCore::Client m_client;
  ASTWrapper::KLASTManager * m_manager;
  DFGWrapper::Host * m_host;
  DFG::PresetTreeWidget * m_treeWidget;
  DFG::DFGWidget * m_dfgWidget;
  DFG::DFGValueEditor * m_dfgValueEditor;
  std::string m_baseInterfaceName;
  static std::string s_currentUINodeName;
};

#endif 
