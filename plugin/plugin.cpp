
// [andrew 20130620] all hell breaks loose in Linux if these aren't included first
#include <QEvent>
#include <QMetaType>
#include <QTextEdit>
#include <QComboBox>
#include <QDataStream>

#include "plugin.h"
#include <maya/MQtUtil.h>
#include <maya/MGlobal.h>
#include <maya/MStatus.h>
#include <maya/MFileIO.h>
#include <maya/MFnPlugin.h>
#include <maya/MUiMessage.h>
#include <maya/MDGMessage.h>
#include <maya/MAnimMessage.h>
#include <maya/MSceneMessage.h>
#include <maya/MEventMessage.h>
#include <maya/MCommandResult.h>
#include <maya/MViewport2Renderer.h>

#include <FabricSplice.h>
#include "FabricDFGWidget.h"
#include "FabricConstraint.h"
#include "FabricDFGCommands.h"
#include "FabricSpliceHelpers.h"
#include "FabricSpliceCommand.h"
#include "FabricSpliceMayaNode.h"
#include "FabricSpliceMayaData.h"
#include "FabricSpliceEditorCmd.h"
#include "FabricDFGMayaNode_Func.h"
#include "FabricDFGWidgetCommand.h"
#include "FabricDFGMayaNode_Graph.h"
#include "FabricSpliceMayaDeformer.h"
#include "FabricUpgradeAttrCommand.h"
#include "FabricDFGMayaDeformer_Func.h"
#include "FabricExtensionPackageNode.h"
#include "FabricImportPatternCommand.h"
#include "FabricExportPatternCommand.h"
#include "FabricDFGMayaDeformer_Graph.h"

#include "FabricCommand.h"
#include "FabricToolContext.h"
#include "FabricRenderCallback.h"
#include "FabricCommandManagerCallback.h"

#ifdef _MSC_VER
  #define MAYA_EXPORT extern "C" __declspec(dllexport) MStatus _cdecl
#else
  #define MAYA_EXPORT extern "C" MStatus
#endif

// Julien Keep for debugging
#include <stdlib.h>
#include <stdio.h>

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
// FabricConstraint           0x0011AE49  // constraintNode
// FabricDFGMayaFuncNode      0x0011AE4A  // canvasFuncNode
// FabricDFGMayaFuncDeformer  0x0011AE4B  // canvasFuncDeformer
// FabricExtensionPackageNode 0x0011AE4C  // extensionPackageNode
const MTypeId gLastValidNodeID(0x0011AF3F);

MCallbackId gOnSceneNewCallbackId;
MCallbackId gOnSceneLoadCallbackId;
MCallbackId gOnSceneSaveCallbackId;
MCallbackId gOnMayaExitCallbackId;
MCallbackId gOnBeforeImportCallbackId;
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
  FabricCommandManagerCallback::GetManagerCallback()->reset();

  FabricSpliceEditorWidget::postClearAll();
  FabricRenderCallback::sDrawContext.invalidate(); 

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
      handleVal.callMethod("", "onNewScene", 0, NULL);
    }
  }
  FabricCommandManagerCallback::GetManagerCallback()->reset();
}

void onSceneLoad(void *userData){
  FabricCommandManagerCallback::GetManagerCallback()->reset();

  FabricSpliceEditorWidget::postClearAll();
  FabricRenderCallback::sDrawContext.invalidate(); 

  if(getenv("FABRIC_SPLICE_PROFILING") != NULL)
    FabricSplice::Logging::enableTimers();

  MStatus status = MS::kSuccess;
  mayaSetLastLoadedScene(MFileIO::currentFile());

  // visit all existing extension package nodes and check if this has already been packaged.
  // check for all names with the same suffix.
  if(FabricExtensionPackageNode::getNumInstances() > 0)
  {
    // since we need to reload the packages - we need to destoy the client
    // maybe this can be optimized
    FabricSplice::DestroyClient();

    try
    {
      FabricCore::Client client = FabricDFGWidget::GetCoreClient();
      for(unsigned int i=0;i<FabricExtensionPackageNode::getNumInstances();i++)
      {
        FabricExtensionPackageNode * existingPackage = FabricExtensionPackageNode::getInstanceByIndex(i);
        existingPackage->loadPackage(client);
      }
    }
    catch(FabricCore::Exception e)
    {
      mayaLogErrorFunc(e.getDesc_cstr());
    }
  }

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
  
  FabricCommandManagerCallback::GetManagerCallback()->reset();
}

void onBeforeImport(void *userData){
  // [FE-6247]
  // before importing anything we mark all current base interfaces
  // as "restored from pers. data", so that they get skipped when
  // FabricDFGBaseInterface::allRestoreFromPersistenceData() is invoked
  // in the above onSceneLoad() function.
  FabricDFGBaseInterface::setAllRestoredFromPersistenceData(true);
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

#define INITPLUGIN_STATE( status, code ) \
      { \
        MStatus tmp = ( code ); \
        if (status != MStatus::kSuccess) \
          status = tmp; \
      }

#if defined(OSMac_)
__attribute__ ((visibility("default")))
#endif
PLUGIN_EXPORT MStatus initializePlugin(MObject obj)
{
  // Uncomment the following to have stdout and stderr printed into files (else these will not
  // be seen in the Maya console)
  //
  // freopen( "MayaLog.txt", "a", stdout );
  // freopen( "MayaError.txt", "a", stderr );
  // std::cout << "\n\n----- initializePlugin -----\n\n" << std::endl;

  // [FE-6287]
  char const *disable_evalContext = ::getenv( "FABRIC_MAYA_DISABLE_EVALCONTEXT" );
  FabricDFGBaseInterface::s_use_evalContext = !(!!disable_evalContext && !!disable_evalContext[0]);
  if (!FabricDFGBaseInterface::s_use_evalContext)
    MGlobal::displayInfo("[Fabric for Maya]: evalContext has been disabled via the environment variable FABRIC_MAYA_DISABLE_EVALCONTEXT.");

  MFnPlugin plugin(obj, "FabricMaya", FabricSplice::GetFabricVersionStr(), "Any");
  MStatus status = MStatus::kSuccess;

  INITPLUGIN_STATE( status, plugin.registerContextCommand("FabricToolContext", FabricToolContextCmd::creator, "FabricToolCommand", FabricToolCmd::creator) );

  loadMenu();

  // FE-6558 : Don't plug the render-callback if not interactive.
  // Otherwise it will crash on linux machine without DISPLAY
  if (MGlobal::mayaState() == MGlobal::kInteractive)
    FabricRenderCallback::plug();

  gOnSceneSaveCallbackId            = MSceneMessage::addCallback(MSceneMessage::kBeforeSave,           onSceneSave    );
  gOnSceneLoadCallbackId            = MSceneMessage::addCallback(MSceneMessage::kAfterOpen,            onSceneLoad    );
  gOnBeforeSceneOpenCallbackId      = MSceneMessage::addCallback(MSceneMessage::kBeforeOpen,           onSceneNew     );
  gOnSceneNewCallbackId             = MSceneMessage::addCallback(MSceneMessage::kBeforeNew,            onSceneNew     );
  gOnMayaExitCallbackId             = MSceneMessage::addCallback(MSceneMessage::kMayaExiting,          onMayaExiting  );
  gOnSceneExportCallbackId          = MSceneMessage::addCallback(MSceneMessage::kBeforeExport,         onSceneSave    );
  gOnBeforeImportCallbackId         = MSceneMessage::addCallback(MSceneMessage::kBeforeImport,         onBeforeImport );
  gOnSceneImportCallbackId          = MSceneMessage::addCallback(MSceneMessage::kAfterImport,          onSceneLoad    );
  gOnSceneCreateReferenceCallbackId = MSceneMessage::addCallback(MSceneMessage::kAfterCreateReference, onSceneLoad    );
  gOnSceneImportReferenceCallbackId = MSceneMessage::addCallback(MSceneMessage::kAfterImportReference, onSceneLoad    );
  gOnSceneLoadReferenceCallbackId   = MSceneMessage::addCallback(MSceneMessage::kAfterLoadReference,   onSceneLoad    );

  gOnNodeAddedCallbackId            = MDGMessage::addNodeAddedCallback  (FabricSpliceBaseInterface::onNodeAdded   );
  gOnNodeRemovedCallbackId          = MDGMessage::addNodeRemovedCallback(FabricSpliceBaseInterface::onNodeRemoved );

  gOnNodeAddedDFGCallbackId         = MDGMessage  ::addNodeAddedCallback      (FabricDFGBaseInterface::onNodeAdded       );
  gOnNodeRemovedDFGCallbackId       = MDGMessage  ::addNodeRemovedCallback    (FabricDFGBaseInterface::onNodeRemoved     );
  gOnAnimCurveEditedCallbackId      = MAnimMessage::addAnimCurveEditedCallback(FabricDFGBaseInterface::onAnimCurveEdited );
 
  INITPLUGIN_STATE( status, plugin.registerData(FabricSpliceMayaData::typeName, FabricSpliceMayaData::id, FabricSpliceMayaData::creator) );

  INITPLUGIN_STATE( status, plugin.registerCommand("fabricSplice",             FabricSpliceCommand        ::creator) );
  INITPLUGIN_STATE( status, plugin.registerCommand("fabricSpliceEditor",       FabricSpliceEditorCmd      ::creator, FabricSpliceEditorCmd::newSyntax) );
  INITPLUGIN_STATE( status, plugin.registerCommand("fabricSpliceManipulation", FabricManipulationCmd      ::creator) );

  INITPLUGIN_STATE( status, plugin.registerNode("spliceMayaNode",     FabricSpliceMayaNode    ::id, FabricSpliceMayaNode    ::creator, FabricSpliceMayaNode    ::initialize) );
  INITPLUGIN_STATE( status, plugin.registerNode("spliceMayaDeformer", FabricSpliceMayaDeformer::id, FabricSpliceMayaDeformer::creator, FabricSpliceMayaDeformer::initialize, MPxNode::kDeformerNode) );

  MQtUtil::registerUIType("FabricSpliceEditor", FabricSpliceEditorWidget::creator, "fabricSpliceEditor");

  INITPLUGIN_STATE( status, plugin.registerCommand("fabricDFG", FabricDFGWidgetCommand::creator, FabricDFGWidgetCommand::newSyntax) );
  MQtUtil::registerUIType("FabricDFGWidget", FabricDFGWidget::creator, "fabricDFGWidget");

  INITPLUGIN_STATE( status, plugin.registerNode("canvasNode",             FabricDFGMayaNode_Graph    ::id, FabricDFGMayaNode_Graph    ::creator, FabricDFGMayaNode_Graph    ::initialize) );
  INITPLUGIN_STATE( status, plugin.registerNode("canvasDeformer",         FabricDFGMayaDeformer_Graph::id, FabricDFGMayaDeformer_Graph::creator, FabricDFGMayaDeformer_Graph::initialize, MPxNode::kDeformerNode) );
  INITPLUGIN_STATE( status, plugin.registerNode("canvasFuncNode",         FabricDFGMayaNode_Func     ::id, FabricDFGMayaNode_Func     ::creator, FabricDFGMayaNode_Func     ::initialize) );
  INITPLUGIN_STATE( status, plugin.registerNode("canvasFuncDeformer",     FabricDFGMayaDeformer_Func ::id, FabricDFGMayaDeformer_Func ::creator, FabricDFGMayaDeformer_Func ::initialize, MPxNode::kDeformerNode) );
  INITPLUGIN_STATE( status, plugin.registerNode("fabricConstraint",       FabricConstraint           ::id, FabricConstraint           ::creator, FabricConstraint           ::initialize) );
  INITPLUGIN_STATE( status, plugin.registerNode("fabricExtensionPackage", FabricExtensionPackageNode ::id, FabricExtensionPackageNode ::creator, FabricExtensionPackageNode ::initialize) );

  INITPLUGIN_STATE( status, plugin.registerCommand("FabricCanvasGetFabricVersion",  FabricDFGGetFabricVersionCommand  ::creator, FabricDFGGetFabricVersionCommand  ::newSyntax) );
  INITPLUGIN_STATE( status, plugin.registerCommand("FabricCanvasGetContextID",      FabricDFGGetContextIDCommand      ::creator, FabricDFGGetContextIDCommand      ::newSyntax) );
  INITPLUGIN_STATE( status, plugin.registerCommand("FabricCanvasGetBindingID",      FabricDFGGetBindingIDCommand      ::creator, FabricDFGGetBindingIDCommand      ::newSyntax) );
  INITPLUGIN_STATE( status, plugin.registerCommand("FabricCanvasIncrementEvalID",   FabricDFGIncrementEvalIDCommand   ::creator, FabricDFGIncrementEvalIDCommand   ::newSyntax) );
  INITPLUGIN_STATE( status, plugin.registerCommand("FabricCanvasDestroyClient",     FabricDFGDestroyClientCommand     ::creator, FabricDFGDestroyClientCommand     ::newSyntax) );
  INITPLUGIN_STATE( status, plugin.registerCommand("FabricCanvasPackageExtensions", FabricDFGPackageExtensionsCommand ::creator, FabricDFGPackageExtensionsCommand ::newSyntax) );
  INITPLUGIN_STATE( status, plugin.registerCommand("FabricCanvasProcessMelQueue",   FabricCanvasProcessMelQueueCommand::creator, FabricCanvasProcessMelQueueCommand::newSyntax) );

  INITPLUGIN_STATE( status, MAYA_REGISTER_DFGUICMD( plugin, RemoveNodes         ) );
  INITPLUGIN_STATE( status, MAYA_REGISTER_DFGUICMD( plugin, Connect             ) );
  INITPLUGIN_STATE( status, MAYA_REGISTER_DFGUICMD( plugin, Disconnect          ) );
  INITPLUGIN_STATE( status, MAYA_REGISTER_DFGUICMD( plugin, AddGraph            ) );
  INITPLUGIN_STATE( status, MAYA_REGISTER_DFGUICMD( plugin, AddFunc             ) );
  INITPLUGIN_STATE( status, MAYA_REGISTER_DFGUICMD( plugin, InstPreset          ) );
  INITPLUGIN_STATE( status, MAYA_REGISTER_DFGUICMD( plugin, AddVar              ) );
  INITPLUGIN_STATE( status, MAYA_REGISTER_DFGUICMD( plugin, AddGet              ) );
  INITPLUGIN_STATE( status, MAYA_REGISTER_DFGUICMD( plugin, AddSet              ) );
  INITPLUGIN_STATE( status, MAYA_REGISTER_DFGUICMD( plugin, AddPort             ) );
  INITPLUGIN_STATE( status, MAYA_REGISTER_DFGUICMD( plugin, AddInstPort         ) );
  INITPLUGIN_STATE( status, MAYA_REGISTER_DFGUICMD( plugin, AddInstBlockPort    ) );
  INITPLUGIN_STATE( status, MAYA_REGISTER_DFGUICMD( plugin, CreatePreset        ) );
  INITPLUGIN_STATE( status, MAYA_REGISTER_DFGUICMD( plugin, EditPort            ) );
  INITPLUGIN_STATE( status, MAYA_REGISTER_DFGUICMD( plugin, RemovePort          ) );
  INITPLUGIN_STATE( status, MAYA_REGISTER_DFGUICMD( plugin, MoveNodes           ) );
  INITPLUGIN_STATE( status, MAYA_REGISTER_DFGUICMD( plugin, ResizeBackDrop      ) );
  INITPLUGIN_STATE( status, MAYA_REGISTER_DFGUICMD( plugin, ImplodeNodes        ) );
  INITPLUGIN_STATE( status, MAYA_REGISTER_DFGUICMD( plugin, ExplodeNode         ) );
  INITPLUGIN_STATE( status, MAYA_REGISTER_DFGUICMD( plugin, AddBackDrop         ) );
  INITPLUGIN_STATE( status, MAYA_REGISTER_DFGUICMD( plugin, SetNodeComment      ) );
  INITPLUGIN_STATE( status, MAYA_REGISTER_DFGUICMD( plugin, SetCode             ) );
  INITPLUGIN_STATE( status, MAYA_REGISTER_DFGUICMD( plugin, EditNode            ) );
  INITPLUGIN_STATE( status, MAYA_REGISTER_DFGUICMD( plugin, RenamePort          ) );
  INITPLUGIN_STATE( status, MAYA_REGISTER_DFGUICMD( plugin, Paste               ) );
  INITPLUGIN_STATE( status, MAYA_REGISTER_DFGUICMD( plugin, SetArgValue         ) );
  INITPLUGIN_STATE( status, MAYA_REGISTER_DFGUICMD( plugin, SetPortDefaultValue ) );
  INITPLUGIN_STATE( status, MAYA_REGISTER_DFGUICMD( plugin, SetRefVarPath       ) );
  INITPLUGIN_STATE( status, MAYA_REGISTER_DFGUICMD( plugin, ReorderPorts        ) );
  INITPLUGIN_STATE( status, MAYA_REGISTER_DFGUICMD( plugin, SetExtDeps          ) );
  INITPLUGIN_STATE( status, MAYA_REGISTER_DFGUICMD( plugin, SplitFromPreset     ) );
  INITPLUGIN_STATE( status, MAYA_REGISTER_DFGUICMD( plugin, DismissLoadDiags    ) );
  INITPLUGIN_STATE( status, MAYA_REGISTER_DFGUICMD( plugin, AddBlock            ) );
  INITPLUGIN_STATE( status, MAYA_REGISTER_DFGUICMD( plugin, AddBlockPort        ) );
  INITPLUGIN_STATE( status, MAYA_REGISTER_DFGUICMD( plugin, AddNLSPort          ) );
  INITPLUGIN_STATE( status, MAYA_REGISTER_DFGUICMD( plugin, ReorderNLSPorts     ) );

  INITPLUGIN_STATE( status, plugin.registerCommand(
                              "dfgImportJSON",
                              FabricDFGImportJSONCommand::creator,
                              FabricDFGImportJSONCommand::newSyntax
                              ) );
  INITPLUGIN_STATE( status, plugin.registerCommand(
                              "dfgReloadJSON",
                              FabricDFGReloadJSONCommand::creator,
                              FabricDFGReloadJSONCommand::newSyntax
                              ) );
  INITPLUGIN_STATE( status, plugin.registerCommand(
                              "dfgExportJSON",
                              FabricDFGExportJSONCommand::creator,
                              FabricDFGExportJSONCommand::newSyntax
                              ) );
  INITPLUGIN_STATE( status, plugin.registerCommand(
                              "FabricCanvasGetExecuteShared",
                              FabricCanvasGetExecuteSharedCommand::creator,
                              FabricCanvasGetExecuteSharedCommand::newSyntax
                              ) );
  INITPLUGIN_STATE( status, plugin.registerCommand(
                              "FabricCanvasSetExecuteShared",
                              FabricCanvasSetExecuteSharedCommand::creator,
                              FabricCanvasSetExecuteSharedCommand::newSyntax
                              ) );
  INITPLUGIN_STATE( status, plugin.registerCommand(
                              "FabricCanvasReloadExtension",
                              FabricCanvasReloadExtensionCommand::creator,
                              FabricCanvasReloadExtensionCommand::newSyntax
                              ) );

  INITPLUGIN_STATE( status, plugin.registerCommand("fabricUpgradeAttrs",  FabricUpgradeAttrCommand  ::creator, FabricUpgradeAttrCommand  ::newSyntax) );
  INITPLUGIN_STATE( status, plugin.registerCommand("fabricImportPattern", FabricImportPatternCommand::creator, FabricImportPatternCommand::newSyntax) );
  INITPLUGIN_STATE( status, plugin.registerCommand("fabricExportPattern", FabricExportPatternCommand::creator, FabricExportPatternCommand::newSyntax) );

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
  FabricSplice::Logging::setSlowOperationFunc(mayaSlowOpFunc);
  // FabricSplice::SceneManagement::setManipulationFunc(FabricSpliceBaseInterface::manipulationCallback);

  MGlobal::executePythonCommandOnIdle("import AEspliceMayaNodeTemplate", true);
  MGlobal::executePythonCommandOnIdle("import AEcanvasNodeTemplate", true);

  if (MGlobal::mayaState() == MGlobal::kInteractive)
    FabricSplice::SetLicenseType(FabricCore::ClientLicenseType_Interactive);
  else
    FabricSplice::SetLicenseType(FabricCore::ClientLicenseType_Compute);

  INITPLUGIN_STATE(status, plugin.registerCommand("FabricCommand", FabricCommand::creator));
  INITPLUGIN_STATE(status, plugin.registerCommand("FabricDFGDeleteAllPVTools", FabricDFGDeleteAllPVToolsCommand::creator));

  return status;
}

#define UNINITPLUGIN_STATE( status, code ) \
      { \
        MStatus tmp = ( code ); \
        if (status != MStatus::kSuccess) \
          status = tmp; \
      }

#if defined(OSMac_)
__attribute__ ((visibility("default")))
#endif
PLUGIN_EXPORT MStatus uninitializePlugin(MObject obj)
{
  MFnPlugin plugin(obj);
  MStatus status = MStatus::kSuccess;

  unloadMenu();

  UNINITPLUGIN_STATE( status, plugin.deregisterCommand("fabricSplice") );
  UNINITPLUGIN_STATE( status, plugin.deregisterCommand("fabricImportPattern") );
  UNINITPLUGIN_STATE( status, plugin.deregisterCommand("fabricExportPattern") );
  UNINITPLUGIN_STATE( status, plugin.deregisterCommand("fabricUpgradeAttrs") );
  UNINITPLUGIN_STATE( status, plugin.deregisterCommand("FabricSpliceEditor") );
  UNINITPLUGIN_STATE( status, plugin.deregisterCommand("proceedToNextScene") );
  UNINITPLUGIN_STATE( status, plugin.deregisterNode(FabricSpliceMayaNode::id) );
  UNINITPLUGIN_STATE( status, plugin.deregisterNode(FabricSpliceMayaDeformer::id) );

  UNINITPLUGIN_STATE( status, MSceneMessage::removeCallback(gOnSceneSaveCallbackId) );
  UNINITPLUGIN_STATE( status, MSceneMessage::removeCallback(gOnSceneLoadCallbackId) );
  UNINITPLUGIN_STATE( status, MSceneMessage::removeCallback(gOnMayaExitCallbackId) );
  UNINITPLUGIN_STATE( status, MSceneMessage::removeCallback(gOnBeforeImportCallbackId) );
  UNINITPLUGIN_STATE( status, MSceneMessage::removeCallback(gOnSceneImportCallbackId) );
  UNINITPLUGIN_STATE( status, MSceneMessage::removeCallback(gOnSceneExportCallbackId) );
  UNINITPLUGIN_STATE( status, MSceneMessage::removeCallback(gOnSceneCreateReferenceCallbackId) );
  UNINITPLUGIN_STATE( status, MSceneMessage::removeCallback(gOnSceneImportReferenceCallbackId) );
  UNINITPLUGIN_STATE( status, MSceneMessage::removeCallback(gOnSceneLoadReferenceCallbackId) );
  UNINITPLUGIN_STATE( status, MEventMessage::removeCallback(gOnModelPanelSetFocusCallbackId) );

  // FE-6558 : Don't unplug the render-callback if not interactive.
  // Otherwise it will crash on linux machine without DISPLAY
  if (MGlobal::mayaState() == MGlobal::kInteractive)
    FabricRenderCallback::unplug();

  UNINITPLUGIN_STATE( status, MDGMessage::removeCallback(gOnNodeAddedCallbackId) );
  UNINITPLUGIN_STATE( status, MDGMessage::removeCallback(gOnNodeRemovedCallbackId) );

  UNINITPLUGIN_STATE( status, MDGMessage::removeCallback(gOnNodeAddedDFGCallbackId) );
  UNINITPLUGIN_STATE( status, MDGMessage::removeCallback(gOnNodeRemovedDFGCallbackId) );
  UNINITPLUGIN_STATE( status, MDGMessage::removeCallback(gOnAnimCurveEditedCallbackId) );

  UNINITPLUGIN_STATE( status, plugin.deregisterData(FabricSpliceMayaData::id) );

  MQtUtil::deregisterUIType("FabricSpliceEditor");

  UNINITPLUGIN_STATE( status, plugin.deregisterContextCommand("FabricToolContext", "FabricToolCommand") );

  UNINITPLUGIN_STATE( status, plugin.deregisterCommand("fabricDFG") );
  MQtUtil::deregisterUIType("FabricDFGWidget");
  UNINITPLUGIN_STATE( status, plugin.deregisterNode(FabricDFGMayaNode_Graph::id) );
  UNINITPLUGIN_STATE( status, plugin.deregisterNode(FabricDFGMayaDeformer_Graph::id) );
  UNINITPLUGIN_STATE( status, plugin.deregisterNode(FabricDFGMayaNode_Func::id) );
  UNINITPLUGIN_STATE( status, plugin.deregisterNode(FabricDFGMayaDeformer_Func::id) );
  UNINITPLUGIN_STATE( status, plugin.deregisterNode(FabricConstraint::id) );
  UNINITPLUGIN_STATE( status, plugin.deregisterNode(FabricExtensionPackageNode::id) );

  UNINITPLUGIN_STATE( status, plugin.deregisterCommand("FabricCanvasGetFabricVersion") );
  UNINITPLUGIN_STATE( status, plugin.deregisterCommand("FabricCanvasGetContextID") );
  UNINITPLUGIN_STATE( status, plugin.deregisterCommand("FabricCanvasGetBindingID") );
  UNINITPLUGIN_STATE( status, plugin.deregisterCommand("FabricCanvasIncrementEvalID") );
  UNINITPLUGIN_STATE( status, plugin.deregisterCommand("FabricCanvasDestroyClient") );
  UNINITPLUGIN_STATE( status, plugin.deregisterCommand("FabricCanvasPackageExtensions") );
  UNINITPLUGIN_STATE( status, plugin.deregisterCommand("FabricCanvasProcessMelQueue") );

  UNINITPLUGIN_STATE( status, MAYA_DEREGISTER_DFGUICMD( plugin, RemoveNodes         ) );
  UNINITPLUGIN_STATE( status, MAYA_DEREGISTER_DFGUICMD( plugin, Connect             ) );
  UNINITPLUGIN_STATE( status, MAYA_DEREGISTER_DFGUICMD( plugin, Disconnect          ) );
  UNINITPLUGIN_STATE( status, MAYA_DEREGISTER_DFGUICMD( plugin, AddGraph            ) );
  UNINITPLUGIN_STATE( status, MAYA_DEREGISTER_DFGUICMD( plugin, AddFunc             ) );
  UNINITPLUGIN_STATE( status, MAYA_DEREGISTER_DFGUICMD( plugin, InstPreset          ) );
  UNINITPLUGIN_STATE( status, MAYA_DEREGISTER_DFGUICMD( plugin, AddVar              ) );
  UNINITPLUGIN_STATE( status, MAYA_DEREGISTER_DFGUICMD( plugin, AddGet              ) );
  UNINITPLUGIN_STATE( status, MAYA_DEREGISTER_DFGUICMD( plugin, AddSet              ) );
  UNINITPLUGIN_STATE( status, MAYA_DEREGISTER_DFGUICMD( plugin, AddPort             ) );
  UNINITPLUGIN_STATE( status, MAYA_DEREGISTER_DFGUICMD( plugin, AddInstPort         ) );
  UNINITPLUGIN_STATE( status, MAYA_DEREGISTER_DFGUICMD( plugin, AddInstBlockPort    ) );
  UNINITPLUGIN_STATE( status, MAYA_DEREGISTER_DFGUICMD( plugin, CreatePreset        ) );
  UNINITPLUGIN_STATE( status, MAYA_DEREGISTER_DFGUICMD( plugin, EditPort            ) );
  UNINITPLUGIN_STATE( status, MAYA_DEREGISTER_DFGUICMD( plugin, RemovePort          ) );
  UNINITPLUGIN_STATE( status, MAYA_DEREGISTER_DFGUICMD( plugin, MoveNodes           ) );
  UNINITPLUGIN_STATE( status, MAYA_DEREGISTER_DFGUICMD( plugin, ResizeBackDrop      ) );
  UNINITPLUGIN_STATE( status, MAYA_DEREGISTER_DFGUICMD( plugin, ImplodeNodes        ) );
  UNINITPLUGIN_STATE( status, MAYA_DEREGISTER_DFGUICMD( plugin, ExplodeNode         ) );
  UNINITPLUGIN_STATE( status, MAYA_DEREGISTER_DFGUICMD( plugin, AddBackDrop         ) );
  UNINITPLUGIN_STATE( status, MAYA_DEREGISTER_DFGUICMD( plugin, SetNodeComment      ) );
  UNINITPLUGIN_STATE( status, MAYA_DEREGISTER_DFGUICMD( plugin, SetCode             ) );
  UNINITPLUGIN_STATE( status, MAYA_DEREGISTER_DFGUICMD( plugin, EditNode            ) );
  UNINITPLUGIN_STATE( status, MAYA_DEREGISTER_DFGUICMD( plugin, RenamePort          ) );
  UNINITPLUGIN_STATE( status, MAYA_DEREGISTER_DFGUICMD( plugin, Paste               ) );
  UNINITPLUGIN_STATE( status, MAYA_DEREGISTER_DFGUICMD( plugin, SetArgValue         ) );
  UNINITPLUGIN_STATE( status, MAYA_DEREGISTER_DFGUICMD( plugin, SetPortDefaultValue ) );
  UNINITPLUGIN_STATE( status, MAYA_DEREGISTER_DFGUICMD( plugin, SetRefVarPath       ) );
  UNINITPLUGIN_STATE( status, MAYA_DEREGISTER_DFGUICMD( plugin, ReorderPorts        ) );
  UNINITPLUGIN_STATE( status, MAYA_DEREGISTER_DFGUICMD( plugin, SetExtDeps          ) );
  UNINITPLUGIN_STATE( status, MAYA_DEREGISTER_DFGUICMD( plugin, SplitFromPreset     ) );
  UNINITPLUGIN_STATE( status, MAYA_DEREGISTER_DFGUICMD( plugin, DismissLoadDiags    ) );
  UNINITPLUGIN_STATE( status, MAYA_DEREGISTER_DFGUICMD( plugin, AddBlock            ) );
  UNINITPLUGIN_STATE( status, MAYA_DEREGISTER_DFGUICMD( plugin, AddBlockPort        ) );
  UNINITPLUGIN_STATE( status, MAYA_DEREGISTER_DFGUICMD( plugin, AddNLSPort          ) );
  UNINITPLUGIN_STATE( status, MAYA_DEREGISTER_DFGUICMD( plugin, ReorderNLSPorts     ) );

  UNINITPLUGIN_STATE( status, plugin.deregisterCommand( "dfgImportJSON" ) );
  UNINITPLUGIN_STATE( status, plugin.deregisterCommand( "dfgReloadJSON" ) );
  UNINITPLUGIN_STATE( status, plugin.deregisterCommand( "dfgExportJSON" ) );
  UNINITPLUGIN_STATE( status, plugin.deregisterCommand( "FabricCanvasGetExecuteShared" ) );
  UNINITPLUGIN_STATE( status, plugin.deregisterCommand( "FabricCanvasSetExecuteShared" ) );
  UNINITPLUGIN_STATE( status, plugin.deregisterCommand( "FabricCanvasReloadExtension"  ) );

  FabricCommandManagerCallback::GetManagerCallback()->clear();
  UNINITPLUGIN_STATE(status, plugin.deregisterCommand("FabricCommand"));
  UNINITPLUGIN_STATE(status, plugin.deregisterCommand("FabricDFGDeleteAllPVTools"));

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

