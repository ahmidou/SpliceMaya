
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
// FabricDFGMayaNode 0x0011AE46
const MTypeId gLastValidNodeID(0x0011AE49);

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

#if defined(OSMac_)
__attribute__ ((visibility("default")))
#endif
MAYA_EXPORT initializePlugin(MObject obj)
{
  MFnPlugin plugin(obj, "FabricSpliceMaya", FabricSplice::GetFabricVersionStr(), "Any");
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
  plugin.registerNode("dfgMayaNode", FabricDFGMayaNode::id, FabricDFGMayaNode::creator, FabricDFGMayaNode::initialize);

  plugin.registerCommand("dfgGetContextID", FabricDFGGetContextIDCommand::creator, FabricDFGGetContextIDCommand::newSyntax);
  plugin.registerCommand("dfgGetBindingID", FabricDFGGetBindingIDCommand::creator, FabricDFGGetBindingIDCommand::newSyntax);

  plugin.registerCommand(
    FabricUI::DFG::DFGUICmd_InstPreset::CmdName().c_str(),
    FabricDFGInstPresetCommand::creator,
    FabricDFGInstPresetCommand::newSyntax
    );
  // plugin.registerCommand(
  //   FabricUI::DFG::DFGUICmd_AddFunc::CmdName().c_str(),
  //   FabricDFGAddFuncCommand::creator,
  //   FabricDFGAddFuncCommand::newSyntax
  //   );
  // plugin.registerCommand(
  //   FabricUI::DFG::DFGUICmd_AddGraph::CmdName().c_str(),
  //   FabricDFGAddGraphCommand::creator,
  //   FabricDFGAddGraphCommand::newSyntax
  //   );
  // plugin.registerCommand(
  //   FabricUI::DFG::DFGUICmd_AddVar::CmdName().c_str(),
  //   FabricDFGAddVarCommand::creator,
  //   FabricDFGAddVarCommand::newSyntax
  //   );
  // plugin.registerCommand(
  //   FabricUI::DFG::DFGUICmd_AddGet::CmdName().c_str(),
  //   FabricDFGAddGetCommand::creator,
  //   FabricDFGAddGetCommand::newSyntax
  //   );
  // plugin.registerCommand(
  //   FabricUI::DFG::DFGUICmd_AddSet::CmdName().c_str(),
  //   FabricDFGAddSetCommand::creator,
  //   FabricDFGAddSetCommand::newSyntax
  //   );
  plugin.registerCommand(
    FabricUI::DFG::DFGUICmd_Connect::CmdName().c_str(),
    FabricDFGConnectCommand::creator,
    FabricDFGConnectCommand::newSyntax
    );
  plugin.registerCommand(
    FabricUI::DFG::DFGUICmd_MoveNodes::CmdName().c_str(),
    FabricDFGMoveNodesCommand::creator,
    FabricDFGMoveNodesCommand::newSyntax
    );
  plugin.registerCommand(
    FabricUI::DFG::DFGUICmd_AddPort::CmdName().c_str(),
    FabricDFGAddPortCommand::creator,
    FabricDFGAddPortCommand::newSyntax
    );
  plugin.registerCommand(
    FabricUI::DFG::DFGUICmd_SetArgType::CmdName().c_str(),
    FabricDFGSetArgTypeCommand::creator,
    FabricDFGSetArgTypeCommand::newSyntax
    );
  // plugin.registerCommand(
  //   FabricUI::DFG::DFGUICmd_Disconnect::CmdName().c_str(),
  //   FabricDFGDisconnectCommand::creator,
  //   FabricDFGDisconnectCommand::newSyntax
  //   );
  // plugin.registerCommand(
  //   FabricUI::DFG::DFGUICmd_RemoveNodes::CmdName().c_str(),
  //   FabricDFGRemoveNodesCommand::creator,
  //   FabricDFGRemoveNodesCommand::newSyntax
  //   );
  // plugin.registerCommand("dfgRenameNode", FabricDFGRenameNodeCommand::creator, FabricDFGRenameNodeCommand::newSyntax);
  // plugin.registerCommand("dfgRemovePort", FabricDFGRemovePortCommand::creator, FabricDFGRemovePortCommand::newSyntax);
  // plugin.registerCommand("dfgRenamePort", FabricDFGRenamePortCommand::creator, FabricDFGRenamePortCommand::newSyntax);
  // plugin.registerCommand("dfgSetArg", FabricDFGSetArgCommand::creator, FabricDFGSetArgCommand::newSyntax);
  // plugin.registerCommand("dfgSetDefaultValue", FabricDFGSetDefaultValueCommand::creator, FabricDFGSetDefaultValueCommand::newSyntax);
  // plugin.registerCommand("dfgSetCode", FabricDFGSetCodeCommand::creator, FabricDFGSetCodeCommand::newSyntax);
  // plugin.registerCommand("dfgGetDesc", FabricDFGGetDescCommand::creator, FabricDFGGetDescCommand::newSyntax);
  // plugin.registerCommand("dfgImportJSON", FabricDFGImportJSONCommand::creator, FabricDFGImportJSONCommand::newSyntax);
  // plugin.registerCommand("dfgExportJSON", FabricDFGExportJSONCommand::creator, FabricDFGExportJSONCommand::newSyntax);
  // plugin.registerCommand("dfgReloadJSON", FabricDFGReloadJSONCommand::creator, FabricDFGReloadJSONCommand::newSyntax);
  // plugin.registerCommand("dfgSetNodeCacheRule", FabricDFGSetNodeCacheRuleCommand::creator, FabricDFGSetNodeCacheRuleCommand::newSyntax);
  // plugin.registerCommand("dfgCopy", FabricDFGCopyCommand::creator, FabricDFGCopyCommand::newSyntax);
  // plugin.registerCommand("dfgPaste", FabricDFGPasteCommand::creator, FabricDFGPasteCommand::newSyntax);
  // plugin.registerCommand("dfgImplodeNodes", FabricDFGImplodeNodesCommand::creator, FabricDFGImplodeNodesCommand::newSyntax);
  // plugin.registerCommand("dfgExplodeNode", FabricDFGExplodeNodeCommand::creator, FabricDFGExplodeNodeCommand::newSyntax);
  // plugin.registerCommand("dfgAddVar", FabricDFGAddVarCommand::creator, FabricDFGAddVarCommand::newSyntax);
  // plugin.registerCommand("dfgAddGet", FabricDFGAddGetCommand::creator, FabricDFGAddGetCommand::newSyntax);
  // plugin.registerCommand("dfgAddSet", FabricDFGAddSetCommand::creator, FabricDFGAddSetCommand::newSyntax);
  // plugin.registerCommand("dfgSetRefVarPath", FabricDFGSetRefVarPathCommand::creator, FabricDFGSetRefVarPathCommand::newSyntax);

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

  plugin.deregisterCommand("dfgGetContextID");
  plugin.deregisterCommand("dfgGetBindingID");
  plugin.deregisterCommand("dfgAddNode");
  plugin.deregisterCommand("dfgRemoveNode");
  plugin.deregisterCommand("dfgRenameNode");
  plugin.deregisterCommand("dfgAddEmptyFunc");
  plugin.deregisterCommand("dfgAddEmptyGraph");
  plugin.deregisterCommand("dfgAddConnection");
  plugin.deregisterCommand("dfgRemoveConnection");
  plugin.deregisterCommand("dfgAddPort");
  plugin.deregisterCommand("dfgRemovePort");
  plugin.deregisterCommand("dfgRenamePort");
  plugin.deregisterCommand("dfgSetArg");
  plugin.deregisterCommand("dfgSetDefaultValue");
  plugin.deregisterCommand("dfgSetCode");
  plugin.deregisterCommand("dfgGetDesc");
  plugin.deregisterCommand("dfgImportJSON");
  plugin.deregisterCommand("dfgExportJSON");
  plugin.deregisterCommand("dfgReloadJSON");
  plugin.deregisterCommand("dfgSetNodeCacheRule");
  plugin.deregisterCommand("dfgCopy");
  plugin.deregisterCommand("dfgPaste");
  plugin.deregisterCommand("dfgImplodeNodes");
  plugin.deregisterCommand("dfgExplodeNode");
  plugin.deregisterCommand("dfgAddVar");
  plugin.deregisterCommand("dfgAddGet");
  plugin.deregisterCommand("dfgAddSet");
  plugin.deregisterCommand("dfgSetRefVarPath");

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
  cmd += "FabricSpliceMaya";
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

