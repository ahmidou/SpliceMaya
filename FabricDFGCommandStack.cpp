#include "FabricDFGCommandStack.h"
#include "FabricDFGBaseInterface.h"
#include "plugin.h"

#include <DFG/Commands/DFGAddNodeCommand.h>
#include <DFG/Commands/DFGRemoveNodeCommand.h>
#include <DFG/Commands/DFGRenameNodeCommand.h>
#include <DFG/Commands/DFGAddEmptyFuncCommand.h>
#include <DFG/Commands/DFGAddEmptyGraphCommand.h>
#include <DFG/Commands/DFGAddConnectionCommand.h>
#include <DFG/Commands/DFGRemoveConnectionCommand.h>
#include <DFG/Commands/DFGAddPortCommand.h>
#include <DFG/Commands/DFGRemovePortCommand.h>
#include <DFG/Commands/DFGRenamePortCommand.h>
#include <DFG/Commands/DFGSetArgCommand.h>
#include <DFG/Commands/DFGSetDefaultValueCommand.h>
#include <DFG/Commands/DFGSetCodeCommand.h>

using namespace FabricServices;
using namespace FabricUI;

bool FabricDFGCommandStack::s_mayaCommandsEnabled = true;
std::vector<FabricDFGCommandStack::Info> FabricDFGCommandStack::s_pendingCommandsToIgnore;

FabricDFGCommandStack g_stack;

bool FabricDFGCommandStack::add(Commands::Command * command)
{
  if(!Commands::CommandStack::add(command))
    return false;
  logMayaCommand(command, command->getID());
  return true;
}

void FabricDFGCommandStack::addCommandToIgnore(const std::string & name, unsigned id, bool undoable)
{
  s_pendingCommandsToIgnore.insert(s_pendingCommandsToIgnore.begin(), Info(name, id, undoable));
}

FabricDFGCommandStack::Info FabricDFGCommandStack::consumeCommandToIgnore(const std::string & name)
{
  if(s_pendingCommandsToIgnore.size() > 0)
  {
    Info result;
    while(result.name != name)
    {
      if(s_pendingCommandsToIgnore.size() == 0)
        return Info();
      
      result = s_pendingCommandsToIgnore[s_pendingCommandsToIgnore.size()-1];
      s_pendingCommandsToIgnore.pop_back();
    }
    return result;
  }
  return Info();
}

void FabricDFGCommandStack::enableMayaCommands(bool state)
{
  s_mayaCommandsEnabled = state;
}

bool FabricDFGCommandStack::logMayaCommand(FabricServices::Commands::Command * genericCommand, unsigned int id, bool undoable, FabricServices::Commands::Command * lastCommand)
{
  bool result = true;
  MString commandName = genericCommand->getName();

  // in case of a compound, undoable only if 
  // this is the last command within a compound
  if(undoable && lastCommand)
    undoable = genericCommand == lastCommand;

  if(commandName == "Compound")
  {
    Commands::CompoundCommand * cmd = (Commands::CompoundCommand*)genericCommand;
    Commands::Command * lastCommand = cmd->getLastNonCompoundCommand();
    for(unsigned int i=0;i<cmd->getNbCommands();i++)
      result = result && logMayaCommand(cmd->getCommand(i), id, undoable, lastCommand);
  }
  else if(commandName == "dfgAddNode")
  {
    addCommandToIgnore(commandName.asChar(), id, undoable);
    if(s_mayaCommandsEnabled)
    {
      DFG::DFGAddNodeCommand * cmd = (DFG::DFGAddNodeCommand*)genericCommand;
      MString nodeName = getNodeNameFromCommand(cmd);
      MString path = cmd->getPath().c_str();
      MString preset = cmd->getPreset().c_str();
      QPointF pos = cmd->getPos();
      MString x, y;
      x.set(pos.x());
      y.set(pos.y());
      DFG::DFGController * controller = (DFG::DFGController *)cmd->controller();
  
      MString cmdStr = "dfgAddNode -node \""+nodeName+"\"";
      cmdStr += " -path \""+path+"\"";
      cmdStr += " -preset \""+preset+"\"";
      cmdStr += " -xpos "+x;
      cmdStr += " -ypos "+y;
      MGlobal::executeCommandOnIdle(cmdStr+";", true);
    }
  }
  else if(commandName == "dfgRemoveNode")
  {
    addCommandToIgnore(commandName.asChar(), id, undoable);
    if(s_mayaCommandsEnabled)
    {
      DFG::DFGRemoveNodeCommand * cmd = (DFG::DFGRemoveNodeCommand*)genericCommand;
      MString nodeName = getNodeNameFromCommand(cmd);
      MString path = cmd->getNodePath().toUtf8().constData();
      DFG::DFGController * controller = (DFG::DFGController *)cmd->controller();
  
      MString cmdStr = "dfgRemoveNode -node \""+nodeName+"\"";
      cmdStr += " -path \""+path+"\"";
      MGlobal::executeCommandOnIdle(cmdStr+";", true);
    }
  }
  else if(commandName == "dfgRenameNode")
  {
    addCommandToIgnore(commandName.asChar(), id, undoable);
    if(s_mayaCommandsEnabled)
    {
      DFG::DFGRenameNodeCommand * cmd = (DFG::DFGRenameNodeCommand*)genericCommand;
      MString nodeName = getNodeNameFromCommand(cmd);
      MString path = cmd->getPath().toUtf8().constData();
      MString name = cmd->getTitle().toUtf8().constData();
      DFG::DFGController * controller = (DFG::DFGController *)cmd->controller();
  
      MString cmdStr = "dfgRenameNode -node \""+nodeName+"\"";
      cmdStr += " -path \""+path+"\"";
      cmdStr += " -name \""+name+"\"";
      MGlobal::executeCommandOnIdle(cmdStr+";", true);
    }
  }
  else if(commandName == "dfgAddEmptyFunc")
  {
    addCommandToIgnore(commandName.asChar(), id, undoable);
    if(s_mayaCommandsEnabled)
    {
      DFG::DFGAddEmptyFuncCommand * cmd = (DFG::DFGAddEmptyFuncCommand*)genericCommand;
      MString nodeName = getNodeNameFromCommand(cmd);
      MString path = cmd->getPath().c_str();
      MString name = cmd->getTitle().c_str();
      QPointF pos = cmd->getPos();
      MString x, y;
      x.set(pos.x());
      y.set(pos.y());
      DFG::DFGController * controller = (DFG::DFGController *)cmd->controller();
  
      MString cmdStr = "dfgAddEmptyFunc -node \""+nodeName+"\"";
      cmdStr += " -path \""+path+"\"";
      cmdStr += " -name \""+name+"\"";
      cmdStr += " -xpos "+x;
      cmdStr += " -ypos "+y;
      MGlobal::executeCommandOnIdle(cmdStr+";", true);
    }
  }
  else if(commandName == "dfgAddEmptyGraph")
  {
    addCommandToIgnore(commandName.asChar(), id, undoable);
    if(s_mayaCommandsEnabled)
    {
      DFG::DFGAddEmptyGraphCommand * cmd = (DFG::DFGAddEmptyGraphCommand*)genericCommand;
      MString nodeName = getNodeNameFromCommand(cmd);
      MString path = cmd->getPath().c_str();
      MString name = cmd->getTitle().c_str();
      QPointF pos = cmd->getPos();
      MString x, y;
      x.set(pos.x());
      y.set(pos.y());
      DFG::DFGController * controller = (DFG::DFGController *)cmd->controller();
  
      MString cmdStr = "dfgAddEmptyGraph -node \""+nodeName+"\"";
      cmdStr += " -path \""+path+"\"";
      cmdStr += " -name \""+name+"\"";
      cmdStr += " -xpos "+x;
      cmdStr += " -ypos "+y;
      MGlobal::executeCommandOnIdle(cmdStr+";", true);
    }
  }
  else if(commandName == "dfgAddConnection")
  {
    addCommandToIgnore(commandName.asChar(), id, undoable);
    if(s_mayaCommandsEnabled)
    {
      DFG::DFGAddConnectionCommand * cmd = (DFG::DFGAddConnectionCommand*)genericCommand;
      MString nodeName = getNodeNameFromCommand(cmd);
      MString srcPath = cmd->getSrcPath();
      MString dstPath = cmd->getDstPath();
      DFG::DFGController * controller = (DFG::DFGController *)cmd->controller();
  
      MString cmdStr = "dfgAddConnection -node \""+nodeName+"\"";
      cmdStr += " -srcPath \""+srcPath+"\"";
      cmdStr += " -dstPath \""+dstPath+"\"";
      if(!cmd->srcIsPin() && !cmd->dstIsPin())
      {
        int srcIsPin = cmd->srcIsPin() ? 1 : 0;
        int dstIsPin = cmd->dstIsPin() ? 1 : 0;
        MString srcIsPinStr, dstIsPinStr;
        srcIsPinStr.set(srcIsPin);
        dstIsPinStr.set(dstIsPin);
        cmdStr += " -srcIsPin "+srcIsPinStr;
        cmdStr += " -dstIsPin "+dstIsPinStr;
      }
      MGlobal::executeCommandOnIdle(cmdStr+";", true);
    }
  }
  else if(commandName == "dfgRemoveConnection")
  {
    addCommandToIgnore(commandName.asChar(), id, undoable);
    if(s_mayaCommandsEnabled)
    {
      DFG::DFGRemoveConnectionCommand * cmd = (DFG::DFGRemoveConnectionCommand*)genericCommand;
      MString nodeName = getNodeNameFromCommand(cmd);
      MString srcPath = cmd->getSrcPath();
      MString dstPath = cmd->getDstPath();
      DFG::DFGController * controller = (DFG::DFGController *)cmd->controller();
  
      MString cmdStr = "dfgRemoveConnection -node \""+nodeName+"\"";
      cmdStr += " -srcPath \""+srcPath+"\"";
      cmdStr += " -dstPath \""+dstPath+"\"";
      if(!cmd->srcIsPin() && !cmd->dstIsPin())
      {
        int srcIsPin = cmd->srcIsPin() ? 1 : 0;
        int dstIsPin = cmd->dstIsPin() ? 1 : 0;
        MString srcIsPinStr, dstIsPinStr;
        srcIsPinStr.set(srcIsPin);
        dstIsPinStr.set(dstIsPin);
        cmdStr += " -srcIsPin "+srcIsPinStr;
        cmdStr += " -dstIsPin "+dstIsPinStr;
      }
      MGlobal::executeCommandOnIdle(cmdStr+";", true);
    }
  }
  else if(commandName == "dfgAddPort")
  {
    addCommandToIgnore(commandName.asChar(), id, undoable);
    if(s_mayaCommandsEnabled)
    {
      DFG::DFGAddPortCommand * cmd = (DFG::DFGAddPortCommand*)genericCommand;
      MString nodeName = getNodeNameFromCommand(cmd);
      MString path = cmd->getExecPath();
      MString name = cmd->getPortName();
      MString portType = "In";
      if(cmd->getPortType() == GraphView::PortType_Input)
        portType = "Out";
      else if(cmd->getPortType() == GraphView::PortType_IO)
        portType = "IO";
      MString dataType = cmd->getDataType();
      DFG::DFGController * controller = (DFG::DFGController *)cmd->controller();
  
      MString cmdStr = "dfgAddPort -node \""+nodeName+"\"";
      cmdStr += " -path \""+path+"\"";
      cmdStr += " -name \""+name+"\"";
      cmdStr += " -portType \""+portType+"\"";
      cmdStr += " -dataType \""+dataType+"\"";
      MGlobal::executeCommandOnIdle(cmdStr+";", true);
    }
  }
  else if(commandName == "dfgRemovePort")
  {
    addCommandToIgnore(commandName.asChar(), id, undoable);
    if(s_mayaCommandsEnabled)
    {
      DFG::DFGRemovePortCommand * cmd = (DFG::DFGRemovePortCommand*)genericCommand;
      MString nodeName = getNodeNameFromCommand(cmd);
      MString path = cmd->getExecPath();
      MString name = cmd->getPortName();
      DFG::DFGController * controller = (DFG::DFGController *)cmd->controller();
  
      MString cmdStr = "dfgRemovePort -node \""+nodeName+"\"";
      cmdStr += " -path \""+path+"\"";
      cmdStr += " -name \""+name+"\"";
      MGlobal::executeCommandOnIdle(cmdStr+";", true);
    }
  }
  else if(commandName == "dfgRenamePort")
  {
    addCommandToIgnore(commandName.asChar(), id, undoable);
    if(s_mayaCommandsEnabled)
    {
      DFG::DFGRenamePortCommand * cmd = (DFG::DFGRenamePortCommand*)genericCommand;
      MString nodeName = getNodeNameFromCommand(cmd);
      MString path = cmd->getPath();
      MString name = cmd->getPortName();
      DFG::DFGController * controller = (DFG::DFGController *)cmd->controller();
  
      MString cmdStr = "dfgRenamePort -node \""+nodeName+"\"";
      cmdStr += " -path \""+path+"\"";
      cmdStr += " -name \""+name+"\"";
      MGlobal::executeCommandOnIdle(cmdStr+";", true);
    }
  }
  else if(commandName == "dfgSetArg")
  {
    addCommandToIgnore(commandName.asChar(), id, undoable);
    if(s_mayaCommandsEnabled)
    {
      DFG::DFGSetArgCommand * cmd = (DFG::DFGSetArgCommand*)genericCommand;
      MString nodeName = getNodeNameFromCommand(cmd);
      MString path = cmd->getArgName();
      MString dataType = cmd->getDataType();
      std::string json = cmd->getJSON();
      DFG::DFGController * controller = (DFG::DFGController *)cmd->controller();
  
      MString cmdStr = "dfgSetArg -node \""+nodeName+"\"";
      cmdStr += " -path \""+path+"\"";
      cmdStr += " -dataType \""+dataType+"\"";
      cmdStr += " -json \""+MString(escapeString(json).c_str())+"\"";
      MGlobal::executeCommandOnIdle(cmdStr+";", true);
    }
  }
  else if(commandName == "dfgSetDefaultValue")
  {
    addCommandToIgnore(commandName.asChar(), id, undoable);
    if(s_mayaCommandsEnabled)
    {
      DFG::DFGSetDefaultValueCommand * cmd = (DFG::DFGSetDefaultValueCommand*)genericCommand;
      MString nodeName = getNodeNameFromCommand(cmd);
      MString path = cmd->getPath();
      MString dataType = cmd->getDataType();
      std::string json = cmd->getJSON();
      DFG::DFGController * controller = (DFG::DFGController *)cmd->controller();
  
      MString cmdStr = "dfgSetDefaultValue -node \""+nodeName+"\"";
      cmdStr += " -path \""+path+"\"";
      cmdStr += " -dataType \""+dataType+"\"";
      cmdStr += " -json \""+MString(escapeString(json).c_str())+"\"";
      MGlobal::executeCommandOnIdle(cmdStr+";", true);
    }
  }
  else if(commandName == "dfgSetCode")
  {
    addCommandToIgnore(commandName.asChar(), id, undoable);
    if(s_mayaCommandsEnabled)
    {
      DFG::DFGSetCodeCommand * cmd = (DFG::DFGSetCodeCommand*)genericCommand;
      MString nodeName = getNodeNameFromCommand(cmd);
      MString path = cmd->getPath().c_str();
      std::string code = cmd->getCode();

      DFG::DFGController * controller = (DFG::DFGController *)cmd->controller();
  
      MString cmdStr = "dfgSetCode -node \""+nodeName+"\"";
      cmdStr += " -path \""+path+"\"";
      cmdStr += " -code \""+MString(escapeString(code).c_str())+"\"";
      MGlobal::executeCommandOnIdle(cmdStr+";", true);
    }
  }
  else
  {
    mayaLogErrorFunc(MString("FabricDFGCommandStack:: unknown DFG command ")+genericCommand->getName());
    result = false;
  }
  return result;
}

FabricDFGCommandStack * FabricDFGCommandStack::getStack()
{
  return &g_stack;
}

MString FabricDFGCommandStack::getNodeNameFromCommand(DFG::DFGCommand * command)
{
  MString interfIdStr = ((DFG::DFGController*)command->controller())->getBinding().getGraph().getMetadata("maya_id").c_str();
  if(interfIdStr.length() == 0)
    return "";
  unsigned int interfId = (unsigned int)interfIdStr.asInt();
  FabricDFGBaseInterface * interf = FabricDFGBaseInterface::getInstanceById(interfId);
  if(interf == NULL)
    return "";
  MFnDependencyNode thisNode(interf->getThisMObject());
  return thisNode.name();
}

std::string FabricDFGCommandStack::escapeString(const std::string & unescaped)
{
  std::string escaped;
  for(size_t i=0;i<unescaped.length();i++)
  {
    switch(unescaped[i])
    {
      case '"':
      {
        escaped += "\\\"";
        break;
      }
      case '\\':
      {
        escaped += "\\\\";
        break;
      }
      case '\t':
      {
        escaped += "\\t";
        break;
      }
      case '\r':
      {
        escaped += "\\r";
        break;
      }
      case '\n':
      {
        escaped += "\\n";
        break;
      }
      default:
        escaped += unescaped[i];
    }
  }
  return escaped;
}
