
// [andrew 20130620] all hell breaks loose in Linux if these aren't included first
#include <QtCore/QDataStream>
#include <QtCore/QMetaType>
#include <QtGui/QTextEdit>
#include <QtGui/QComboBox>
#include <QtCore/QEvent>

#include "plugin.h"
#include <maya/MGlobal.h>
#include <maya/MFnPlugin.h>
#include <maya/MSceneMessage.h>
#include <maya/MDGMessage.h>
#include <maya/MAnimMessage.h>
#include <maya/MUiMessage.h>
#include <maya/MEventMessage.h>
#include <maya/MQtUtil.h>
#include <maya/MCommandResult.h>
#include <maya/MViewport2Renderer.h>
#include <maya/MFileIO.h>

#include <FabricSplice.h>
#include "FabricSpliceMayaNode.h"
#include "FabricSpliceMayaDeformer.h"
#include "FabricSpliceCommand.h"
#include "FabricSpliceEditorCmd.h"
#include "FabricSpliceMayaData.h"
#include "FabricSpliceToolContext.h"
#include "FabricSpliceRenderCallback.h"
#include "FabricDFGWidgetCommand.h"
#include "FabricDFGWidget.h"
#include "FabricDFGCanvasNode.h"
#include "FabricDFGCanvasDeformer.h"
#include "FabricDFGCommands.h"
#include "FabricSpliceHelpers.h"
#include "FabricUpgradeAttrCommand.h"

#ifdef _MSC_VER
  #define MAYA_EXPORT extern "C" __declspec(dllexport) MStatus _cdecl
#else
  #define MAYA_EXPORT extern "C" MStatus
#endif

// FE Owned IDs 0x0011AE40 - 0x0011AF3F
const MTypeId gFirstValidNodeID(0x0011AE40);
// FabricSpliceMayaNode       0x0011AE41
// FabricSpliceMayaDeformer   0x0011AE42
// FabricSpliceInlineGeometry 0x0011AE43  /* no longer in use, but not available */
// FabricSpliceMayaDebugger   0x0011AE44  /* no longer in use, but not available */
// FabricSpliceMayaData       0x0011AE45
// FabricDFGMayaNode          0x0011AE46  /* no longer in use, but not available */
// FabricDFGMayaNode          0x0011AE47  // canvasNode
// FabricDFGMayaDeformer      0x0011AE48  // canvasDeformer
const MTypeId gLastValidNodeID(0x0011AF3F);

MCallbackId gOnSceneNewCallbackId;
MCallbackId gOnSceneLoadCallbackId;
MCallbackId gOnSceneSaveCallbackId;
MCallbackId gOnMayaExitCallbackId;
MCallbackId gOnSceneImportCallbackId;
MCallbackId gOnSceneExportCallbackId;
MCallbackId gOnSceneCreateReferenceCallbackId;
MCallbackId gOnSceneImportReferenceCallbackId;
MCallbackId gOnSceneLoadReferenceCallbackId;
MCallbackId gOnNodeAddedCallbackId;
MCallbackId gOnNodeRemovedCallbackId;
MCallbackId gOnNodeAddedDFGCallbackId;
MCallbackId gOnNodeRemovedDFGCallbackId;
MCallbackId gOnAnimCurveEditedCallbackId;
MCallbackId gOnBeforeSceneOpenCallbackId;
MCallbackId gOnModelPanelSetFocusCallbackId;

void onSceneSave(void *userData){

  MStatus status = MS::kSuccess;
  mayaSetLastLoadedScene(MFileIO::beforeSaveFilename(&status));
  if(mayaGetLastLoadedScene().length() == 0) // this happens during copy & paste
    return;

  std::vector<FabricSpliceBaseInterface*> instances = FabricSpliceBaseInterface::getInstances();

  for(uint32_t i = 0; i < instances.size(); ++i){
    FabricSpliceBaseInterface *node = instances[i];
    node->storePersistenceData(mayaGetLastLoadedScene(), &status);
  }

  FabricDFGBaseInterface::allStorePersistenceData(mayaGetLastLoadedScene(), &status);
}

void onSceneNew(void *userData){
  FabricSpliceEditorWidget::postClearAll();
  FabricSpliceRenderCallback::sDrawContext.invalidate(); 

  MString cmd = "source \"FabricDFGUI.mel\"; deleteDFGWidget();";
  MGlobal::executeCommandOnIdle(cmd, false);
  FabricDFGWidget::Destroy();
 
  char const *no_client_persistence = ::getenv( "FABRIC_DISABLE_CLIENT_PERSISTENCE" );
  if (!!no_client_persistence && !!no_client_persistence[0])
  {
    // [FE-5944] old behavior: destroy the client.
    FabricSplice::DestroyClient();
  }
  else
  {
    // [FE-5508]
    // rather than destroying the client via
    // FabricSplice::DestroyClient() we only
    // remove all singleton objects.
    const FabricCore::Client * client = NULL;
    FECS_DGGraph_getClient(&client);
    if (client)
    {
      FabricCore::RTVal handleVal = FabricSplice::constructObjectRTVal("SingletonHandle");
      handleVal.callMethod("", "removeAllObjects", 0, NULL);
    }
  }
}

void onSceneLoad(void *userData){
  FabricSpliceEditorWidget::postClearAll();
  FabricSpliceRenderCallback::sDrawContext.invalidate(); 

  if(getenv("FABRIC_SPLICE_PROFILING") != NULL)
    FabricSplice::Logging::enableTimers();

  MStatus status = MS::kSuccess;
  mayaSetLastLoadedScene(MFileIO::currentFile());

  std::vector<FabricSpliceBaseInterface*> instances = FabricSpliceBaseInterface::getInstances();

  // each node will only restore once, so it's safe for import too
  FabricSplice::Logging::AutoTimer persistenceTimer("Maya::onSceneLoad");
  for(uint32_t i = 0; i < instances.size(); ++i){
    FabricSpliceBaseInterface *node = instances[i];
    node->restoreFromPersistenceData(mayaGetLastLoadedScene(), &status); 
    if( status != MS::kSuccess)
      return;
  }
  FabricSpliceEditorWidget::postClearAll();

  FabricDFGBaseInterface::allRestoreFromPersistenceData(mayaGetLastLoadedScene(), &status);

  if(getenv("FABRIC_SPLICE_PROFILING") != NULL)
  {
    for(unsigned int i=0;i<FabricSplice::Logging::getNbTimers();i++)
    {
      std::string name = FabricSplice::Logging::getTimerName(i);
      FabricSplice::Logging::logTimer(name.c_str());
      FabricSplice::Logging::resetTimer(name.c_str());
    }
  }

  // [FE-6612] invalidate all DFG nodes.
  for (unsigned int i=0;i<FabricDFGBaseInterface::getNumInstances();i++)
    FabricDFGBaseInterface::getInstanceByIndex(i)->invalidateNode();
}

bool gSceneIsDestroying = false;
void onMayaExiting(void *userData){
  gSceneIsDestroying = true;
  std::vector<FabricSpliceBaseInterface*> instances = FabricSpliceBaseInterface::getInstances();

  for(uint32_t i = 0; i < instances.size(); ++i){
    FabricSpliceBaseInterface *node = instances[i];
    node->resetInternalData();
  }

  FabricDFGBaseInterface::allResetInternalData();

  FabricDFGWidget::Destroy();

  FabricSplice::DestroyClient(true);
}

bool isDestroyingScene()
{
  return gSceneIsDestroying;
}

#define MAYA_REGISTER_DFGUICMD( plugin, Name ) \
  plugin.registerCommand( \
    MayaDFGUICmd_##Name::GetName(), \
    MayaDFGUICmd_##Name::GetCreator(), \
    MayaDFGUICmd_##Name::GetCreateSyntax() \
    )

#define MAYA_DEREGISTER_DFGUICMD( plugin, Name ) \
  plugin.deregisterCommand( \
    MayaDFGUICmd_##Name::GetName() \
    )

#if defined(OSMac_)
__attribute__ ((visibility("default")))
#endif
MAYA_EXPORT initializePlugin(MObject obj)
{
  // [FE-6287]
  char const *disable_evalContext = ::getenv( "FABRIC_MAYA_DISABLE_EVALCONTEXT" );
  FabricDFGBaseInterface::s_use_evalContext = !(!!disable_evalContext && !!disable_evalContext[0]);
  if (!FabricDFGBaseInterface::s_use_evalContext)
    MGlobal::displayInfo("[Fabric for Maya]: evalContext has been disabled via the environment variable FABRIC_MAYA_DISABLE_EVALCONTEXT.");

  MFnPlugin plugin(obj, "FabricMaya", FabricSplice::GetFabricVersionStr(), "Any");
  MStatus status;

  status = plugin.registerContextCommand("FabricSpliceToolContext", FabricSpliceToolContextCmd::creator, "FabricSpliceToolCommand", FabricSpliceToolCmd::creator  );

  loadMenu();

  // FE-6558 : Don't plug the render-callback if not interactive.
  // Otherwise it will crash on linux machine without DISPLAY
  if (MGlobal::mayaState() == MGlobal::kInteractive)
    FabricSpliceRenderCallback::plug();

  gOnSceneSaveCallbackId = MSceneMessage::addCallback(MSceneMessage::kBeforeSave, onSceneSave);
  gOnSceneLoadCallbackId = MSceneMessage::addCallback(MSceneMessage::kAfterOpen, onSceneLoad);
  gOnBeforeSceneOpenCallbackId = MSceneMessage::addCallback(MSceneMessage::kBeforeOpen, onSceneNew);
  gOnSceneNewCallbackId = MSceneMessage::addCallback(MSceneMessage::kBeforeNew, onSceneNew);
  gOnMayaExitCallbackId = MSceneMessage::addCallback(MSceneMessage::kMayaExiting, onMayaExiting);
  gOnSceneExportCallbackId = MSceneMessage::addCallback(MSceneMessage::kBeforeExport, onSceneSave);
  gOnSceneImportCallbackId = MSceneMessage::addCallback(MSceneMessage::kAfterImport, onSceneLoad);
  gOnSceneCreateReferenceCallbackId = MSceneMessage::addCallback(MSceneMessage::kAfterCreateReference, onSceneLoad);
  gOnSceneImportReferenceCallbackId = MSceneMessage::addCallback(MSceneMessage::kAfterImportReference, onSceneLoad);
  gOnSceneLoadReferenceCallbackId = MSceneMessage::addCallback(MSceneMessage::kAfterLoadReference, onSceneLoad);
  gOnNodeAddedCallbackId = MDGMessage::addNodeAddedCallback(FabricSpliceBaseInterface::onNodeAdded);
  gOnNodeRemovedCallbackId = MDGMessage::addNodeRemovedCallback(FabricSpliceBaseInterface::onNodeRemoved);
  gOnNodeAddedDFGCallbackId = MDGMessage::addNodeAddedCallback(FabricDFGBaseInterface::onNodeAdded);
  gOnNodeRemovedDFGCallbackId = MDGMessage::addNodeRemovedCallback(FabricDFGBaseInterface::onNodeRemoved);
  gOnAnimCurveEditedCallbackId = MAnimMessage::addAnimCurveEditedCallback(FabricDFGBaseInterface::onAnimCurveEdited);
 
  plugin.registerData(FabricSpliceMayaData::typeName, FabricSpliceMayaData::id, FabricSpliceMayaData::creator);


  plugin.registerCommand("fabricSplice", FabricSpliceCommand::creator);//, FabricSpliceEditorCmd::newSyntax);
  plugin.registerCommand("fabricSpliceEditor", FabricSpliceEditorCmd::creator, FabricSpliceEditorCmd::newSyntax);
  plugin.registerCommand("fabricSpliceManipulation", FabricSpliceManipulationCmd::creator);

  plugin.registerNode("spliceMayaNode", FabricSpliceMayaNode::id, FabricSpliceMayaNode::creator, FabricSpliceMayaNode::initialize);
  plugin.registerNode("spliceMayaDeformer", FabricSpliceMayaDeformer::id, FabricSpliceMayaDeformer::creator, FabricSpliceMayaDeformer::initialize, MPxNode::kDeformerNode);


  MQtUtil::registerUIType("FabricSpliceEditor", FabricSpliceEditorWidget::creator, "fabricSpliceEditor");

  plugin.registerCommand("fabricDFG", FabricDFGWidgetCommand::creator, FabricDFGWidgetCommand::newSyntax);
  MQtUtil::registerUIType("FabricDFGWidget", FabricDFGWidget::creator, "fabricDFGWidget");

  plugin.registerNode("canvasNode", FabricDFGMayaNode::id, FabricDFGMayaNode::creator, FabricDFGMayaNode::initialize);
  plugin.registerNode("canvasDeformer", FabricDFGMayaDeformer::id, FabricDFGMayaDeformer::creator, FabricDFGMayaDeformer::initialize, MPxNode::kDeformerNode);

  plugin.registerCommand("FabricCanvasGetContextID", FabricDFGGetContextIDCommand::creator, FabricDFGGetContextIDCommand::newSyntax);
  plugin.registerCommand("FabricCanvasGetBindingID", FabricDFGGetBindingIDCommand::creator, FabricDFGGetBindingIDCommand::newSyntax);
  plugin.registerCommand("FabricCanvasDestroyClient", FabricDFGDestroyClientCommand::creator, FabricDFGDestroyClientCommand::newSyntax);

  MAYA_REGISTER_DFGUICMD( plugin, AddBackDrop );
  MAYA_REGISTER_DFGUICMD( plugin, AddFunc );
  MAYA_REGISTER_DFGUICMD( plugin, AddGet );
  MAYA_REGISTER_DFGUICMD( plugin, AddGraph );
  MAYA_REGISTER_DFGUICMD( plugin, AddPort );
  MAYA_REGISTER_DFGUICMD( plugin, AddSet );
  MAYA_REGISTER_DFGUICMD( plugin, AddVar );
  MAYA_REGISTER_DFGUICMD( plugin, Connect );
  MAYA_REGISTER_DFGUICMD( plugin, CreatePreset );
  MAYA_REGISTER_DFGUICMD( plugin, Disconnect );
  MAYA_REGISTER_DFGUICMD( plugin, DismissLoadDiags );
  MAYA_REGISTER_DFGUICMD( plugin, EditNode );
  MAYA_REGISTER_DFGUICMD( plugin, EditPort );
  MAYA_REGISTER_DFGUICMD( plugin, ExplodeNode );
  MAYA_REGISTER_DFGUICMD( plugin, ImplodeNodes );
  MAYA_REGISTER_DFGUICMD( plugin, InstPreset );
  MAYA_REGISTER_DFGUICMD( plugin, MoveNodes );
  MAYA_REGISTER_DFGUICMD( plugin, Paste );
  MAYA_REGISTER_DFGUICMD( plugin, RemoveNodes );
  MAYA_REGISTER_DFGUICMD( plugin, RemovePort );
  MAYA_REGISTER_DFGUICMD( plugin, RenamePort );
  MAYA_REGISTER_DFGUICMD( plugin, ReorderPorts );
  MAYA_REGISTER_DFGUICMD( plugin, ResizeBackDrop );
  MAYA_REGISTER_DFGUICMD( plugin, SetArgValue );
  MAYA_REGISTER_DFGUICMD( plugin, SetCode );
  MAYA_REGISTER_DFGUICMD( plugin, SetExtDeps );
  MAYA_REGISTER_DFGUICMD( plugin, SetNodeComment );
  MAYA_REGISTER_DFGUICMD( plugin, SetPortDefaultValue );
  MAYA_REGISTER_DFGUICMD( plugin, SetRefVarPath );
  MAYA_REGISTER_DFGUICMD( plugin, SplitFromPreset );

  plugin.registerCommand(
    "dfgImportJSON",
    FabricDFGImportJSONCommand::creator,
    FabricDFGImportJSONCommand::newSyntax
    );
  plugin.registerCommand(
    "dfgReloadJSON",
    FabricDFGReloadJSONCommand::creator,
    FabricDFGReloadJSONCommand::newSyntax
    );
  plugin.registerCommand(
    "dfgExportJSON",
    FabricDFGExportJSONCommand::creator,
    FabricDFGExportJSONCommand::newSyntax
    );

  plugin.registerCommand(
    "FabricCanvasSetExecuteShared",
    FabricCanvasSetExecuteSharedCommand::creator,
    FabricCanvasSetExecuteSharedCommand::newSyntax
    );

  plugin.registerCommand("fabricUpgradeAttrs", FabricUpgradeAttrCommand::creator, FabricUpgradeAttrCommand::newSyntax);

  MString pluginPath = plugin.loadPath();
  MString lastFolder("plug-ins");
  MString moduleFolder = pluginPath.substring(0, pluginPath.length() - lastFolder.length() - 2);
  initModuleFolder(moduleFolder);

  FabricSplice::Initialize();
  FabricSplice::Logging::setLogFunc(mayaLogFunc);
  FabricSplice::Logging::setLogErrorFunc(mayaLogErrorFunc);
  FabricSplice::Logging::setKLReportFunc(mayaKLReportFunc);
  FabricSplice::Logging::setKLStatusFunc(mayaKLStatusFunc);
  FabricSplice::Logging::setCompilerErrorFunc(mayaCompilerErrorFunc);
  // FabricSplice::SceneManagement::setManipulationFunc(FabricSpliceBaseInterface::manipulationCallback);

  MGlobal::executePythonCommandOnIdle("import AEspliceMayaNodeTemplate", true);
  MGlobal::executePythonCommandOnIdle("import AEcanvasNodeTemplate", true);

  if (MGlobal::mayaState() == MGlobal::kInteractive)
    FabricSplice::SetLicenseType(FabricCore::ClientLicenseType_Interactive);
  else
    FabricSplice::SetLicenseType(FabricCore::ClientLicenseType_Compute);

  return status;
}

#if defined(OSMac_)
__attribute__ ((visibility("default")))
#endif
MAYA_EXPORT uninitializePlugin(MObject obj)
{
  MFnPlugin plugin(obj);
  MStatus status;

  unloadMenu();

  plugin.deregisterCommand("fabricSplice");
  plugin.deregisterCommand("fabricUpgradeAttrs");
  plugin.deregisterCommand("FabricSpliceEditor");
  plugin.deregisterCommand("proceedToNextScene");
  plugin.deregisterNode(FabricSpliceMayaNode::id);
  plugin.deregisterNode(FabricSpliceMayaDeformer::id);

  MSceneMessage::removeCallback(gOnSceneSaveCallbackId);
  MSceneMessage::removeCallback(gOnSceneLoadCallbackId);
  MSceneMessage::removeCallback(gOnMayaExitCallbackId);
  MSceneMessage::removeCallback(gOnSceneImportCallbackId);
  MSceneMessage::removeCallback(gOnSceneExportCallbackId);
  MSceneMessage::removeCallback(gOnSceneCreateReferenceCallbackId);
  MSceneMessage::removeCallback(gOnSceneImportReferenceCallbackId);
  MSceneMessage::removeCallback(gOnSceneLoadReferenceCallbackId);
  MEventMessage::removeCallback(gOnModelPanelSetFocusCallbackId);

  // FE-6558 : Don't plug the render-callback if not interactive.
  // Otherwise it will crash on linux machine without DISPLAY
  if (MGlobal::mayaState() == MGlobal::kInteractive)
    FabricSpliceRenderCallback::unplug();

  MDGMessage::removeCallback(gOnNodeAddedCallbackId);
  MDGMessage::removeCallback(gOnNodeRemovedCallbackId);

  MDGMessage::removeCallback(gOnNodeAddedDFGCallbackId);
  MDGMessage::removeCallback(gOnNodeRemovedDFGCallbackId);
  MDGMessage::removeCallback(gOnAnimCurveEditedCallbackId);

  plugin.deregisterData(FabricSpliceMayaData::id);

  MQtUtil::deregisterUIType("FabricSpliceEditor");

  plugin.deregisterContextCommand("FabricSpliceToolContext", "FabricSpliceToolCommand");

  plugin.deregisterCommand("fabricDFG");
  MQtUtil::deregisterUIType("FabricDFGWidget");
  plugin.deregisterNode(FabricDFGMayaNode::id);
  plugin.deregisterNode(FabricDFGMayaDeformer::id);

  MAYA_DEREGISTER_DFGUICMD( plugin, AddBackDrop );
  MAYA_DEREGISTER_DFGUICMD( plugin, AddFunc );
  MAYA_DEREGISTER_DFGUICMD( plugin, AddGet );
  MAYA_DEREGISTER_DFGUICMD( plugin, AddGraph );
  MAYA_DEREGISTER_DFGUICMD( plugin, AddPort );
  MAYA_DEREGISTER_DFGUICMD( plugin, AddSet );
  MAYA_DEREGISTER_DFGUICMD( plugin, AddVar );
  MAYA_DEREGISTER_DFGUICMD( plugin, Connect );
  MAYA_DEREGISTER_DFGUICMD( plugin, CreatePreset );
  MAYA_DEREGISTER_DFGUICMD( plugin, Disconnect );
  MAYA_DEREGISTER_DFGUICMD( plugin, DismissLoadDiags );
  MAYA_DEREGISTER_DFGUICMD( plugin, EditNode );
  MAYA_DEREGISTER_DFGUICMD( plugin, EditPort );
  MAYA_DEREGISTER_DFGUICMD( plugin, ExplodeNode );
  MAYA_DEREGISTER_DFGUICMD( plugin, ImplodeNodes );
  MAYA_DEREGISTER_DFGUICMD( plugin, InstPreset );
  MAYA_DEREGISTER_DFGUICMD( plugin, MoveNodes );
  MAYA_DEREGISTER_DFGUICMD( plugin, Paste );
  MAYA_DEREGISTER_DFGUICMD( plugin, RemoveNodes );
  MAYA_DEREGISTER_DFGUICMD( plugin, RemovePort );
  MAYA_DEREGISTER_DFGUICMD( plugin, RenamePort );
  MAYA_DEREGISTER_DFGUICMD( plugin, ReorderPorts );
  MAYA_DEREGISTER_DFGUICMD( plugin, ResizeBackDrop );
  MAYA_DEREGISTER_DFGUICMD( plugin, SetArgValue );
  MAYA_DEREGISTER_DFGUICMD( plugin, SetCode );
  MAYA_DEREGISTER_DFGUICMD( plugin, SetExtDeps );
  MAYA_DEREGISTER_DFGUICMD( plugin, SetNodeComment );
  MAYA_DEREGISTER_DFGUICMD( plugin, SetPortDefaultValue );
  MAYA_DEREGISTER_DFGUICMD( plugin, SetRefVarPath );
  MAYA_DEREGISTER_DFGUICMD( plugin, SplitFromPreset );

  plugin.deregisterCommand( "dfgImportJSON" );
  plugin.deregisterCommand( "dfgReloadJSON" );
  plugin.deregisterCommand( "dfgExportJSON" );

  // [pzion 20141201] RM#3318: it seems that sending KL report statements
  // at this point, which might result from destructors called by
  // destroying the Core client, can cause Maya to crash on OS X.
  // 
  FabricSplice::Logging::setKLReportFunc(0);

  FabricSplice::DestroyClient();
  FabricSplice::Finalize();
  return status;
}

void loadMenu()
{
  MString cmd = "source \"FabricSpliceMenu.mel\"; FabricSpliceLoadMenu(\"";
  cmd += "FabricMaya";
  cmd += "\");";
  MStatus commandStatus = MGlobal::executeCommandOnIdle(cmd, false);
  if (commandStatus != MStatus::kSuccess)
    MGlobal::displayError("Failed to load FabricSpliceMenu.mel. Adjust your MAYA_SCRIPT_PATH!");
}

void unloadMenu()
{
  MString cmd = "source \"FabricSpliceMenu.mel\"; FabricSpliceUnloadMenu();";
  MStatus commandStatus = MGlobal::executeCommandOnIdle(cmd, false);
  if (commandStatus != MStatus::kSuccess)
    MGlobal::displayError("Failed to load FabricSpliceMenu.mel. Adjust your MAYA_SCRIPT_PATH!");
}

