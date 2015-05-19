#include "FabricSpliceHelpers.h"
#include "FabricSpliceEditorWidget.h"

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

void mayaSetLastLoadedScene(MString scene)
{
  gLastLoadedScene = scene;
}
