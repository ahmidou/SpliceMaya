
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QLabel>

#include "FabricDFGWidget.h"
#include "FabricDFGBaseInterface.h"
#include "FabricSpliceHelpers.h"

#include <maya/MGlobal.h>

std::string FabricDFGWidget::s_currentUINodeName;
std::map<FabricDFGWidget*, FabricDFGBaseInterface*> FabricDFGWidget::s_widgets;

FabricDFGWidget::FabricDFGWidget(QWidget * parent)
:DFG::DFGCombinedWidget(parent)
{
  m_baseInterfaceName = s_currentUINodeName;
  
  FabricDFGBaseInterface * interf = FabricDFGBaseInterface::getInstanceByName(m_baseInterfaceName.c_str());

  if(interf)
  {
    s_widgets.insert(std::pair<FabricDFGWidget*, FabricDFGBaseInterface*>(this, interf));

    m_mayaClient = interf->getCoreClient();
    FabricServices::ASTWrapper::KLASTManager * manager = interf->getASTManager();
    FabricServices::DFGWrapper::Host * host = interf->getDFGHost();

    DFGWrapper::Binding binding = interf->getDFGBinding();
    DFGWrapper::GraphExecutablePtr graph = DFGWrapper::GraphExecutablePtr::StaticCast(binding.getExecutable());

    DFG::DFGConfig config;
    config.graphConfig.useOpenGL = false;
    init(&m_mayaClient, manager, host, binding, graph, FabricDFGCommandStack::getStack(), false, config);
  }
}

FabricDFGWidget::~FabricDFGWidget()
{
}

QWidget * FabricDFGWidget::creator(QWidget * parent, const QString & name)
{
  return new FabricDFGWidget(parent);
}

void FabricDFGWidget::setCurrentUINodeName(const char * node)
{
  if(node)
    s_currentUINodeName = node;
}

void FabricDFGWidget::closeWidgetsForBaseInterface(FabricDFGBaseInterface * interf)
{
  std::map<FabricDFGWidget*, FabricDFGBaseInterface*>::iterator it;
  for(it = s_widgets.begin(); it != s_widgets.end(); it++)
  {
    if(it->second == interf)
    {
      s_widgets.erase(it);

      QWidget * parent = (QWidget*)it->first->parent();

      // layout widget
      if(parent)
        parent = (QWidget*)parent->parent();

      // dock widget
      if(parent)
        parent = (QWidget*)parent->parent();

      if(parent)
      {
        parent->close();
        parent->deleteLater();
      }
      break;
    }
  }
}

void FabricDFGWidget::onRecompilation()
{
  FabricDFGBaseInterface * interf = FabricDFGBaseInterface::getInstanceByName(m_baseInterfaceName.c_str());
  if(interf)
  {
    interf->invalidateNode();
  }
}

void FabricDFGWidget::mayaLog(const char * message)
{
  mayaLogFunc(message);
}