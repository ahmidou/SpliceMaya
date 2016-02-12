
#include "FabricSpliceEditorCmd.h"
#include "FabricSpliceKLSourceCodeWidget.h"
#include "FabricSpliceEditorWidget.h"
#include "FabricSpliceHelpers.h"

#include <FabricSplice.h>

#include <maya/MGlobal.h>
#include <maya/MArgParser.h>
#include <maya/MArgList.h>

void* FabricSpliceEditorCmd::creator(){
  return new FabricSpliceEditorCmd;
}

MSyntax FabricSpliceEditorCmd::newSyntax()
{
  MSyntax syntax;
  syntax.addFlag("-a", "-action", MSyntax::kString);
  syntax.addFlag("-o", "-operator", MSyntax::kString);
  syntax.addFlag("-n", "-node", MSyntax::kString);
  syntax.enableQuery(true);
  syntax.enableEdit(true);
  return syntax;
}

MStatus FabricSpliceEditorCmd::doIt(const MArgList &args){

  MStatus status = MS::kSuccess;
  mayaClearError();

  MArgParser argData(syntax(), args, &status);
  
  if(!argData.isFlagSet("action"))
  {
    mayaLogErrorFunc("Action (-a, -action) not provided by FabricSpliceEditor command.");
    return mayaErrorOccured();
  }

  MString action = argData.flagArgumentString("action", 0);

  if(action == "createIfNotOpen")
  {
    FabricSpliceEditorWidget::getFirstOrOpen(false);
  }
  else if(action == "setOperator")
  {
    if(!argData.isFlagSet("operator"))
    {
      mayaLogErrorFunc("Action 'setOperator' requires the operator (-o, -operator) flag to be provided by FabricSpliceEditor command.");
      return mayaErrorOccured();
    }
    MString opName = argData.flagArgumentString("operator", 0);
    FabricSpliceEditorWidget::setAllOperatorNames(opName.asChar(), false);
  }
  else if(action == "setNode")
  {
    if(!argData.isFlagSet("node"))
    {
      mayaLogErrorFunc("Action 'setNode' requires the node (-n, -node) flag to be provided by FabricSpliceEditor command.");
      return mayaErrorOccured();
    }
    MString nodeName = argData.flagArgumentString("node", 0);
    FabricSpliceEditorWidget::setAllNodeDropDowns(nodeName.asChar(), false);
  }
  else if(action == "update")
  {
    FabricSpliceEditorWidget::updateAll();
  }
  else if(action == "clear")
  {
    FabricSpliceEditorWidget::clearAll();
  }
  else
  {
    mayaLogErrorFunc("Action '" + action + "' not supported by FabricSpliceEditor command.");
    return mayaErrorOccured();
  }

  return mayaErrorOccured();
}

