
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
const MTypeId gLastValidNodeID(0x0011AE49);

MCallbackId gOnSceneNewCallbackId;
MCallbackId gOnSceneLoadCallbackId;
MCallbackId gOnSceneSaveCallbackId;
MCallbackId gOnMayaExitCallbackId;
MCallbackId gOnSceneImportCallbackId;
MCallbackId gOnSceneExportCallbackId;
MCallbackId gOnSceneReferenceCallbackId;
MCallbackId gOnSceneImportReferenceCallbackId;
#define gRenderCallbackCount 32
MCallbackId gRenderCallbacks[gRenderCallbackCount];
// bool gRenderCallbackUsed[gRenderCallbackCount];
// MString gRenderCallbackPanelName[gRenderCallbackCount];
MCallbackId gOnNodeAddedCallbackId;
MCallbackId gOnNodeRemovedCallbackId;
MCallbackId gBeforeSceneOpenCallbackId;
MCallbackId gOnModelPanelSetFocusCallbackId;

void onSceneSave(void *userData){

  MStatus status = MS::kSuccess;
  mayaSetLastLoadedScene(MFileIO::beforeSaveFilename(&status));
  if(mayaGetLastLoadedScene().length() == 0) // this happens during copy & paste
    return;

  std::vector<FabricSpliceBaseInterface*> instances = FabricSpliceBaseInterface::getInstances();

  for(int i = 0; i < instances.size(); ++i){
    FabricSpliceBaseInterface *node = instances[i];
    node->storePersistenceData(mayaGetLastLoadedScene(), &status);
  }
}

void onSceneNew(void *userData){
  FabricSpliceEditorWidget::postClearAll();
  FabricSpliceRenderCallback::sDrawContext.invalidate(); 
  FabricSplice::DestroyClient();
}

void onSceneLoad(void *userData){
  onSceneNew(userData);

  if(getenv("FABRIC_SPLICE_PROFILING") != NULL)
    FabricSplice::Logging::enableTimers();

  MStatus status = MS::kSuccess;
  mayaSetLastLoadedScene(MFileIO::currentFile());

  std::vector<FabricSpliceBaseInterface*> instances = FabricSpliceBaseInterface::getInstances();

  // each node will only restore once, so it's safe for import too
  FabricSplice::Logging::AutoTimer persistenceTimer("Maya::onSceneLoad");
  for(int i = 0; i < instances.size(); ++i){
    FabricSpliceBaseInterface *node = instances[i];
    node->restoreFromPersistenceData(mayaGetLastLoadedScene(), &status); 
    if( status != MS::kSuccess)
      return;
  }
  FabricSpliceEditorWidget::postClearAll();

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

  for(int i = 0; i < instances.size(); ++i){
    FabricSpliceBaseInterface *node = instances[i];
    node->resetInternalData();
  }

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

  if(panelName == "modelPanel0")
    gRenderCallbacks[0] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel0", FabricSpliceRenderCallback::draw);
  else if(panelName == "modelPanel1")
    gRenderCallbacks[1] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel1", FabricSpliceRenderCallback::draw);
  else if(panelName == "modelPanel2")
    gRenderCallbacks[2] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel2", FabricSpliceRenderCallback::draw);
  else if(panelName == "modelPanel3")
    gRenderCallbacks[3] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel3", FabricSpliceRenderCallback::draw);
  else if(panelName == "modelPanel4")
    gRenderCallbacks[4] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel4", FabricSpliceRenderCallback::draw);
  else if(panelName == "modelPanel5")
    gRenderCallbacks[5] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel5", FabricSpliceRenderCallback::draw);
  else if(panelName == "modelPanel6")
    gRenderCallbacks[6] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel6", FabricSpliceRenderCallback::draw);
  else if(panelName == "modelPanel7")
    gRenderCallbacks[7] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel7", FabricSpliceRenderCallback::draw);
  else if(panelName == "modelPanel8")
    gRenderCallbacks[8] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel8", FabricSpliceRenderCallback::draw);
  else if(panelName == "modelPanel9")
    gRenderCallbacks[9] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel9", FabricSpliceRenderCallback::draw);
  else if(panelName == "modelPanel10")
    gRenderCallbacks[10] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel10", FabricSpliceRenderCallback::draw);
  else if(panelName == "modelPanel11")
    gRenderCallbacks[11] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel11", FabricSpliceRenderCallback::draw);
  else if(panelName == "modelPanel12")
    gRenderCallbacks[12] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel12", FabricSpliceRenderCallback::draw);
  else if(panelName == "modelPanel13")
    gRenderCallbacks[13] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel13", FabricSpliceRenderCallback::draw);
  else if(panelName == "modelPanel14")
    gRenderCallbacks[14] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel14", FabricSpliceRenderCallback::draw);
  else if(panelName == "modelPanel15")
    gRenderCallbacks[15] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel15", FabricSpliceRenderCallback::draw);
  else if(panelName == "modelPanel16")
    gRenderCallbacks[16] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel16", FabricSpliceRenderCallback::draw);
  else if(panelName == "modelPanel17")
    gRenderCallbacks[17] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel17", FabricSpliceRenderCallback::draw);
  else if(panelName == "modelPanel18")
    gRenderCallbacks[18] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel18", FabricSpliceRenderCallback::draw);
  else if(panelName == "modelPanel19")
    gRenderCallbacks[19] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel19", FabricSpliceRenderCallback::draw);
  else if(panelName == "modelPanel20")
    gRenderCallbacks[20] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel20", FabricSpliceRenderCallback::draw);
  else if(panelName == "modelPanel21")
    gRenderCallbacks[21] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel21", FabricSpliceRenderCallback::draw);
  else if(panelName == "modelPanel22")
    gRenderCallbacks[22] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel22", FabricSpliceRenderCallback::draw);
  else if(panelName == "modelPanel23")
    gRenderCallbacks[23] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel23", FabricSpliceRenderCallback::draw);
  else if(panelName == "modelPanel24")
    gRenderCallbacks[24] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel24", FabricSpliceRenderCallback::draw);
  else if(panelName == "modelPanel25")
    gRenderCallbacks[25] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel25", FabricSpliceRenderCallback::draw);
  else if(panelName == "modelPanel26")
    gRenderCallbacks[26] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel26", FabricSpliceRenderCallback::draw);
  else if(panelName == "modelPanel27")
    gRenderCallbacks[27] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel27", FabricSpliceRenderCallback::draw);
  else if(panelName == "modelPanel28")
    gRenderCallbacks[28] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel28", FabricSpliceRenderCallback::draw);
  else if(panelName == "modelPanel29")
    gRenderCallbacks[29] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel29", FabricSpliceRenderCallback::draw);
  else if(panelName == "modelPanel30")
    gRenderCallbacks[30] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel30", FabricSpliceRenderCallback::draw);
  else if(panelName == "modelPanel31")
    gRenderCallbacks[31] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel31", FabricSpliceRenderCallback::draw);

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

  gOnSceneSaveCallbackId = MSceneMessage::addCallback(MSceneMessage::kBeforeSave, onSceneSave);
  gOnSceneLoadCallbackId = MSceneMessage::addCallback(MSceneMessage::kAfterOpen, onSceneLoad);
  gBeforeSceneOpenCallbackId = MSceneMessage::addCallback(MSceneMessage::kBeforeOpen, onSceneNew);
  gOnSceneNewCallbackId = MSceneMessage::addCallback(MSceneMessage::kBeforeNew, onSceneNew);
  gOnMayaExitCallbackId = MSceneMessage::addCallback(MSceneMessage::kMayaExiting, onMayaExiting);
  gOnSceneExportCallbackId = MSceneMessage::addCallback(MSceneMessage::kBeforeExport, onSceneSave);
  gOnSceneImportCallbackId = MSceneMessage::addCallback(MSceneMessage::kAfterImport, onSceneLoad);
  gOnSceneReferenceCallbackId = MSceneMessage::addCallback(MSceneMessage::kAfterLoadReference, onSceneLoad);
  gOnSceneImportReferenceCallbackId = MSceneMessage::addCallback(MSceneMessage::kAfterImportReference, onSceneLoad);
  gRenderCallbacks[0] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel0", FabricSpliceRenderCallback::draw);
  gRenderCallbacks[1] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel1", FabricSpliceRenderCallback::draw);
  gRenderCallbacks[2] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel2", FabricSpliceRenderCallback::draw);
  gRenderCallbacks[3] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel3", FabricSpliceRenderCallback::draw);
  gRenderCallbacks[4] = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel4", FabricSpliceRenderCallback::draw);
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
  MSceneMessage::removeCallback(gOnSceneReferenceCallbackId);
  MSceneMessage::removeCallback(gOnSceneImportReferenceCallbackId);
  MEventMessage::removeCallback(gOnModelPanelSetFocusCallbackId);

  for(unsigned int i=0;i<gRenderCallbackCount;i++)
    MUiMessage::removeCallback(gRenderCallbacks[i]);

  MDGMessage::removeCallback(gOnNodeAddedCallbackId);
  MDGMessage::removeCallback(gOnNodeRemovedCallbackId);

  plugin.deregisterData(FabricSpliceMayaData::id);

  MQtUtil::deregisterUIType("FabricSpliceEditor");

  plugin.deregisterContextCommand("FabricSpliceToolContext", "FabricSpliceToolCommand");

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

