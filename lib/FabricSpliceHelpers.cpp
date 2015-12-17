#include "FabricSpliceEditorWidget.h" // [pzion 20150519] Must come first because of some stupid macro definition somewhere
#include "FabricSpliceHelpers.h"
#include <Canvas/DFG/DFGLogWidget.h>
#include <Canvas/DFG/DFGCombinedWidget.h>
#include <Licensing/Licensing.h>
#include <FabricSplice.h>

#include <maya/MGlobal.h>

MString gLastLoadedScene;
MString mayaGetLastLoadedScene()
{
  return gLastLoadedScene;
}

MString gModuleFolder;
void initModuleFolder(MString moduleFolder)
{
  gModuleFolder = moduleFolder;
}

MString getModuleFolder()
{
  return gModuleFolder;
}

void mayaLogFunc(const MString & message)
{
  MGlobal::displayInfo(MString("[Splice] ")+message);
  FabricUI::DFG::DFGLogWidget::log(message.asChar());
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
  FabricUI::DFG::DFGLogWidget::log(message.asChar());
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
  MString composed = "[KL Compiler "+MString(level)+"]: line "+line+", op '"+MString(file)+"': "+MString(desc);
  MGlobal::displayInfo(composed);
  FabricSpliceEditorWidget::reportAllCompilerError(row, col, file, level, desc);
  FabricUI::DFG::DFGLogWidget::log(composed.asChar());
}

void mayaKLStatusFunc(const char * topicData, unsigned int topicLength,  const char * messageData, unsigned int messageLength)
{
  MString composed = MString("[KL Status]: ")+MString(messageData, messageLength);
  MGlobal::displayInfo(composed);

  FTL::StrRef topic( topicData, topicLength );
  FTL::StrRef message( messageData, messageLength );
  FabricCore::Client *client =
    const_cast<FabricCore::Client *>( FabricSplice::DGGraph::getClient() );
  if ( topic == FTL_STR( "licensing" ) )
  {
    try
    {
      if (MGlobal::mayaState() == MGlobal::kInteractive)
      {
        FabricUI_HandleLicenseData(
          NULL,
          *client,
          message,
          false // modalDialogs
          );
      }
    }
    catch ( FabricCore::Exception e )
    {
      FabricUI::DFG::DFGLogWidget::log(e.getDesc_cstr());
    }
  }
  // else
  //   FabricUI::DFG::DFGLogWidget::log(composed.asChar());
}

void mayaRefreshFunc()
{
  MGlobal::executeCommandOnIdle("refresh");
}

void mayaSetLastLoadedScene(MString scene)
{
  gLastLoadedScene = scene;
}
