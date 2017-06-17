//
// Copyright (c) 2010-2017 Fabric Software Inc. All rights reserved.
//

#include <QCheckBox>
#include <maya/MStatus.h>
#include <maya/MGlobal.h>
#include "FabricDFGWidget.h"
#include <maya/MEventMessage.h>
#include "FabricSpliceHelpers.h"
#include "FabricDFGBaseInterface.h"
#include <DFG/Dialogs/DFGEditPortDialog.h>
#include <FabricUI/Util/LoadFabricStyleSheet.h>
 
FabricDFGWidget *FabricDFGWidget::s_widget = NULL;
FabricServices::ASTWrapper::KLASTManager *s_manager = NULL;

using namespace FabricUI;
using namespace DFG;

void FabricDFGWidget::OnSelectCanvasNodeInDCC(
  void *client) 
{
  if(s_widget == NULL)
    return;

  // When selecting objects in the Outliner, check if there are canvasNodes
  // If so, update the DFGView with the content of the fist node selected.

  MSelectionList sList;
  MGlobal::getActiveSelectionList(sList);

  FabricDFGBaseInterface *baseInterface = s_widget->getBaseInterface();
  MStringArray array;
  sList.getSelectionStrings(array);
  for (unsigned int  i = 0; i < array.length(); ++i) 
  {
    std::string str = array[i].asChar();
    if(baseInterface->getInstanceByName(str))
    {
      MGlobal::executeCommandOnIdle(MString("fabricDFG -a changeNode -node ") + array[i], false /* displayEnabled */);
      break;
    }
  } 
}

FabricDFGWidget::FabricDFGWidget(
  QWidget * parent)
  : FabricDFGWidgetBaseClass(parent)
  , m_initialized( false )
{
  GetCoreClient();
  m_dfgHost = GetCoreClient().getDFGHost();

  QObject::connect(
    this, SIGNAL( portEditDialogCreated(FabricUI::DFG::DFGBaseDialog *)),
    this, SLOT( onPortEditDialogCreated(FabricUI::DFG::DFGBaseDialog *)) );
  QObject::connect(
    this, SIGNAL( portEditDialogInvoked(FabricUI::DFG::DFGBaseDialog *, FTL::JSONObjectEnc<>*)),
    this, SLOT( onPortEditDialogInvoked(FabricUI::DFG::DFGBaseDialog *, FTL::JSONObjectEnc<>*)) );

  // For the moment, don't allow the update the the canvas graph on selection
  // Must be completed in FE-7690
  //m_onSelectionChangedCallbackId = MEventMessage::addEventCallback("SelectionChanged", &FabricDFGWidget::OnSelectCanvasNodeInDCC);
}

FabricDFGWidget::~FabricDFGWidget()
{
  //MMessage::removeCallback(m_onSelectionChangedCallbackId);
  s_widget = NULL;
}

FabricDFGWidget *FabricDFGWidget::Instance(
  bool createIfNull)
{
  if ( !s_widget && createIfNull )
  {
    FabricCore::Client client = GetCoreClient();
    s_manager = new FabricServices::ASTWrapper::KLASTManager( &client );
    s_widget = new FabricDFGWidget( NULL );
  }
  return s_widget;
}

void FabricDFGWidget::Destroy()
{
  delete s_widget;
  s_widget = NULL;
  delete s_manager;
  s_manager = NULL;
}

QWidget * FabricDFGWidget::creator(
  QWidget * parent, 
  const QString & name)
{
  return Instance();
}

void FabricDFGWidget::SetCurrentUINodeName(
  const char * node)
{
  if ( node )
    Instance()->setCurrentUINodeName( node );
}

FabricCore::Client FabricDFGWidget::GetCoreClient()
{
  return FabricSplice::ConstructClient();
}

FabricCore::DFGHost &FabricDFGWidget::getDFGHost()
{
  return m_dfgHost;
}

FabricDFGBaseInterface *FabricDFGWidget::getBaseInterface()
{
  if (s_widget == NULL)
    return NULL;

  if (s_widget->getDfgWidget() == NULL)
    return NULL;
  if (s_widget->getDfgWidget()->getUIController() == NULL)
    return NULL;

  MString interfIdStr = s_widget->getDfgWidget()->getUIController()->getBinding().getMetadata("maya_id");
  if (interfIdStr.length() == 0)
    return NULL;
  return FabricDFGBaseInterface::getInstanceById((uint32_t)interfIdStr.asInt());
}

void FabricDFGWidget::refreshScene() 
{
  MStatus status;
  M3dView view = M3dView::active3dView(&status);
  view.refresh(true, true);
}

void FabricDFGWidget::setCurrentUINodeName(
  const char * node)
{
  std::string currentUINodeName = node;

  FabricDFGBaseInterface *interf =
    FabricDFGBaseInterface::getInstanceByName( currentUINodeName.c_str() );

  FabricCore::DFGBinding binding = interf->getDFGBinding();
  FabricCore::DFGExec exec = binding.getExec();

  if ( !m_initialized )
  {
    DFG::DFGConfig config;
    config.graphConfig.useOpenGL = false;

    FabricCore::Client client = GetCoreClient();
#ifdef FABRIC_SCENEHUB
    init( client,
          s_manager,
          m_dfgHost,
          binding,
          "",
          exec,
          &m_cmdHandler,
          &FabricSpliceRenderCallback::shHostGLRenderer, 
          false, 
          config);
#else
    init( client,
          s_manager, 
          m_dfgHost, 
          binding, 
          "", 
          exec, 
          &m_cmdHandler,
          false, 
          config);
#endif
    m_initialized = true;
  }
  else
    getDfgWidget()->replaceBinding( binding );
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

void FabricDFGWidget::onPortEditDialogCreated(
  DFG::DFGBaseDialog * dialog)
{
  DFG::DFGController * controller = dialog->getDFGController();
  if(!controller->isViewingRootGraph())
    return;

  DFG::DFGEditPortDialog * editPortDialog = (DFG::DFGEditPortDialog *)dialog;
  QString title = editPortDialog->title();

  // FE-6747/FE-7369 : Don't edit the port if it's been set already.
  if(!editPortDialog->dataType().isEmpty())
  {
    //editPortDialog->setDataTypeReadOnly(true);
    editPortDialog->setSoftRangeReadOnly(true);
    editPortDialog->setHardRangeReadOnly(true);
    editPortDialog->setComboReadOnly(true);
    editPortDialog->setVisibilityReadOnly(true);
  }

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

void FabricDFGWidget::onPortEditDialogInvoked(
  DFG::DFGBaseDialog * dialog, 
  FTL::JSONObjectEnc<> * additionalMetaData)
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

void FabricDFGWidget::keyPressEvent(
  QKeyEvent * event)
{
  if(event->modifiers() == Qt::NoModifier)
  {
    Qt::Key key = (Qt::Key)event->key();
    if(key != Qt::Key_Escape)
    {
      event->accept();
      return;
    }
  }
  FabricDFGWidgetBaseClass::keyPressEvent(event);
}
