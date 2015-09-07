#include "FabricDFGWidgetCommand.h"
#include "FabricDFGWidget.h"
#include "FabricSpliceHelpers.h"

#include <maya/MStringArray.h>
#include <maya/MSyntax.h>
#include <maya/MArgDatabase.h>
#include <maya/MQtUtil.h>

#include <QtGui/QToolBox>

#define kActionFlag "-a"
#define kActionFlagLong "-action"
#define kNodeFlag "-n"
#define kNodeFlagLong "-node"
#define kJsonFlag "-j"
#define kJsonFlagLong "-json"

MSyntax FabricDFGWidgetCommand::newSyntax()
{
  MSyntax syntax;
  syntax.addFlag(kActionFlag, kActionFlagLong, MSyntax::kString);
  syntax.addFlag(kNodeFlag, kNodeFlagLong, MSyntax::kString);
  syntax.addFlag(kJsonFlag, kJsonFlagLong, MSyntax::kString);
  syntax.enableQuery(false);
  syntax.enableEdit(false);
  return syntax;
}

void* FabricDFGWidgetCommand::creator()
{
  return new FabricDFGWidgetCommand;
}

MStatus FabricDFGWidgetCommand::doIt(const MArgList &args)
{
  MStatus status = MS::kSuccess;
  mayaClearError();

  MArgParser argData(syntax(), args, &status);

  if(!argData.isFlagSet("action"))
  {
    mayaLogErrorFunc("fabricDFG: Action (-a, -action) not provided.");
    return mayaErrorOccured();
  }

  MString action = argData.flagArgumentString("action", 0);

  if(!argData.isFlagSet("node"))
  {
    mayaLogErrorFunc("fabricDFG: Action \""+action+"\" requires the \"node\" argument to be provided.");
    return mayaErrorOccured();
  }
    
  if(action == "showUI")
  {
    // inform the UI of the next node to edit
    FabricDFGWidget::SetCurrentUINodeName(argData.flagArgumentString("node", 0).asChar());

    // use mel to open a floating window
    MString cmd = "source \"FabricDFGUI.mel\"; showDFGWidget(\"";
    cmd += "FabricForMaya";
    cmd += "\");";
    MStatus commandStatus = MGlobal::executeCommandOnIdle(cmd, false);
    if (commandStatus != MStatus::kSuccess)
      MGlobal::displayError("Failed to load FabricSpliceMenu.mel. Adjust your MAYA_SCRIPT_PATH!");
  }
  else
  {
    mayaLogErrorFunc("fabricDFG: Action '" + action + "' not supported.");
    return mayaErrorOccured();
  }

  return mayaErrorOccured();
}

