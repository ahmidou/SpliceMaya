
// [andrew 20130620] all hell breaks loose in Linux if these aren't included first
#include <QtCore/QDataStream>
#include <QtCore/QMetaType>
#include <QtGui/QTextEdit>
#include <QtGui/QComboBox>
#include <QtCore/QEvent>

#include "plugin.h"
#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>
#include <maya/MSceneMessage.h>
#include <maya/MDGMessage.h>
#include <maya/MUiMessage.h>
#include <maya/MEventMessage.h>
#include <maya/MQtUtil.h>
#include <maya/MCommandResult.h>
#include <maya/MFileIO.h>

#include <FabricSplice.h>
#include "FabricSpliceMayaNode.h"
#include "FabricSpliceMayaDeformer.h"
#include "FabricSpliceCommand.h"
#include "FabricSpliceEditorCmd.h"
#include "FabricSpliceMayaData.h"
#include "FabricSpliceToolContext.h"
#include "FabricSpliceRenderCallback.h"
#include "ProceedToNextSceneCommand.h"
#include "FabricDFGWidgetCommand.h"
#include "FabricDFGWidget.h"
#include "FabricDFGMayaNode.h"
#include "FabricDFGCommands.h"
#include "FabricSpliceHelpers.h"

#ifdef _MSC_VER
  #define MAYA_EXPORT extern "C" __declspec(dllexport) MStatus _cdecl
#else
  #define MAYA_EXPORT extern "C" MStatus
#endif

// FE Owned IDs 0x0011AE40 - 0x0011AF3F
const MTypeId gFirstValidNodeID(0x0011AE40);
// FabricSpliceMayaNode 0x0011AE41
// FabricSpliceMayaDeformer 0x0011AE42
// FabricSpliceInlineGeometry 0x0011AE43 /* no longer in use, but not available */
// FabricSpliceMayaDebugger 0x0011AE44 /* no longer in use, but not available */
// FabricSpliceMayaData 0x0011AE45
// FabricDFGMayaNode 0x0011AE46 // dfgMayaNode
// FabricDFGMayaNode 0x0011AE47 // canvasNode
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
#define gRenderCallbackCount 32
MCallbackId gRenderCallbacks[gRenderCallbackCount];
bool gRenderCallbacksSet[gRenderCallbackCount];
// bool gRenderCallbackUsed[gRenderCallbackCount];
// MString gRenderCallbackPanelName[gRenderCallbackCount];
MCallbackId gOnNodeAddedCallbackId;
MCallbackId gOnNodeRemovedCallbackId;
MCallbackId gBeforeSceneOpenCallbackId;
MCallbackId gOnModelPanelSetFocusCallbackId;

void resetRenderCallbacks() {
  for(unsigned int i=0;i<gRenderCallbackCount;i++)
    gRenderCallbacksSet[i] = false;
}

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
 
  FabricSplice::DestroyClient();
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

void onModelPanelSetFocus(void * client)
{
  MString panelName;
  MGlobal::executeCommand("getPanel -wf;", panelName, false);

  if(panelName == "modelPanel0" && !gRenderCallbacksSet[0]) {
    MStatus status;
    gRenderCallbacks[0] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel0", FabricSpliceRenderCallback::draw, NULL, &status);
    gRenderCallbacksSet[0] = status == MStatus::kSuccess;
  }
  else if(panelName == "modelPanel1" && !gRenderCallbacksSet[1]) {
    MStatus status;
    gRenderCallbacks[1] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel1", FabricSpliceRenderCallback::draw, NULL, &status);
    gRenderCallbacksSet[1] = status == MStatus::kSuccess;
  }
  else if(panelName == "modelPanel2" && !gRenderCallbacksSet[2]) {
    MStatus status;
    gRenderCallbacks[2] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel2", FabricSpliceRenderCallback::draw, NULL, &status);
    gRenderCallbacksSet[2] = status == MStatus::kSuccess;
  }
  else if(panelName == "modelPanel3" && !gRenderCallbacksSet[3]) {
    MStatus status;
    gRenderCallbacks[3] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel3", FabricSpliceRenderCallback::draw, NULL, &status);
    gRenderCallbacksSet[3] = status == MStatus::kSuccess;
  }
  else if(panelName == "modelPanel4" && !gRenderCallbacksSet[4]) {
    MStatus status;
    gRenderCallbacks[4] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel4", FabricSpliceRenderCallback::draw, NULL, &status);
    gRenderCallbacksSet[4] = status == MStatus::kSuccess;
  }
  else if(panelName == "modelPanel5" && !gRenderCallbacksSet[5]) {
    MStatus status;
    gRenderCallbacks[5] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel5", FabricSpliceRenderCallback::draw, NULL, &status);
    gRenderCallbacksSet[5] = status == MStatus::kSuccess;
  }
  else if(panelName == "modelPanel6" && !gRenderCallbacksSet[6]) {
    MStatus status;
    gRenderCallbacks[6] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel6", FabricSpliceRenderCallback::draw, NULL, &status);
    gRenderCallbacksSet[6] = status == MStatus::kSuccess;
  }
  else if(panelName == "modelPanel7" && !gRenderCallbacksSet[7]) {
    MStatus status;
    gRenderCallbacks[7] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel7", FabricSpliceRenderCallback::draw, NULL, &status);
    gRenderCallbacksSet[7] = status == MStatus::kSuccess;
  }
  else if(panelName == "modelPanel8" && !gRenderCallbacksSet[8]) {
    MStatus status;
    gRenderCallbacks[8] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel8", FabricSpliceRenderCallback::draw, NULL, &status);
    gRenderCallbacksSet[8] = status == MStatus::kSuccess;
  }
  else if(panelName == "modelPanel9" && !gRenderCallbacksSet[9]) {
    MStatus status;
    gRenderCallbacks[9] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel9", FabricSpliceRenderCallback::draw, NULL, &status);
    gRenderCallbacksSet[9] = status == MStatus::kSuccess;
  }
  else if(panelName == "modelPanel10" && !gRenderCallbacksSet[10]) {
    MStatus status;
    gRenderCallbacks[10] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel10", FabricSpliceRenderCallback::draw, NULL, &status);
    gRenderCallbacksSet[10] = status == MStatus::kSuccess;
  }
  else if(panelName == "modelPanel11" && !gRenderCallbacksSet[11]) {
    MStatus status;
    gRenderCallbacks[11] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel11", FabricSpliceRenderCallback::draw, NULL, &status);
    gRenderCallbacksSet[11] = status == MStatus::kSuccess;
  }
  else if(panelName == "modelPanel12" && !gRenderCallbacksSet[12]) {
    MStatus status;
    gRenderCallbacks[12] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel12", FabricSpliceRenderCallback::draw, NULL, &status);
    gRenderCallbacksSet[12] = status == MStatus::kSuccess;
  }
  else if(panelName == "modelPanel13" && !gRenderCallbacksSet[13]) {
    MStatus status;
    gRenderCallbacks[13] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel13", FabricSpliceRenderCallback::draw, NULL, &status);
    gRenderCallbacksSet[13] = status == MStatus::kSuccess;
  }
  else if(panelName == "modelPanel14" && !gRenderCallbacksSet[14]) {
    MStatus status;
    gRenderCallbacks[14] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel14", FabricSpliceRenderCallback::draw, NULL, &status);
    gRenderCallbacksSet[14] = status == MStatus::kSuccess;
  }
  else if(panelName == "modelPanel15" && !gRenderCallbacksSet[15]) {
    MStatus status;
    gRenderCallbacks[15] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel15", FabricSpliceRenderCallback::draw, NULL, &status);
    gRenderCallbacksSet[15] = status == MStatus::kSuccess;
  }
  else if(panelName == "modelPanel16" && !gRenderCallbacksSet[16]) {
    MStatus status;
    gRenderCallbacks[16] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel16", FabricSpliceRenderCallback::draw, NULL, &status);
    gRenderCallbacksSet[16] = status == MStatus::kSuccess;
  }
  else if(panelName == "modelPanel17" && !gRenderCallbacksSet[17]) {
    MStatus status;
    gRenderCallbacks[17] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel17", FabricSpliceRenderCallback::draw, NULL, &status);
    gRenderCallbacksSet[17] = status == MStatus::kSuccess;
  }
  else if(panelName == "modelPanel18" && !gRenderCallbacksSet[18]) {
    MStatus status;
    gRenderCallbacks[18] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel18", FabricSpliceRenderCallback::draw, NULL, &status);
    gRenderCallbacksSet[18] = status == MStatus::kSuccess;
  }
  else if(panelName == "modelPanel19" && !gRenderCallbacksSet[19]) {
    MStatus status;
    gRenderCallbacks[19] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel19", FabricSpliceRenderCallback::draw, NULL, &status);
    gRenderCallbacksSet[19] = status == MStatus::kSuccess;
  }
  else if(panelName == "modelPanel20" && !gRenderCallbacksSet[20]) {
    MStatus status;
    gRenderCallbacks[20] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel20", FabricSpliceRenderCallback::draw, NULL, &status);
    gRenderCallbacksSet[20] = status == MStatus::kSuccess;
  }
  else if(panelName == "modelPanel21" && !gRenderCallbacksSet[21]) {
    MStatus status;
    gRenderCallbacks[21] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel21", FabricSpliceRenderCallback::draw, NULL, &status);
    gRenderCallbacksSet[21] = status == MStatus::kSuccess;
  }
  else if(panelName == "modelPanel22" && !gRenderCallbacksSet[22]) {
    MStatus status;
    gRenderCallbacks[22] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel22", FabricSpliceRenderCallback::draw, NULL, &status);
    gRenderCallbacksSet[22] = status == MStatus::kSuccess;
  }
  else if(panelName == "modelPanel23" && !gRenderCallbacksSet[23]) {
    MStatus status;
    gRenderCallbacks[23] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel23", FabricSpliceRenderCallback::draw, NULL, &status);
    gRenderCallbacksSet[23] = status == MStatus::kSuccess;
  }
  else if(panelName == "modelPanel24" && !gRenderCallbacksSet[24]) {
    MStatus status;
    gRenderCallbacks[24] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel24", FabricSpliceRenderCallback::draw, NULL, &status);
    gRenderCallbacksSet[24] = status == MStatus::kSuccess;
  }
  else if(panelName == "modelPanel25" && !gRenderCallbacksSet[25]) {
    MStatus status;
    gRenderCallbacks[25] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel25", FabricSpliceRenderCallback::draw, NULL, &status);
    gRenderCallbacksSet[25] = status == MStatus::kSuccess;
  }
  else if(panelName == "modelPanel26" && !gRenderCallbacksSet[26]) {
    MStatus status;
    gRenderCallbacks[26] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel26", FabricSpliceRenderCallback::draw, NULL, &status);
    gRenderCallbacksSet[26] = status == MStatus::kSuccess;
  }
  else if(panelName == "modelPanel27" && !gRenderCallbacksSet[27]) {
    MStatus status;
    gRenderCallbacks[27] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel27", FabricSpliceRenderCallback::draw, NULL, &status);
    gRenderCallbacksSet[27] = status == MStatus::kSuccess;
  }
  else if(panelName == "modelPanel28" && !gRenderCallbacksSet[28]) {
    MStatus status;
    gRenderCallbacks[28] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel28", FabricSpliceRenderCallback::draw, NULL, &status);
    gRenderCallbacksSet[28] = status == MStatus::kSuccess;
  }
  else if(panelName == "modelPanel29" && !gRenderCallbacksSet[29]) {
    MStatus status;
    gRenderCallbacks[29] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel29", FabricSpliceRenderCallback::draw, NULL, &status);
    gRenderCallbacksSet[29] = status == MStatus::kSuccess;
  }
  else if(panelName == "modelPanel30" && !gRenderCallbacksSet[30]) {
    MStatus status;
    gRenderCallbacks[30] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel30", FabricSpliceRenderCallback::draw, NULL, &status);
    gRenderCallbacksSet[30] = status == MStatus::kSuccess;
  }
  else if(panelName == "modelPanel31" && !gRenderCallbacksSet[31]) {
    MStatus status;
    gRenderCallbacks[31] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel31", FabricSpliceRenderCallback::draw, NULL, &status);
    gRenderCallbacksSet[31] = status == MStatus::kSuccess;
  }

  MGlobal::executeCommandOnIdle("refresh;", false);
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
  MFnPlugin plugin(obj, "FabricMaya", FabricSplice::GetFabricVersionStr(), "Any");
  MStatus status;

  status = plugin.registerContextCommand("FabricSpliceToolContext", FabricSpliceToolContextCmd::creator, "FabricSpliceToolCommand", FabricSpliceToolCmd::creator  );

  loadMenu();

  resetRenderCallbacks();

  gOnSceneSaveCallbackId = MSceneMessage::addCallback(MSceneMessage::kBeforeSave, onSceneSave);
  gOnSceneLoadCallbackId = MSceneMessage::addCallback(MSceneMessage::kAfterOpen, onSceneLoad);
  gBeforeSceneOpenCallbackId = MSceneMessage::addCallback(MSceneMessage::kBeforeOpen, onSceneNew);
  gOnSceneNewCallbackId = MSceneMessage::addCallback(MSceneMessage::kBeforeNew, onSceneNew);
  gOnMayaExitCallbackId = MSceneMessage::addCallback(MSceneMessage::kMayaExiting, onMayaExiting);
  gOnSceneExportCallbackId = MSceneMessage::addCallback(MSceneMessage::kBeforeExport, onSceneSave);
  gOnSceneImportCallbackId = MSceneMessage::addCallback(MSceneMessage::kAfterImport, onSceneLoad);
  gOnSceneCreateReferenceCallbackId = MSceneMessage::addCallback(MSceneMessage::kAfterCreateReference, onSceneLoad);
  gOnSceneImportReferenceCallbackId = MSceneMessage::addCallback(MSceneMessage::kAfterImportReference, onSceneLoad);
  gOnSceneLoadReferenceCallbackId = MSceneMessage::addCallback(MSceneMessage::kAfterLoadReference, onSceneLoad);
  gRenderCallbacks[0] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel0", FabricSpliceRenderCallback::draw);
  gRenderCallbacksSet[0] = true;
  gRenderCallbacks[1] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel1", FabricSpliceRenderCallback::draw);
  gRenderCallbacksSet[1] = true;
  gRenderCallbacks[2] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel2", FabricSpliceRenderCallback::draw);
  gRenderCallbacksSet[2] = true;
  gRenderCallbacks[3] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel3", FabricSpliceRenderCallback::draw);
  gRenderCallbacksSet[3] = true;
  gRenderCallbacks[4] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel4", FabricSpliceRenderCallback::draw);
  gRenderCallbacksSet[4] = true;
  gOnNodeAddedCallbackId = MDGMessage::addNodeAddedCallback(FabricSpliceBaseInterface::onNodeAdded);
  gOnNodeRemovedCallbackId = MDGMessage::addNodeRemovedCallback(FabricSpliceBaseInterface::onNodeRemoved);
  gOnModelPanelSetFocusCallbackId = MEventMessage::addEventCallback("ModelPanelSetFocus", &onModelPanelSetFocus);

  plugin.registerData(FabricSpliceMayaData::typeName, FabricSpliceMayaData::id, FabricSpliceMayaData::creator);


  plugin.registerCommand("fabricSplice", FabricSpliceCommand::creator);//, FabricSpliceEditorCmd::newSyntax);
  plugin.registerCommand("fabricSpliceEditor", FabricSpliceEditorCmd::creator, FabricSpliceEditorCmd::newSyntax);
  plugin.registerCommand("fabricSpliceManipulation", FabricSpliceManipulationCmd::creator);
  plugin.registerCommand("proceedToNextScene", ProceedToNextSceneCommand::creator);//, FabricSpliceEditorCmd::newSyntax);

  plugin.registerNode("spliceMayaNode", FabricSpliceMayaNode::id, FabricSpliceMayaNode::creator, FabricSpliceMayaNode::initialize);
  plugin.registerNode("spliceMayaDeformer", FabricSpliceMayaDeformer::id, FabricSpliceMayaDeformer::creator, FabricSpliceMayaDeformer::initialize, MPxNode::kDeformerNode);


  MQtUtil::registerUIType("FabricSpliceEditor", FabricSpliceEditorWidget::creator, "fabricSpliceEditor");

  plugin.registerCommand("fabricDFG", FabricDFGWidgetCommand::creator, FabricDFGWidgetCommand::newSyntax);
  MQtUtil::registerUIType("FabricDFGWidget", FabricDFGWidget::creator, "fabricDFGWidget");

  // obsolete node
  plugin.registerNode("dfgMayaNode", 0x0011AE46, FabricDFGMayaNode::creator, FabricDFGMayaNode::initialize);
  plugin.registerNode("canvasNode", FabricDFGMayaNode::id, FabricDFGMayaNode::creator, FabricDFGMayaNode::initialize);

  plugin.registerCommand("dfgGetContextID", FabricDFGGetContextIDCommand::creator, FabricDFGGetContextIDCommand::newSyntax);
  plugin.registerCommand("dfgGetBindingID", FabricDFGGetBindingIDCommand::creator, FabricDFGGetBindingIDCommand::newSyntax);

  MAYA_REGISTER_DFGUICMD( plugin, AddBackDrop );
  MAYA_REGISTER_DFGUICMD( plugin, AddFunc );
  MAYA_REGISTER_DFGUICMD( plugin, AddGet );
  MAYA_REGISTER_DFGUICMD( plugin, AddGraph );
  MAYA_REGISTER_DFGUICMD( plugin, AddPort );
  MAYA_REGISTER_DFGUICMD( plugin, AddSet );
  MAYA_REGISTER_DFGUICMD( plugin, AddVar );
  MAYA_REGISTER_DFGUICMD( plugin, Connect );
  MAYA_REGISTER_DFGUICMD( plugin, Disconnect );
  MAYA_REGISTER_DFGUICMD( plugin, ExplodeNode );
  MAYA_REGISTER_DFGUICMD( plugin, ImplodeNodes );
  MAYA_REGISTER_DFGUICMD( plugin, InstPreset );
  MAYA_REGISTER_DFGUICMD( plugin, MoveNodes );
  MAYA_REGISTER_DFGUICMD( plugin, Paste );
  MAYA_REGISTER_DFGUICMD( plugin, RemoveNodes );
  MAYA_REGISTER_DFGUICMD( plugin, RemovePort );
  MAYA_REGISTER_DFGUICMD( plugin, RenamePort );
  MAYA_REGISTER_DFGUICMD( plugin, ResizeBackDrop );
  MAYA_REGISTER_DFGUICMD( plugin, SetArgType );
  MAYA_REGISTER_DFGUICMD( plugin, SetArgValue );
  MAYA_REGISTER_DFGUICMD( plugin, SetCode );
  MAYA_REGISTER_DFGUICMD( plugin, SetNodeComment );
  MAYA_REGISTER_DFGUICMD( plugin, SetNodeTitle );
  MAYA_REGISTER_DFGUICMD( plugin, SetPortDefaultValue );
  MAYA_REGISTER_DFGUICMD( plugin, SetRefVarPath );
  MAYA_REGISTER_DFGUICMD( plugin, ReorderPorts );

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
  MGlobal::executePythonCommandOnIdle("import AEdfgMayaNodeTemplate", true);

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
  plugin.deregisterCommand("FabricSpliceEditor");
  plugin.deregisterCommand("proceedToNextScene");
  plugin.deregisterNode(0x0011AE46);
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

  for(unsigned int i=0;i<gRenderCallbackCount;i++)
    MUiMessage::removeCallback(gRenderCallbacks[i]);

  MDGMessage::removeCallback(gOnNodeAddedCallbackId);
  MDGMessage::removeCallback(gOnNodeRemovedCallbackId);

  plugin.deregisterData(FabricSpliceMayaData::id);

  MQtUtil::deregisterUIType("FabricSpliceEditor");

  plugin.deregisterContextCommand("FabricSpliceToolContext", "FabricSpliceToolCommand");

  plugin.deregisterCommand("fabricDFG");
  MQtUtil::deregisterUIType("FabricDFGWidget");
  plugin.deregisterNode(FabricDFGMayaNode::id);

  MAYA_DEREGISTER_DFGUICMD( plugin, AddBackDrop );
  MAYA_DEREGISTER_DFGUICMD( plugin, AddFunc );
  MAYA_DEREGISTER_DFGUICMD( plugin, AddGet );
  MAYA_DEREGISTER_DFGUICMD( plugin, AddGraph );
  MAYA_DEREGISTER_DFGUICMD( plugin, AddPort );
  MAYA_DEREGISTER_DFGUICMD( plugin, AddSet );
  MAYA_DEREGISTER_DFGUICMD( plugin, AddVar );
  MAYA_DEREGISTER_DFGUICMD( plugin, Connect );
  MAYA_DEREGISTER_DFGUICMD( plugin, Disconnect );
  MAYA_DEREGISTER_DFGUICMD( plugin, ExplodeNode );
  MAYA_DEREGISTER_DFGUICMD( plugin, ImplodeNodes );
  MAYA_DEREGISTER_DFGUICMD( plugin, InstPreset );
  MAYA_DEREGISTER_DFGUICMD( plugin, MoveNodes );
  MAYA_DEREGISTER_DFGUICMD( plugin, Paste );
  MAYA_DEREGISTER_DFGUICMD( plugin, RemoveNodes );
  MAYA_DEREGISTER_DFGUICMD( plugin, RemovePort );
  MAYA_DEREGISTER_DFGUICMD( plugin, RenamePort );
  MAYA_DEREGISTER_DFGUICMD( plugin, ResizeBackDrop );
  MAYA_DEREGISTER_DFGUICMD( plugin, SetArgType );
  MAYA_DEREGISTER_DFGUICMD( plugin, SetArgValue );
  MAYA_DEREGISTER_DFGUICMD( plugin, SetCode );
  MAYA_DEREGISTER_DFGUICMD( plugin, SetNodeComment );
  MAYA_DEREGISTER_DFGUICMD( plugin, SetNodeTitle );
  MAYA_DEREGISTER_DFGUICMD( plugin, SetPortDefaultValue );
  MAYA_DEREGISTER_DFGUICMD( plugin, SetRefVarPath );
  MAYA_DEREGISTER_DFGUICMD( plugin, ReorderPorts );

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

