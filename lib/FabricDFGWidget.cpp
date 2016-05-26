//
// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
//

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
FabricCore::Client FabricDFGWidget::s_coreClient;

FabricDFGWidget::FabricDFGWidget(QWidget * parent)
#ifdef FABRIC_SCENEHUB
  : DFG::SHDFGCombinedWidget(parent)
#else
  : DFG::DFGCombinedWidget(parent)
#endif
  , m_initialized( false )
{
  GetCoreClient();
  m_dfgHost = s_coreClient.getDFGHost();

  QObject::connect(
    this, SIGNAL( portEditDialogCreated(FabricUI::DFG::DFGBaseDialog *)),
    this, SLOT( onPortEditDialogCreated(FabricUI::DFG::DFGBaseDialog *)) );
  QObject::connect(
    this, SIGNAL( portEditDialogInvoked(FabricUI::DFG::DFGBaseDialog *, FTL::JSONObjectEnc<>*)),
    this, SLOT( onPortEditDialogInvoked(FabricUI::DFG::DFGBaseDialog *, FTL::JSONObjectEnc<>*)) );
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
  s_coreClient = FabricCore::Client();
}

QWidget * FabricDFGWidget::creator(QWidget * parent, const QString & name)
{
  return Instance();
}

void FabricDFGWidget::initMenu() {

  m_menuBar = new QMenuBar(this);
  m_menuBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  addWidget(m_menuBar);

  // populate the menu bar
  QObject::connect(
    m_dfgWidget,
    SIGNAL(additionalMenuActionsRequested(QString, QMenu*, bool)),
    this, SLOT(onAdditionalMenuActionsRequested(QString, QMenu *, bool))
    );

  m_dfgWidget->populateMenuBar(m_menuBar, false /* addFileMenu */, true /* addDCCMenu */);
}

void FabricDFGWidget::SetCurrentUINodeName(const char * node)
{
  if ( node )
    Instance()->setCurrentUINodeName( node );
}

void FabricDFGWidget::refreshScene() {
  MStatus status;
  M3dView view = M3dView::active3dView(&status);
  view.refresh(true, true);
}

void FabricDFGWidget::setCurrentUINodeName(const char * node)
{
  std::string currentUINodeName = node;

  FabricDFGBaseInterface *interf =
    FabricDFGBaseInterface::getInstanceByName( currentUINodeName.c_str() );

  FabricCore::DFGBinding binding = interf->getDFGBinding();
  FabricCore::DFGExec exec = binding.getExec();

  if ( !m_initialized )
  {
    FabricServices::ASTWrapper::KLASTManager *manager =
      ASTWrapper::KLASTManager::retainGlobalManager( &s_coreClient );

    DFG::DFGConfig config;
    config.graphConfig.useOpenGL = false;

    init( s_coreClient, manager, m_dfgHost, binding, "", exec, &m_cmdHandler,
          false, config );

    // adjust some background colors that cannot be defined
    // using DFG::DFGConfig (note: this also kinda solves [FE-6009]).
    getTreeWidget()    ->setStyleSheet("background-color: rgb(48,48,48); border: 1px solid black;");
    getDfgValueEditor()->setStyleSheet("background-color: rgb(48,48,48);");
    getDfgLogWidget()  ->setStyleSheet("background-color: rgb(48,48,48); border: 1px solid black;");

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

void FabricDFGWidget::onSelectCanvasNodeInDCC()
{
  MString interfIdStr = getDfgWidget()->getUIController()->getBinding().getMetadata("maya_id");
  if (interfIdStr.length() == 0)
    return;
  FabricDFGBaseInterface *interf = FabricDFGBaseInterface::getInstanceById((uint32_t)interfIdStr.asInt());
  if (interf)
  {
    MFnDependencyNode mayaNode(interf->getThisMObject());
    MGlobal::executeCommandOnIdle(MString("select ") + mayaNode.name(), true /* displayEnabled */);
  }
}

void FabricDFGWidget::onImportGraphInDCC()
{
  MString interfIdStr = getDfgWidget()->getUIController()->getBinding().getMetadata("maya_id");
  if (interfIdStr.length() == 0)
    return;
  FabricDFGBaseInterface *interf = FabricDFGBaseInterface::getInstanceById((uint32_t)interfIdStr.asInt());
  if (interf)
  {
    MFnDependencyNode mayaNode(interf->getThisMObject());
    MGlobal::executeCommand(MString("dfgImportJSON -m ") + mayaNode.name() + MString(" -f \"\""), true /* displayEnabled */, false /* undoEnabled */);

    // refresh Canvas.
    MGlobal::executeCommandOnIdle(MString("fabricDFG -a showUI -node ") + mayaNode.name(), false /* displayEnabled */);
  }
}

void FabricDFGWidget::onExportGraphInDCC()
{
  MString interfIdStr = getDfgWidget()->getUIController()->getBinding().getMetadata("maya_id");
  if (interfIdStr.length() == 0)
    return;
  FabricDFGBaseInterface *interf = FabricDFGBaseInterface::getInstanceById((uint32_t)interfIdStr.asInt());
  if (interf)
  {
    MFnDependencyNode mayaNode(interf->getThisMObject());
    MGlobal::executeCommand(MString("dfgExportJSON -m ") + mayaNode.name() + MString(" -f \"\""), true /* displayEnabled */, false /* undoEnabled */);
  }
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
      opaque = true;
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

void FabricDFGWidget::onPortEditDialogInvoked(DFG::DFGBaseDialog * dialog, FTL::JSONObjectEnc<> * additionalMetaData)
{
  DFG::DFGController * controller = dialog->getDFGController();
  if(!controller->isViewingRootGraph())
    return;

  if(additionalMetaData)
  {
    QCheckBox * addAttributeCheckBox = (QCheckBox *)dialog->input("add attribute");
    if(addAttributeCheckBox->checkState() == Qt::Unchecked)
    {
      FTL::JSONEnc<> enc( *additionalMetaData, FTL_STR("addAttribute") );
      FTL::JSONStringEnc<> valueEnc( enc, FTL_STR("false") );
    }

    QCheckBox * nativeCheckBox = (QCheckBox *)dialog->input("native DCC array");
    if(nativeCheckBox->checkState() == Qt::Checked)
    {
      FTL::JSONEnc<> enc( *additionalMetaData, FTL_STR("nativeArray") );
      FTL::JSONStringEnc<> valueEnc( enc, FTL_STR("true") );
    }

    QCheckBox * opaqueCheckBox = (QCheckBox *)dialog->input("opaque in DCC");
    if(opaqueCheckBox->checkState() == Qt::Checked)
    {
      FTL::JSONEnc<> enc( *additionalMetaData, FTL_STR("opaque") );
      FTL::JSONStringEnc<> valueEnc( enc, FTL_STR("true") );
    }
  }
}
