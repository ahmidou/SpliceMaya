
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QCheckBox>

#include <DFG/Dialogs/DFGEditPortDialog.h>

#include "FabricDFGWidget.h"
#include "FabricDFGBaseInterface.h"
#include "FabricSpliceHelpers.h"

#include <maya/MGlobal.h>

FabricDFGWidget *FabricDFGWidget::s_widget = NULL;

FabricDFGWidget::FabricDFGWidget(QWidget * parent)
  : DFG::DFGCombinedWidget(parent)
  , m_initialized( false )
{
  m_coreClient = FabricSplice::ConstructClient();
  m_dfgHost = m_coreClient.getDFGHost();

  QObject::connect(
    this, SIGNAL( portEditDialogCreated(FabricUI::DFG::DFGBaseDialog *)),
    this, SLOT( onPortEditDialogCreated(FabricUI::DFG::DFGBaseDialog *)) );
  QObject::connect(
    this, SIGNAL( portEditDialogInvoked(FabricUI::DFG::DFGBaseDialog *)),
    this, SLOT( onPortEditDialogInvoked(FabricUI::DFG::DFGBaseDialog *)) );
}

FabricDFGWidget::~FabricDFGWidget()
{
  s_widget = NULL;
}

FabricDFGWidget *FabricDFGWidget::Instance()
{
  if ( !s_widget )
    s_widget = new FabricDFGWidget( NULL );
  return s_widget;
}

void FabricDFGWidget::Destroy()
{
  if ( s_widget )
    delete s_widget;
  s_widget = NULL;
}

QWidget * FabricDFGWidget::creator(QWidget * parent, const QString & name)
{
  return Instance();
}

void FabricDFGWidget::SetCurrentUINodeName(const char * node)
{
  if ( node )
    Instance()->setCurrentUINodeName( node );
}

void FabricDFGWidget::setCurrentUINodeName(const char * node)
{
  m_currentUINodeName = node;

  FabricDFGBaseInterface *interf =
    FabricDFGBaseInterface::getInstanceByName( m_currentUINodeName.c_str() );

  FabricCore::DFGBinding binding = interf->getDFGBinding();
  FabricCore::DFGExec exec = binding.getExec();

  if ( !m_initialized )
  {
    FabricServices::ASTWrapper::KLASTManager *manager =
      ASTWrapper::KLASTManager::retainGlobalManager( &m_coreClient );

    DFG::DFGConfig config;
    config.graphConfig.useOpenGL = false;

    init( m_coreClient, manager, m_dfgHost, binding, "", exec, &m_cmdHandler,
          false, config );

    m_initialized = true;
  }
  else
    getDfgWidget()->getUIController()->setBindingExec( binding, "", exec );
}

void FabricDFGWidget::onUndo()
{
  MGlobal::executeCommandOnIdle("undo");
}

void FabricDFGWidget::onRedo()
{
  MGlobal::executeCommandOnIdle("redo");
}

void FabricDFGWidget::onPortEditDialogCreated(DFG::DFGBaseDialog * dialog)
{
  DFG::DFGController * controller = dialog->getDFGController();
  if(!controller->isViewingRootGraph())
    return;

  DFG::DFGEditPortDialog * editPortDialog = (DFG::DFGEditPortDialog *)dialog;
  QString title = editPortDialog->title();
  bool addAttribute = true;
  bool native = false;
  bool opaque = false;
  bool enabled = true;
  if(title.length() > 0)
  {
    FabricCore::DFGExec &exec = controller->getExec();
    QString addAttributeSetting = exec.getExecPortMetadata(title.toUtf8().constData(), "addAttribute");
    if(addAttributeSetting == "false")
      addAttribute = false;
    QString nativeSetting = exec.getExecPortMetadata(title.toUtf8().constData(), "nativeArray");
    if(nativeSetting == "true")
      native = true;
    QString opaqueSetting = exec.getExecPortMetadata(title.toUtf8().constData(), "opaque");
    if(opaqueSetting == "true")
      native = true;
    enabled = false;
  }

  QCheckBox * addAttributeCheckBox = new QCheckBox();
  addAttributeCheckBox->setCheckState(addAttribute ? Qt::Checked : Qt::Unchecked);
  addAttributeCheckBox->setEnabled(enabled);
  dialog->addInput(addAttributeCheckBox, "add attribute", "maya specific");

  QCheckBox * nativeCheckBox = new QCheckBox();
  nativeCheckBox->setCheckState(native ? Qt::Checked : Qt::Unchecked);
  nativeCheckBox->setEnabled(enabled);
  dialog->addInput(nativeCheckBox, "native DCC array", "maya specific");

  QCheckBox * opaqueCheckBox = new QCheckBox();
  opaqueCheckBox->setCheckState(opaque ? Qt::Checked : Qt::Unchecked);
  opaqueCheckBox->setEnabled(enabled);
  dialog->addInput(opaqueCheckBox, "opaque in DCC", "maya specific");
}

void FabricDFGWidget::onPortEditDialogInvoked(DFG::DFGBaseDialog * dialog)
{
  DFG::DFGController * controller = dialog->getDFGController();
  if(!controller->isViewingRootGraph())
    return;

  FabricDFGBaseInterface * interf = FabricDFGBaseInterface::getInstanceByName(m_currentUINodeName.c_str());
  if(interf)
  {
    QCheckBox * addAttributeCheckBox = (QCheckBox *)dialog->input("add attribute");
    interf->setAddAttributeForNextAttribute(addAttributeCheckBox->checkState() == Qt::Checked);
    QCheckBox * nativeCheckBox = (QCheckBox *)dialog->input("native DCC array");
    interf->setUseNativeArrayForNextAttribute(nativeCheckBox->checkState() == Qt::Checked);
    QCheckBox * opaqueCheckBox = (QCheckBox *)dialog->input("opaque in DCC");
    interf->setUseOpaqueForNextAttribute(opaqueCheckBox->checkState() == Qt::Checked);
  }
}
