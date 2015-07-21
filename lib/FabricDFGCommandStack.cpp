#include "FabricDFGCommandStack.h"
#include "FabricDFGBaseInterface.h"
#include "FabricSpliceHelpers.h"

#include <DFG/DFGUICmd_QUndo/DFGRenameNodeCommand.h>
#include <DFG/DFGUICmd_QUndo/DFGAddPortCommand.h>
#include <DFG/DFGUICmd_QUndo/DFGRemovePortCommand.h>
#include <DFG/DFGUICmd_QUndo/DFGRenamePortCommand.h>
#include <DFG/DFGUICmd_QUndo/DFGSetArgCommand.h>
#include <DFG/DFGUICmd_QUndo/DFGSetCodeCommand.h>
#include <DFG/DFGUICmd_QUndo/DFGSetNodeCacheRuleCommand.h>
#include <DFG/DFGUICmd_QUndo/DFGCopyCommand.h>
#include <DFG/DFGUICmd_QUndo/DFGImplodeNodesCommand.h>
#include <DFG/DFGUICmd_QUndo/DFGExplodeNodeCommand.h>

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
  FabricDFGBaseInterface * interf = NULL;

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
  else if(commandName == "dfgRenameNode")
  {
    addCommandToIgnore(commandName.asChar(), id, undoable);
    if(s_mayaCommandsEnabled)
    {
      DFG::DFGRenameNodeCommand * cmd = (DFG::DFGRenameNodeCommand*)genericCommand;
      interf = getInterfaceFromCommand(cmd);
      MString nodeName = getNodeNameFromCommand(cmd);
      MString path = cmd->getPath();
      MString name = cmd->getTitle();
  
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
      interf = getInterfaceFromCommand(cmd);
      MString nodeName = getNodeNameFromCommand(cmd);
      MString path = cmd->getInstPath();
      MString name = cmd->getTitle();
      QPointF pos = cmd->getPos();
      MString x, y;
      x.set(pos.x());
      y.set(pos.y());
  
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
      interf = getInterfaceFromCommand(cmd);
      MString nodeName = getNodeNameFromCommand(cmd);
      MString path = cmd->getInstPath();
      MString name = cmd->getTitle();
      QPointF pos = cmd->getPos();
      MString x, y;
      x.set(pos.x());
      y.set(pos.y());
  
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
      interf = getInterfaceFromCommand(cmd);
      MString nodeName = getNodeNameFromCommand(cmd);
      MString srcPath = cmd->getSrcPath();
      MString dstPath = cmd->getDstPath();
  
      MString cmdStr = "dfgAddConnection -node \""+nodeName+"\"";
      cmdStr += " -srcPath \""+srcPath+"\"";
      cmdStr += " -dstPath \""+dstPath+"\"";
      MGlobal::executeCommandOnIdle(cmdStr+";", true);
    }
  }
  else if(commandName == "dfgRemoveConnection")
  {
    addCommandToIgnore(commandName.asChar(), id, undoable);
    if(s_mayaCommandsEnabled)
    {
      DFG::DFGRemoveConnectionCommand * cmd = (DFG::DFGRemoveConnectionCommand*)genericCommand;
      interf = getInterfaceFromCommand(cmd);
      MString nodeName = getNodeNameFromCommand(cmd);
      MString srcPath = cmd->getSrcPath();
      MString dstPath = cmd->getDstPath();
  
      MString cmdStr = "dfgRemoveConnection -node \""+nodeName+"\"";
      cmdStr += " -srcPath \""+srcPath+"\"";
      cmdStr += " -dstPath \""+dstPath+"\"";
      MGlobal::executeCommandOnIdle(cmdStr+";", true);
    }
  }
  else if(commandName == "dfgRemoveAllConnections")
  {
    addCommandToIgnore(commandName.asChar(), id, undoable);
    if(s_mayaCommandsEnabled)
    {
      DFG::DFGRemoveAllConnectionsCommand * cmd = (DFG::DFGRemoveAllConnectionsCommand*)genericCommand;
      interf = getInterfaceFromCommand(cmd);
      MString nodeName = getNodeNameFromCommand(cmd);
      MString path = cmd->getPath();
  
      MString cmdStr = "dfgRemoveAllConnections -node \""+nodeName+"\"";
      cmdStr += " -path \""+path+"\"";
      MGlobal::executeCommandOnIdle(cmdStr+";", true);
    }
  }
  else if(commandName == "dfgAddPort")
  {
    addCommandToIgnore(commandName.asChar(), id, undoable);
    if(s_mayaCommandsEnabled)
    {
      DFG::DFGAddPortCommand * cmd = (DFG::DFGAddPortCommand*)genericCommand;
      interf = getInterfaceFromCommand(cmd);
      MString nodeName = getNodeNameFromCommand(cmd);
      MString execPath = cmd->getExecPath();
      MString name = cmd->getPortName();
      MString portType = "In";
      if(cmd->getPortType() == GraphView::PortType_Input)
        portType = "Out";
      else if(cmd->getPortType() == GraphView::PortType_IO)
        portType = "IO";
      MString dataType = cmd->getDataType();
  
      MString cmdStr = "dfgAddPort -node \""+nodeName+"\"";
      cmdStr += " -execPath \""+execPath+"\"";
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
      interf = getInterfaceFromCommand(cmd);
      MString nodeName = getNodeNameFromCommand(cmd);
      MString path = cmd->getExecPath();
      MString name = cmd->getPortName();
  
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
      interf = getInterfaceFromCommand(cmd);
      MString nodeName = getNodeNameFromCommand(cmd);
      MString path = cmd->getPath();
      MString name = cmd->getPortName();
  
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
      interf = getInterfaceFromCommand(cmd);
      MString nodeName = getNodeNameFromCommand(cmd);
      MString path = cmd->getArgName();
      MString dataType = cmd->getDataType();
      std::string json = cmd->getJSON();
  
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
      DFG::DFGCommand * cmd = 0; //(DFG::DFGSetDefaultValueCommand*)genericCommand;
      interf = getInterfaceFromCommand(cmd);
      MString nodeName = getNodeNameFromCommand(cmd);
      MString path = cmd->getPath();
      MString dataType = cmd->getDataType();
      std::string json = cmd->getJSON();
  
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
      interf = getInterfaceFromCommand(cmd);
      MString nodeName = getNodeNameFromCommand(cmd);
      MString path = cmd->getPath();
      std::string code = cmd->getCode();

  
      MString cmdStr = "dfgSetCode -node \""+nodeName+"\"";
      cmdStr += " -path \""+path+"\"";
      cmdStr += " -code \""+MString(escapeString(code).c_str())+"\"";
      MGlobal::executeCommandOnIdle(cmdStr+";", true);
    }
  }
  else if(commandName == "dfgSetNodeCacheRule")
  {
    addCommandToIgnore(commandName.asChar(), id, undoable);
    if(s_mayaCommandsEnabled)
    {
      DFG::DFGSetNodeCacheRuleCommand * cmd = (DFG::DFGSetNodeCacheRuleCommand*)genericCommand;
      interf = getInterfaceFromCommand(cmd);
      MString nodeName = getNodeNameFromCommand(cmd);
      MString path = cmd->getPath();
      MString cacheRule = cmd->getRuleName();
  
      MString cmdStr = "dfgSetNodeCacheRule -node \""+nodeName+"\"";
      cmdStr += " -path \""+path+"\"";
      cmdStr += " -cacheRule \""+cacheRule+"\"";
      MGlobal::executeCommandOnIdle(cmdStr+";", true);
    }
  }
  else if(commandName == "dfgCopy")
  {
    addCommandToIgnore(commandName.asChar(), id, undoable);
    if(s_mayaCommandsEnabled)
    {
      DFG::DFGCopyCommand * cmd = (DFG::DFGCopyCommand*)genericCommand;
      interf = getInterfaceFromCommand(cmd);
      MString nodeName = getNodeNameFromCommand(cmd);
  
      MString cmdStr = "dfgCopy -node \""+nodeName+"\"";
      std::vector<std::string> paths = cmd->getNodePaths();
      cmdStr += " -paths \"";
      for(unsigned int i=0;i<paths.size();i++)
      {
        if(i > 0)
          cmdStr += ",";
        cmdStr += paths[i].c_str();
      }
      cmdStr += "\"";
      // cmdStr += " -cacheRule \""+cacheRule+"\"";
      MGlobal::executeCommandOnIdle(cmdStr+";", true);
    }
  }
  else if(commandName == "dfgPaste")
  {
    addCommandToIgnore(commandName.asChar(), id, undoable);
    if(s_mayaCommandsEnabled)
    {
      DFG::DFGCommand * cmd = 0; //(DFG::DFGPasteCommand*)genericCommand;
      interf = getInterfaceFromCommand(cmd);
      MString nodeName = getNodeNameFromCommand(cmd);
  
      MString cmdStr = "dfgPaste -node \""+nodeName+"\"";
      // cmdStr += " -path \""+path+"\"";
      // cmdStr += " -cacheRule \""+cacheRule+"\"";
      MGlobal::executeCommandOnIdle(cmdStr+";", true);
    }
  }
  else if(commandName == "dfgImplodeNodes")
  {
    addCommandToIgnore(commandName.asChar(), id, undoable);
    if(s_mayaCommandsEnabled)
    {
      DFG::DFGImplodeNodesCommand * cmd = (DFG::DFGImplodeNodesCommand*)genericCommand;
      interf = getInterfaceFromCommand(cmd);
      MString nodeName = getNodeNameFromCommand(cmd);
  
      MString cmdStr = "dfgImplodeNodes -node \""+nodeName+"\"";
      MString name = cmd->getDesiredName();
      std::vector<std::string> paths = cmd->getNodePaths();
      cmdStr += " -name \""+name+"\"";
      cmdStr += " -paths \"";
      for(unsigned int i=0;i<paths.size();i++)
      {
        if(i > 0)
          cmdStr += ",";
        cmdStr += paths[i].c_str();
      }
      cmdStr += "\"";
      // cmdStr += " -cacheRule \""+cacheRule+"\"";
      MGlobal::executeCommandOnIdle(cmdStr+";", true);
    }
  }
  else if(commandName == "dfgExplodeNode")
  {
    addCommandToIgnore(commandName.asChar(), id, undoable);
    if(s_mayaCommandsEnabled)
    {
      DFG::DFGExplodeNodeCommand * cmd = (DFG::DFGExplodeNodeCommand*)genericCommand;
      interf = getInterfaceFromCommand(cmd);
      MString nodeName = getNodeNameFromCommand(cmd);
  
      MString cmdStr = "dfgExplodeNode -node \""+nodeName+"\"";
      MString path = cmd->getNodePath();
      cmdStr += " -path \""+path+"\"";
      MGlobal::executeCommandOnIdle(cmdStr+";", true);
    }
  }
  else if(commandName == "dfgAddVar")
  {
    addCommandToIgnore(commandName.asChar(), id, undoable);
    if(s_mayaCommandsEnabled)
    {
      DFG::DFGAddVarCommand * cmd = (DFG::DFGAddVarCommand*)genericCommand;
      interf = getInterfaceFromCommand(cmd);
      MString nodeName = getNodeNameFromCommand(cmd);
  
      MString cmdStr = "dfgAddVar -node \""+nodeName+"\"";
      MString varName = cmd->getVarName();
      MString dataType = cmd->getDataType();
      MString extDep = cmd->getExtDep();

      cmdStr += " -varName \""+varName+"\"";
      cmdStr += " -dataType \""+dataType+"\"";
      if(extDep.length() > 0)
        cmdStr += " -extension \""+extDep+"\"";
      MGlobal::executeCommandOnIdle(cmdStr+";", true);
    }
  }
  else if(commandName == "dfgAddGet")
  {
    addCommandToIgnore(commandName.asChar(), id, undoable);
    if(s_mayaCommandsEnabled)
    {
      DFG::DFGAddGetCommand * cmd = (DFG::DFGAddGetCommand*)genericCommand;
      interf = getInterfaceFromCommand(cmd);
      MString nodeName = getNodeNameFromCommand(cmd);
  
      MString cmdStr = "dfgAddGet -node \""+nodeName+"\"";
      MString varPath = cmd->getVarPath();
      cmdStr += " -varPath \""+varPath+"\"";

      MGlobal::executeCommandOnIdle(cmdStr+";", true);
    }
  }
  else if(commandName == "dfgAddSet")
  {
    addCommandToIgnore(commandName.asChar(), id, undoable);
    if(s_mayaCommandsEnabled)
    {
      DFG::DFGAddSetCommand * cmd = (DFG::DFGAddSetCommand*)genericCommand;
      interf = getInterfaceFromCommand(cmd);
      MString nodeName = getNodeNameFromCommand(cmd);
  
      MString cmdStr = "dfgAddSet -node \""+nodeName+"\"";
      MString varPath = cmd->getVarPath();
      cmdStr += " -varPath \""+varPath+"\"";

      MGlobal::executeCommandOnIdle(cmdStr+";", true);
    }
  }
  else if(commandName == "dfgSetRefVarPath")
  {
    addCommandToIgnore(commandName.asChar(), id, undoable);
    if(s_mayaCommandsEnabled)
    {
      DFG::DFGSetRefVarPathCommand * cmd = (DFG::DFGSetRefVarPathCommand*)genericCommand;
      interf = getInterfaceFromCommand(cmd);
      MString nodeName = getNodeNameFromCommand(cmd);
  
      MString cmdStr = "dfgSetRefVarPath -node \""+nodeName+"\"";
      MString path = cmd->getPath();
      MString varPath = cmd->getVarPath();
      cmdStr += " -path \""+path+"\"";
      cmdStr += " -varPath \""+varPath+"\"";

      MGlobal::executeCommandOnIdle(cmdStr+";", true);
    }
  }
  else
  {
    mayaLogErrorFunc(MString("FabricDFGCommandStack:: unknown DFG command ")+genericCommand->getName());
    result = false;
  }

  // increment the eval id
  if(interf)
    interf->incrementEvalID();

  return result;
}

FabricDFGCommandStack * FabricDFGCommandStack::getStack()
{
  return &g_stack;
}

FabricDFGBaseInterface * FabricDFGCommandStack::getInterfaceFromCommand(FabricUI::DFG::DFGCommand * command)
{
  FabricCore::DFGBinding binding = ((DFG::DFGController*)command->controller())->getCoreDFGBinding();
  MString interfIdStr = binding.getMetadata("maya_id");
  if(interfIdStr.length() == 0)
    return NULL;
  unsigned int interfId = (unsigned int)interfIdStr.asInt();
  FabricDFGBaseInterface * interf = FabricDFGBaseInterface::getInstanceById(interfId);
  return interf;
}

MString FabricDFGCommandStack::getNodeNameFromCommand(DFG::DFGCommand * command)
{
  FabricDFGBaseInterface * interf = getInterfaceFromCommand(command);
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
