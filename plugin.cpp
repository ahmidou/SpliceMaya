
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
MCallbackId gOnSceneReferenceCallbackId;
MCallbackId gOnSceneImportReferenceCallbackId;
MCallbackId gRenderCallback0;
MCallbackId gRenderCallback1;
MCallbackId gRenderCallback2;
MCallbackId gRenderCallback3;
MCallbackId gRenderCallback4;
MCallbackId gOnNodeAddedCallbackId;
MCallbackId gOnNodeRemovedCallbackId;
MCallbackId gBeforeSceneOpenCallbackId;


MString gLastLoadedScene;
MString mayaGetLastLoadedScene()
{
  return gLastLoadedScene;
}

MString gModuleFolder;
void initModuleFolder(MFnPlugin &plugin){
  MString pluginPath = plugin.loadPath();
  MString lastFolder("plug-ins");

  gModuleFolder = pluginPath.substring(0, pluginPath.length() - lastFolder.length() - 2);
}

MString getModuleFolder()
{
  return gModuleFolder;
}

void onSceneSave(void *userData){

  MStatus status = MS::kSuccess;
  gLastLoadedScene = MFileIO::beforeSaveFilename(&status);
  if(gLastLoadedScene.length() == 0) // this happens during copy & paste
    return;

  std::vector<FabricSpliceBaseInterface*> instances = FabricSpliceBaseInterface::getInstances();

  for(int i = 0; i < instances.size(); ++i){
    FabricSpliceBaseInterface *node = instances[i];
    node->storePersistenceData(gLastLoadedScene, &status);
  }

  FabricDFGBaseInterface::allStorePersistenceData(gLastLoadedScene, &status);
}

void onSceneNew(void *userData){
  FabricSpliceEditorWidget::postClearAll();
  FabricSpliceRenderCallback::sDrawContext.invalidate(); 
  FabricDFGCommandStack::getStack()->clear();
  FabricSplice::DestroyClient();
}

void onSceneLoad(void *userData){
  MGlobal::executeCommandOnIdle("unloadPlugin \"FabricSpliceManipulation.py\";");
  MGlobal::executeCommandOnIdle("loadPlugin \"FabricSpliceManipulation.py\";");
  FabricSpliceEditorWidget::postClearAll();
  FabricSpliceRenderCallback::sDrawContext.invalidate(); 

  if(getenv("FABRIC_SPLICE_PROFILING") != NULL)
    FabricSplice::Logging::enableTimers();

  MStatus status = MS::kSuccess;
  gLastLoadedScene = MFileIO::currentFile();

  std::vector<FabricSpliceBaseInterface*> instances = FabricSpliceBaseInterface::getInstances();

  // each node will only restore once, so it's safe for import too
  FabricSplice::Logging::AutoTimer persistenceTimer("Maya::onSceneLoad");
  for(int i = 0; i < instances.size(); ++i){
    FabricSpliceBaseInterface *node = instances[i];
    node->restoreFromPersistenceData(gLastLoadedScene, &status); 
    if( status != MS::kSuccess)
      return;
  }
  FabricSpliceEditorWidget::postClearAll();

  FabricDFGBaseInterface::allRestoreFromPersistenceData(gLastLoadedScene, &status);

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

  FabricDFGBaseInterface::allResetInternalData();

  FabricSplice::DestroyClient(true);
}

bool isDestroyingScene()
{
  return gSceneIsDestroying;
}

void mayaLogFunc(const MString & message)
{
  MGlobal::displayInfo(MString("[Splice] ")+message);
}

void mayaLogFunc(const char * message, unsigned int length)
{
  mayaLogFunc(MString(message));
}

bool gErrorEnabled = true;
void mayaErrorLogEnable(bool enable)
{
  gErrorEnabled = enable;
}

bool gErrorOccured = false;
void mayaLogErrorFunc(const MString & message)
{
  if(!gErrorEnabled)
    return;
  MGlobal::displayError(MString("[Splice] ")+message);
  gErrorOccured = true;
}

void mayaLogErrorFunc(const char * message, unsigned int length)
{
  mayaLogErrorFunc(MString(message));
}

void mayaClearError()
{
  gErrorOccured = false;
}

MStatus mayaErrorOccured()
{
  MStatus result = MS::kSuccess;
  if(gErrorOccured)
    result = MS::kFailure;
  gErrorOccured = false;
  return result;
}

void mayaKLReportFunc(const char * message, unsigned int length)
{
  MGlobal::displayInfo(MString("[KL]: ")+MString(message));
}

void mayaCompilerErrorFunc(unsigned int row, unsigned int col, const char * file, const char * level, const char * desc)
{
  MString line;
  line.set(row);
  MGlobal::displayInfo("[KL Compiler "+MString(level)+"]: line "+line+", op '"+MString(file)+"': "+MString(desc));
  FabricSpliceEditorWidget::reportAllCompilerError(row, col, file, level, desc);
}

void mayaKLStatusFunc(const char * topic, unsigned int topicLength,  const char * message, unsigned int messageLength)
{
  MGlobal::displayInfo(MString("[KL Status]: ")+MString(message));
}

void mayaRefreshFunc()
{
  MGlobal::executeCommandOnIdle("refresh");
}



#if defined(OSMac_)
__attribute__ ((visibility("default")))
#endif
MAYA_EXPORT initializePlugin(MObject obj)
{
  MFnPlugin plugin(obj, getPluginName().asChar(), FabricSplice::GetFabricVersionStr(), "Any");
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
  gOnSceneReferenceCallbackId = MSceneMessage::addCallback(MSceneMessage::kAfterReference, onSceneLoad);
  gOnSceneImportReferenceCallbackId = MSceneMessage::addCallback(MSceneMessage::kAfterImportReference, onSceneLoad);
  gRenderCallback0 = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel0", FabricSpliceRenderCallback::draw);
  gRenderCallback1 = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel1", FabricSpliceRenderCallback::draw);
  gRenderCallback2 = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel2", FabricSpliceRenderCallback::draw);
  gRenderCallback3 = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel3", FabricSpliceRenderCallback::draw);
  gRenderCallback4 = MUiMessage::add3dViewPostRenderMsgCallback("modelPanel4", FabricSpliceRenderCallback::draw);
  gOnNodeAddedCallbackId = MDGMessage::addNodeAddedCallback(FabricSpliceBaseInterface::onNodeAdded);
  gOnNodeRemovedCallbackId = MDGMessage::addNodeRemovedCallback(FabricSpliceBaseInterface::onNodeRemoved);

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

  plugin.registerCommand("dfgAddNode", FabricDFGAddNodeCommand::creator, FabricDFGAddNodeCommand::newSyntax);
  plugin.registerCommand("dfgRemoveNode", FabricDFGRemoveNodeCommand::creator, FabricDFGRemoveNodeCommand::newSyntax);
  plugin.registerCommand("dfgRenameNode", FabricDFGRenameNodeCommand::creator, FabricDFGRenameNodeCommand::newSyntax);
  plugin.registerCommand("dfgAddEmptyFunc", FabricDFGAddEmptyFuncCommand::creator, FabricDFGAddEmptyFuncCommand::newSyntax);
  plugin.registerCommand("dfgAddEmptyGraph", FabricDFGAddEmptyGraphCommand::creator, FabricDFGAddEmptyGraphCommand::newSyntax);
  plugin.registerCommand("dfgAddConnection", FabricDFGAddConnectionCommand::creator, FabricDFGAddConnectionCommand::newSyntax);
  plugin.registerCommand("dfgRemoveConnection", FabricDFGRemoveConnectionCommand::creator, FabricDFGRemoveConnectionCommand::newSyntax);
  plugin.registerCommand("dfgAddPort", FabricDFGAddPortCommand::creator, FabricDFGAddPortCommand::newSyntax);
  plugin.registerCommand("dfgRemovePort", FabricDFGRemovePortCommand::creator, FabricDFGRemovePortCommand::newSyntax);
  plugin.registerCommand("dfgRenamePort", FabricDFGRenamePortCommand::creator, FabricDFGRenamePortCommand::newSyntax);
  plugin.registerCommand("dfgSetArg", FabricDFGSetArgCommand::creator, FabricDFGSetArgCommand::newSyntax);
  plugin.registerCommand("dfgSetDefaultValue", FabricDFGSetDefaultValueCommand::creator, FabricDFGSetDefaultValueCommand::newSyntax);
  plugin.registerCommand("dfgSetCode", FabricDFGSetCodeCommand::creator, FabricDFGSetCodeCommand::newSyntax);
  plugin.registerCommand("dfgGetDesc", FabricDFGGetDescCommand::creator, FabricDFGGetDescCommand::newSyntax);
  plugin.registerCommand("dfgImportJSON", FabricDFGImportJSONCommand::creator, FabricDFGImportJSONCommand::newSyntax);
  plugin.registerCommand("dfgExportJSON", FabricDFGExportJSONCommand::creator, FabricDFGExportJSONCommand::newSyntax);

  initModuleFolder(plugin);

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
  MSceneMessage::removeCallback(gOnSceneReferenceCallbackId);
  MSceneMessage::removeCallback(gOnSceneImportReferenceCallbackId);
  MUiMessage::removeCallback(gRenderCallback0);
  MUiMessage::removeCallback(gRenderCallback1);
  MUiMessage::removeCallback(gRenderCallback2);
  MUiMessage::removeCallback(gRenderCallback3);
  MUiMessage::removeCallback(gRenderCallback4);
  MDGMessage::removeCallback(gOnNodeAddedCallbackId);
  MDGMessage::removeCallback(gOnNodeRemovedCallbackId);

  plugin.deregisterData(FabricSpliceMayaData::id);

  MQtUtil::deregisterUIType("FabricSpliceEditor");

  plugin.deregisterContextCommand("FabricSpliceToolContext", "FabricSpliceToolCommand");

  plugin.deregisterCommand("fabricDFG");
  MQtUtil::deregisterUIType("FabricDFGWidget");
  plugin.deregisterNode(FabricDFGMayaNode::id);

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

  // [pzion 20141201] RM#3318: it seems that sending KL report statements
  // at this point, which might result from destructors called by
  // destroying the Core client, can cause Maya to crash on OS X.
  // 
  FabricSplice::Logging::setKLReportFunc(0);

  FabricSplice::DestroyClient();
  FabricSplice::Finalize();
  return status;
}

MString getPluginName()
{
  return "FabricSpliceMaya";
}

void loadMenu()
{
  MString cmd = "source \"FabricSpliceMenu.mel\"; FabricSpliceLoadMenu(\"";
  cmd += getPluginName();
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

