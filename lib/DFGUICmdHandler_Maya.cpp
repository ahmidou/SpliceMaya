/*
 *  Copyright 2010-2015 Fabric Software Inc. All rights reserved.
 */

#include "DFGUICmdHandler_Maya.h"
#include "FabricDFGBaseInterface.h"

#include <FabricUI/DFG/DFGUICmd/DFGUICmds.h>

#include <maya/MGlobal.h>

#include <sstream>

void DFGUICmdHandler_Maya::encodeBooleanArg(
  FTL::CStrRef name,
  bool value,
  std::stringstream &cmd
  )
{
  cmd << " -";
  cmd << name.c_str();
  cmd << " ";
  cmd << (value? "true": "false");
}

void DFGUICmdHandler_Maya::encodeMELString(
  FTL::CStrRef str,
  std::stringstream &ss
  )
{
  ss << '"';
  for ( FTL::CStrRef::IT it = str.begin(); it != str.end(); ++it )
  {
    switch ( *it )
    {
      case '"':
        ss << "\\\"";
        break;
      
      case '\\':
        ss << "\\\\";
        break;
      
      case '\t':
        ss << "\\t";
        break;
      
      case '\r':
        ss << "\\r";
        break;
      
      case '\n':
        ss << "\\n";
        break;
      
      default:
        ss << *it;
        break;
    }
  }
  ss << '"';
}

void DFGUICmdHandler_Maya::encodeStringArg(
  FTL::CStrRef name,
  FTL::CStrRef value,
  std::stringstream &cmd
  )
{
  cmd << " -";
  cmd << name.c_str();
  cmd << " ";
  encodeMELString( value, cmd );
}

void DFGUICmdHandler_Maya::encodeStringsArg(
  FTL::CStrRef name,
  FTL::ArrayRef<FTL::CStrRef> values,
  std::stringstream &cmd
  )
{
  for ( FTL::ArrayRef<FTL::CStrRef>::IT it = values.begin();
    it != values.end(); ++it )
  {
    FTL::CStrRef value = *it;
    encodeStringArg( name, value, cmd );
  }
}

void DFGUICmdHandler_Maya::encodePositionArg(
  FTL::CStrRef name,
  QPointF value,
  std::stringstream &cmd
  )
{
  cmd << " -";
  cmd << name.c_str();
  cmd << " ";
  cmd << value.x();
  cmd << " ";
  cmd << value.y();
}

void DFGUICmdHandler_Maya::encodePositionsArg(
  FTL::CStrRef name,
  FTL::ArrayRef<QPointF> values,
  std::stringstream &cmd
  )
{
  for ( FTL::ArrayRef<QPointF>::IT it = values.begin();
    it != values.end(); ++it )
  {
    QPointF value = *it;
    encodePositionArg( name, value, cmd );
  }
}

void DFGUICmdHandler_Maya::encodeSizeArg(
  FTL::CStrRef name,
  QSizeF value,
  std::stringstream &cmd
  )
{
  cmd << " -";
  cmd << name.c_str();
  cmd << " ";
  cmd << value.width();
  cmd << " ";
  cmd << value.height();
}

void DFGUICmdHandler_Maya::encodeSizesArg(
  FTL::CStrRef name,
  FTL::ArrayRef<QSizeF> values,
  std::stringstream &cmd
  )
{
  for ( FTL::ArrayRef<QSizeF>::IT it = values.begin();
    it != values.end(); ++it )
  {
    QSizeF value = *it;
    encodeSizeArg( name, value, cmd );
  }
}

void DFGUICmdHandler_Maya::encodeBinding(
  FabricCore::DFGBinding const &binding,
  std::stringstream &cmd
  )
{
  FTL::CStrRef mayaNodeName = getNodeNameFromBinding( binding ).asChar();
  encodeStringArg( FTL_STR("mayaNode"), mayaNodeName.c_str(), cmd );
}

void DFGUICmdHandler_Maya::encodeExec(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  std::stringstream &cmd
  )
{
  encodeBinding( binding, cmd );
  encodeStringArg( FTL_STR("execPath"), execPath, cmd );
}

std::string DFGUICmdHandler_Maya::dfgDoInstPreset(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef preset,
  QPointF pos
  )
{
  std::stringstream cmd;
  cmd << FabricUI::DFG::DFGUICmd_InstPreset::CmdName();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringArg( FTL_STR("presetPath"), preset, cmd );
  encodePositionArg( FTL_STR("xy"), pos, cmd );
  cmd << ';';

  MString result;
  MGlobal::executeCommand(
    cmd.str().c_str(),
    result,
    true, // displayEnabled
    true  // undoEnabled
    );
  return result.asChar();
}

std::string DFGUICmdHandler_Maya::dfgDoAddVar(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef desiredNodeName,
  FTL::CStrRef type,
  FTL::CStrRef extDep,
  QPointF pos
  )
{
  std::stringstream cmd;
  cmd << FabricUI::DFG::DFGUICmd_AddVar::CmdName();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringArg( FTL_STR("desiredNodeName"), desiredNodeName, cmd );
  encodeStringArg( FTL_STR("type"), type, cmd );
  encodeStringArg( FTL_STR("extDep"), extDep, cmd );
  encodePositionArg( FTL_STR("xy"), pos, cmd );
  cmd << ';';

  MString result;
  MGlobal::executeCommand(
    cmd.str().c_str(),
    result,
    true, // displayEnabled
    true  // undoEnabled
    );
  return result.asChar();
}

std::string DFGUICmdHandler_Maya::dfgDoAddGet(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef desiredNodeName,
  FTL::CStrRef varPath,
  QPointF pos
  )
{
  std::stringstream cmd;
  cmd << FabricUI::DFG::DFGUICmd_AddGet::CmdName();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringArg( FTL_STR("desiredNodeName"), desiredNodeName, cmd );
  encodeStringArg( FTL_STR("varPath"), varPath, cmd );
  encodePositionArg( FTL_STR("xy"), pos, cmd );
  cmd << ';';

  MString result;
  MGlobal::executeCommand(
    cmd.str().c_str(),
    result,
    true, // displayEnabled
    true  // undoEnabled
    );
  return result.asChar();
}

std::string DFGUICmdHandler_Maya::dfgDoAddSet(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef desiredNodeName,
  FTL::CStrRef varPath,
  QPointF pos
  )
{
  std::stringstream cmd;
  cmd << FabricUI::DFG::DFGUICmd_AddSet::CmdName();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringArg( FTL_STR("desiredNodeName"), desiredNodeName, cmd );
  encodeStringArg( FTL_STR("varPath"), varPath, cmd );
  encodePositionArg( FTL_STR("xy"), pos, cmd );
  cmd << ';';

  MString result;
  MGlobal::executeCommand(
    cmd.str().c_str(),
    result,
    true, // displayEnabled
    true  // undoEnabled
    );
  return result.asChar();
}

std::string DFGUICmdHandler_Maya::dfgDoAddGraph(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef title,
  QPointF pos
  )
{
  std::stringstream cmd;
  cmd << FabricUI::DFG::DFGUICmd_AddGraph::CmdName();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringArg( FTL_STR("title"), title, cmd );
  encodePositionArg( FTL_STR("xy"), pos, cmd );
  cmd << ';';

  MString result;
  MGlobal::executeCommand(
    cmd.str().c_str(),
    result,
    true, // displayEnabled
    true  // undoEnabled
    );
  return result.asChar();
}

std::string DFGUICmdHandler_Maya::dfgDoAddFunc(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef title,
  FTL::CStrRef code,
  QPointF pos
  )
{
  std::stringstream cmd;
  cmd << FabricUI::DFG::DFGUICmd_AddFunc::CmdName();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringArg( FTL_STR("title"), title, cmd );
  encodeStringArg( FTL_STR("code"), code, cmd );
  encodePositionArg( FTL_STR("xy"), pos, cmd );
  cmd << ';';

  MString result;
  MGlobal::executeCommand(
    cmd.str().c_str(),
    result,
    true, // displayEnabled
    true  // undoEnabled
    );
  return result.asChar();
}

void DFGUICmdHandler_Maya::dfgDoRemoveNodes(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::ArrayRef<FTL::CStrRef> nodes
  )
{
  std::stringstream cmd;
  cmd << FabricUI::DFG::DFGUICmd_RemoveNodes::CmdName();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringsArg( FTL_STR("nodeName"), nodes, cmd );
  cmd << ';';

  MGlobal::executeCommand(
    cmd.str().c_str(),
    true, // displayEnabled
    true  // undoEnabled
    );
}

void DFGUICmdHandler_Maya::dfgDoConnect(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef srcPortPath, 
  FTL::CStrRef dstPortPath
  )
{
  std::stringstream cmd;
  cmd << FabricUI::DFG::DFGUICmd_Connect::CmdName();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringArg( FTL_STR("srcPortPath"), srcPortPath, cmd );
  encodeStringArg( FTL_STR("dstPortPath"), dstPortPath, cmd );
  cmd << ';';

  MGlobal::executeCommand(
    cmd.str().c_str(),
    true, // displayEnabled
    true  // undoEnabled
    );
 }

void DFGUICmdHandler_Maya::dfgDoDisconnect(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef srcPortPath, 
  FTL::CStrRef dstPortPath
  )
{
  std::stringstream cmd;
  cmd << FabricUI::DFG::DFGUICmd_Disconnect::CmdName();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringArg( FTL_STR("srcPortPath"), srcPortPath, cmd );
  encodeStringArg( FTL_STR("dstPortPath"), dstPortPath, cmd );
  cmd << ';';

  MGlobal::executeCommand(
    cmd.str().c_str(),
    true, // displayEnabled
    true  // undoEnabled
    );
}

std::string DFGUICmdHandler_Maya::dfgDoAddPort(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef desiredPortName,
  FabricCore::DFGPortType portType,
  FTL::CStrRef typeSpec,
  FTL::CStrRef connectToPortPath
  )
{
  std::stringstream cmd;
  cmd << FabricUI::DFG::DFGUICmd_AddPort::CmdName();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringArg( FTL_STR("desiredPortName"), desiredPortName, cmd );
  FTL::CStrRef portTypeStr;
  switch ( portType )
  {
    case FabricCore::DFGPortType_In:
      portTypeStr = FTL_STR("In");
      break;
    case FabricCore::DFGPortType_IO:
      portTypeStr = FTL_STR("IO");
      break;
    case FabricCore::DFGPortType_Out:
      portTypeStr = FTL_STR("Out");
      break;
  }
  encodeStringArg( FTL_STR("portType"), portTypeStr, cmd );
  encodeStringArg( FTL_STR("typeSpec"), typeSpec, cmd );
  encodeStringArg( FTL_STR("connectToPortPath"), connectToPortPath, cmd );
  cmd << ';';

  MString mResult;
  MGlobal::executeCommand(
    cmd.str().c_str(),
    mResult,
    true, // displayEnabled
    true  // undoEnabled
    );

  return mResult.asChar();
}

void DFGUICmdHandler_Maya::dfgDoRemovePort(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef portName
  )
{
  std::stringstream cmd;
  cmd << FabricUI::DFG::DFGUICmd_RemovePort::CmdName();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringArg( FTL_STR("portName"), portName, cmd );
  cmd << ';';

  MGlobal::executeCommand(
    cmd.str().c_str(),
    true, // displayEnabled
    true  // undoEnabled
    );
}

void DFGUICmdHandler_Maya::dfgDoMoveNodes(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::ArrayRef<FTL::CStrRef> nodes,
  FTL::ArrayRef<QPointF> poss
  )
{
  std::stringstream cmd;
  cmd << FabricUI::DFG::DFGUICmd_MoveNodes::CmdName();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringsArg( FTL_STR("nodeName"), nodes, cmd );
  encodePositionsArg( FTL_STR("xy"), poss, cmd );
  cmd << ';';

  MGlobal::executeCommand(
    cmd.str().c_str(),
    true, // displayEnabled
    true  // undoEnabled
    );
}

void DFGUICmdHandler_Maya::dfgDoResizeBackDrop(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef name,
  QPointF newTopLeftPos,
  QSizeF newSize
  )
{
  std::stringstream cmd;
  cmd << FabricUI::DFG::DFGUICmd_ResizeBackDrop::CmdName();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringArg( FTL_STR("name"), name, cmd );
  encodePositionArg( FTL_STR("xy"), newTopLeftPos, cmd );
  cmd << " -size ";
  cmd << newSize.width();
  cmd << " ";
  cmd << newSize.height();
  cmd << ';';

  MGlobal::executeCommand(
    cmd.str().c_str(),
    true, // displayEnabled
    true  // undoEnabled
    );
}

std::string DFGUICmdHandler_Maya::dfgDoImplodeNodes(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef desiredName,
  FTL::ArrayRef<FTL::CStrRef> nodes
  )
{
  std::stringstream cmd;
  cmd << FabricUI::DFG::DFGUICmd_ImplodeNodes::CmdName();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringArg( FTL_STR("d"), desiredName, cmd );
  encodeStringsArg( FTL_STR("n"), nodes, cmd );
  cmd << ';';

  MString result;
  MGlobal::executeCommand(
    cmd.str().c_str(),
    result,
    true, // displayEnabled
    true  // undoEnabled
    );
  return result.asChar();
}

std::vector<std::string> DFGUICmdHandler_Maya::dfgDoExplodeNode(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef nodeName
  )
{
  std::stringstream cmd;
  cmd << FabricUI::DFG::DFGUICmd_ExplodeNode::CmdName();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringArg( FTL_STR("n"), nodeName, cmd );
  cmd << ';';

  MStringArray mResult;
  MGlobal::executeCommand(
    cmd.str().c_str(),
    mResult,
    true, // displayEnabled
    true  // undoEnabled
    );

  std::vector<std::string> result;
  result.reserve( mResult.length() );
  for ( unsigned i = 0; i < mResult.length(); ++i )
    result.push_back( mResult[i].asChar() );
  return result;
}

void DFGUICmdHandler_Maya::dfgDoAddBackDrop(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef title,
  QPointF pos
  )
{
  std::stringstream cmd;
  cmd << FabricUI::DFG::DFGUICmd_AddBackDrop::CmdName();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringArg( FTL_STR("title"), title, cmd );
  encodePositionArg( FTL_STR("xy"), pos, cmd );
  cmd << ';';

  MGlobal::executeCommand(
    cmd.str().c_str(),
    true, // displayEnabled
    true  // undoEnabled
    );
}

void DFGUICmdHandler_Maya::dfgDoSetNodeTitle(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef node,
  FTL::CStrRef title
  )
{
  std::stringstream cmd;
  cmd << FabricUI::DFG::DFGUICmd_SetNodeTitle::CmdName();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringArg( FTL_STR("nodeName"), node, cmd );
  encodeStringArg( FTL_STR("title"), title, cmd );
  cmd << ';';

  MGlobal::executeCommand(
    cmd.str().c_str(),
    true, // displayEnabled
    true  // undoEnabled
    );
}

void DFGUICmdHandler_Maya::dfgDoSetNodeComment(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef node,
  FTL::CStrRef comment
  )
{
  std::stringstream cmd;
  cmd << FabricUI::DFG::DFGUICmd_SetNodeComment::CmdName();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringArg( FTL_STR("nodeName"), node, cmd );
  encodeStringArg( FTL_STR("comment"), comment, cmd );
  cmd << ';';

  MGlobal::executeCommand(
    cmd.str().c_str(),
    true, // displayEnabled
    true  // undoEnabled
    );
}

void DFGUICmdHandler_Maya::dfgDoSetCode(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef code
  )
{
  std::stringstream cmd;
  cmd << FabricUI::DFG::DFGUICmd_SetCode::CmdName();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringArg( FTL_STR("code"), code, cmd );
  cmd << ';';

  MGlobal::executeCommand(
    cmd.str().c_str(),
    true, // displayEnabled
    true  // undoEnabled
    );
}

std::string DFGUICmdHandler_Maya::dfgDoRenamePort(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef name,
  FTL::CStrRef desiredName
  )
{
  std::stringstream cmd;
  cmd << FabricUI::DFG::DFGUICmd_RenamePort::CmdName();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringArg( FTL_STR("oldPortName"), name, cmd );
  encodeStringArg( FTL_STR("desiredNewPortName"), desiredName, cmd );
  cmd << ';';

  MString mResult;
  MGlobal::executeCommand(
    cmd.str().c_str(),
    mResult,
    true, // displayEnabled
    true  // undoEnabled
    );

  return mResult.asChar();
}

std::vector<std::string> DFGUICmdHandler_Maya::dfgDoPaste(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef json,
  QPointF cursorPos
  )
{
  std::stringstream cmd;
  cmd << FabricUI::DFG::DFGUICmd_Paste::CmdName();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringArg( FTL_STR("json"), json, cmd );
  encodePositionArg( FTL_STR("xy"), cursorPos, cmd );
  cmd << ';';

  MStringArray mResult;
  MGlobal::executeCommand(
    cmd.str().c_str(),
    mResult,
    true, // displayEnabled
    true  // undoEnabled
    );

  std::vector<std::string> result;
  result.reserve( mResult.length() );
  for ( unsigned i = 0; i < mResult.length(); ++i )
    result.push_back( mResult[i].asChar() );
  return result;
}

void DFGUICmdHandler_Maya::dfgDoSetArgType(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef argName,
  FTL::CStrRef typeName
  )
{
  std::stringstream cmd;
  cmd << FabricUI::DFG::DFGUICmd_SetArgType::CmdName();
  encodeBinding( binding, cmd );
  encodeStringArg( FTL_STR("argument"), argName, cmd );
  encodeStringArg( FTL_STR("type"), typeName, cmd );
  cmd << ';';

  MGlobal::executeCommand(
    cmd.str().c_str(),
    true, // displayEnabled
    true  // undoEnabled
    );
}

void DFGUICmdHandler_Maya::dfgDoSetArgValue(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef name,
  FabricCore::RTVal const &value
  )
{
  std::stringstream cmd;
  cmd << FabricUI::DFG::DFGUICmd_SetArgValue::CmdName();
  encodeBinding( binding, cmd );
  encodeStringArg( FTL_STR("name"), name, cmd );
  FabricCore::RTVal valueJSON = value.getJSON();
  encodeStringArg( FTL_STR("value"), valueJSON.getStringCString(), cmd );
  cmd << ';';

  MGlobal::executeCommand(
    cmd.str().c_str(),
    true, // displayEnabled
    true  // undoEnabled
    );
}

void DFGUICmdHandler_Maya::dfgDoSetPortDefaultValue(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef port,
  FabricCore::RTVal const &value
  )
{
  std::stringstream cmd;
  cmd << FabricUI::DFG::DFGUICmd_SetPortDefaultValue::CmdName();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringArg( FTL_STR("port"), port, cmd );
  FabricCore::RTVal valueJSON = value.getJSON();
  encodeStringArg( FTL_STR("value"), valueJSON.getStringCString(), cmd );
  cmd << ';';

  MGlobal::executeCommand(
    cmd.str().c_str(),
    true, // displayEnabled
    true  // undoEnabled
    );
}

void DFGUICmdHandler_Maya::dfgDoSetRefVarPath(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef node,
  FTL::CStrRef varPath
  )
{
  std::stringstream cmd;
  cmd << FabricUI::DFG::DFGUICmd_SetRefVarPath::CmdName();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringArg( FTL_STR("node"), node, cmd );
  encodeStringArg( FTL_STR("varPath"), varPath, cmd );
  cmd << ';';

  MGlobal::executeCommand(
    cmd.str().c_str(),
    true, // displayEnabled
    true  // undoEnabled
    );
}

MString DFGUICmdHandler_Maya::getNodeNameFromBinding(
  FabricCore::DFGBinding const &binding
  )
{
  MString interfIdStr = binding.getMetadata("maya_id");
  if(interfIdStr.length() == 0)
    return "";
  unsigned int interfId = (unsigned int)interfIdStr.asInt();

  FabricDFGBaseInterface * interf =
    FabricDFGBaseInterface::getInstanceById(interfId);
  if(interf == NULL)
    return "";

  MFnDependencyNode thisNode(interf->getThisMObject());
  return thisNode.name();
}
