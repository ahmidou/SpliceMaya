
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QCheckBox>

#include <DFG/Dialogs/DFGEditPortDialog.h>

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
    FabricCore::DFGHost host = interf->getDFGHost();
    FabricCore::DFGBinding binding = interf->getDFGBinding();
    FabricCore::DFGExec exec = binding.getExec();

    DFG::DFGConfig config;
    config.graphConfig.useOpenGL = false;
    init(m_mayaClient, manager, host, binding, exec, FabricDFGCommandStack::getStack(), false, config);
  }

    QObject::connect(this, SIGNAL(portEditDialogCreated(FabricUI::DFG::DFGBaseDialog*)), 
      this, SLOT(onPortEditDialogCreated(FabricUI::DFG::DFGBaseDialog*)));
    QObject::connect(this, SIGNAL(portEditDialogInvoked(FabricUI::DFG::DFGBaseDialog*)), 
      this, SLOT(onPortEditDialogInvoked(FabricUI::DFG::DFGBaseDialog*)));
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

void FabricDFGWidget::onPortEditDialogCreated(DFG::DFGBaseDialog * dialog)
{
  DFG::DFGController * controller = dialog->getDFGController();
  if(!controller->isViewingRootGraph())
    return;

  DFG::DFGEditPortDialog * editPortDialog = (DFG::DFGEditPortDialog *)dialog;
  QString title = editPortDialog->title();
  bool native = false;
  bool enabled = true;
  if(title.length() > 0)
  {
    FabricCore::DFGExec exec = controller->getCoreDFGExec();
    QString nativeSetting = exec.getExecPortMetadata(title.toUtf8().constData(), "nativeArray");
    if(nativeSetting == "true")
      native = true;
    enabled = false;
  }

  QCheckBox * nativeCheckBox = new QCheckBox();
  nativeCheckBox->setCheckState(native ? Qt::Checked : Qt::Unchecked);
  nativeCheckBox->setEnabled(enabled);
  dialog->addInput(nativeCheckBox, "native DCC array");
}

void FabricDFGWidget::onPortEditDialogInvoked(DFG::DFGBaseDialog * dialog)
{
  DFG::DFGController * controller = dialog->getDFGController();
  if(!controller->isViewingRootGraph())
    return;

  FabricDFGBaseInterface * interf = FabricDFGBaseInterface::getInstanceByName(m_baseInterfaceName.c_str());
  if(interf)
  {
    QCheckBox * nativeCheckBox = (QCheckBox *)dialog->input("native DCC array");
    interf->setUseNativeArrayForNextAttribute(nativeCheckBox->checkState() == Qt::Checked);
  }
}
