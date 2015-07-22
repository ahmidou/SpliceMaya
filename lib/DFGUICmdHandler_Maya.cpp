/*
 *  Copyright 2010-2015 Fabric Software Inc. All rights reserved.
 */

#include "DFGUICmdHandler_Maya.h"
#include "FabricDFGBaseInterface.h"

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
  encodeStringArg(
    FTL_STR("mayaNode"),
    getNodeNameFromBinding( binding ).asChar(),
    cmd
    );
}

void DFGUICmdHandler_Maya::encodeExec(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  std::stringstream &cmd
  )
{
  encodeBinding( binding, cmd );
  encodeStringArg(
    FTL_STR("exec"),
    execPath,
    cmd
    );
}

std::string DFGUICmdHandler_Maya::dfgDoAddInstFromPreset(
  FTL::CStrRef desc,
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef preset,
  QPointF pos
  )
{
  std::stringstream cmd;
  cmd << CmdName_AddInstFromPreset();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringArg( FTL_STR("preset"), preset, cmd );
  encodePositionArg( FTL_STR("position"), pos, cmd );
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
  FTL::CStrRef desc,
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef desiredName,
  FTL::CStrRef dataType,
  FTL::CStrRef extDep,
  QPointF pos
  )
{
  std::stringstream cmd;
  cmd << CmdName_AddVar();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringArg( FTL_STR("desiredName"), desiredName, cmd );
  encodeStringArg( FTL_STR("dataType"), dataType, cmd );
  encodeStringArg( FTL_STR("extDep"), extDep, cmd );
  encodePositionArg( FTL_STR("position"), pos, cmd );
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
  FTL::CStrRef desc,
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef desiredName,
  FTL::CStrRef varPath,
  QPointF pos
  )
{
  std::stringstream cmd;
  cmd << CmdName_AddGet();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringArg( FTL_STR("desiredName"), desiredName, cmd );
  encodeStringArg( FTL_STR("varPath"), varPath, cmd );
  encodePositionArg( FTL_STR("position"), pos, cmd );
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
  FTL::CStrRef desc,
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef desiredName,
  FTL::CStrRef varPath,
  QPointF pos
  )
{
  std::stringstream cmd;
  cmd << CmdName_AddSet();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringArg( FTL_STR("desiredName"), desiredName, cmd );
  encodeStringArg( FTL_STR("varPath"), varPath, cmd );
  encodePositionArg( FTL_STR("position"), pos, cmd );
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

std::string DFGUICmdHandler_Maya::dfgDoAddInstWithEmptyGraph(
  FTL::CStrRef desc,
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef title,
  QPointF pos
  )
{
  std::stringstream cmd;
  cmd << CmdName_AddInstWithEmptyGraph();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringArg( FTL_STR("title"), title, cmd );
  encodePositionArg( FTL_STR("position"), pos, cmd );
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

std::string DFGUICmdHandler_Maya::dfgDoAddInstWithEmptyFunc(
  FTL::CStrRef desc,
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef title,
  FTL::CStrRef code,
  QPointF pos
  )
{
  std::stringstream cmd;
  cmd << CmdName_AddInstWithEmptyFunc();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringArg( FTL_STR("title"), title, cmd );
  encodeStringArg( FTL_STR("code"), code, cmd );
  encodePositionArg( FTL_STR("position"), pos, cmd );
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
  FTL::CStrRef desc,
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::ArrayRef<FTL::CStrRef> nodes
  )
{
  std::stringstream cmd;
  cmd << CmdName_RemoveNodes();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringsArg( FTL_STR("node"), nodes, cmd );
  cmd << ';';

  MGlobal::executeCommand(
    cmd.str().c_str(),
    true, // displayEnabled
    true  // undoEnabled
    );
}

void DFGUICmdHandler_Maya::dfgDoConnect(
  FTL::CStrRef desc,
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef srcPort, 
  FTL::CStrRef dstPort
  )
{
  std::stringstream cmd;
  cmd << CmdName_Connect();
  encodeExec( binding, execPath, exec, cmd );
  cmd << " -srcPort ";
  encodeMELString( srcPort, cmd );
  cmd << " -dstPort ";
  encodeMELString( dstPort, cmd );
  cmd << ';';

  MGlobal::executeCommand(
    cmd.str().c_str(),
    true, // displayEnabled
    true  // undoEnabled
    );
 }

void DFGUICmdHandler_Maya::dfgDoDisconnect(
  FTL::CStrRef desc,
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef srcPort, 
  FTL::CStrRef dstPort
  )
{
  std::stringstream cmd;
  cmd << CmdName_Disconnect();
  encodeExec( binding, execPath, exec, cmd );
  cmd << " -srcPort ";
  encodeMELString( srcPort, cmd );
  cmd << " -dstPort ";
  encodeMELString( dstPort, cmd );
  cmd << ';';

  MGlobal::executeCommand(
    cmd.str().c_str(),
    true, // displayEnabled
    true  // undoEnabled
    );
}

std::string DFGUICmdHandler_Maya::dfgDoAddPort(
  FTL::CStrRef desc,
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef desiredPortName,
  FabricCore::DFGPortType portType,
  FTL::CStrRef typeSpec,
  FTL::CStrRef portToConnect
  )
{
  std::stringstream cmd;
  cmd << CmdName_AddPort();
  encodeExec( binding, execPath, exec, cmd );
  cmd << " -desiredPortName ";
  encodeMELString( desiredPortName, cmd );
  cmd << " -portType ";
  switch ( portType )
  {
    case FabricCore::DFGPortType_In:
      encodeMELString( "In", cmd );
      break;
    case FabricCore::DFGPortType_IO:
      encodeMELString( "IO", cmd );
      break;
    case FabricCore::DFGPortType_Out:
      encodeMELString( "Out", cmd );
      break;
  }
  cmd << " -typeSpec ";
  encodeMELString( typeSpec, cmd );
  if ( !portToConnect.empty() )
  {
    cmd << " -portToConnect ";
    encodeMELString( portToConnect, cmd );
  }
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
  FTL::CStrRef desc,
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef portName
  )
{
  std::stringstream cmd;
  cmd << CmdName_RemovePort();
  encodeExec( binding, execPath, exec, cmd );
  cmd << " -port ";
  encodeMELString( portName, cmd );
  cmd << ';';

  MGlobal::executeCommand(
    cmd.str().c_str(),
    true, // displayEnabled
    true  // undoEnabled
    );
}

void DFGUICmdHandler_Maya::dfgDoMoveNodes(
  FTL::CStrRef desc,
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::ArrayRef<FTL::CStrRef> nodes,
  FTL::ArrayRef<QPointF> poss
  )
{
  std::stringstream cmd;
  cmd << CmdName_MoveNodes();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringsArg( FTL_STR("node"), nodes, cmd );
  encodePositionsArg( FTL_STR("position"), poss, cmd );
  cmd << ';';

  MGlobal::executeCommand(
    cmd.str().c_str(),
    true, // displayEnabled
    true  // undoEnabled
    );
}

void DFGUICmdHandler_Maya::dfgDoResizeBackDropNode(
  FTL::CStrRef desc,
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef name,
  QPointF newTopLeftPos,
  QSizeF newSize
  )
{
  std::stringstream cmd;
  cmd << CmdName_ResizeBackDropNode();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringArg( FTL_STR("name"), name, cmd );
  encodePositionArg( FTL_STR("position"), newTopLeftPos, cmd );
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
  FTL::CStrRef desc,
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef desiredName,
  FTL::ArrayRef<FTL::CStrRef> nodes
  )
{
  std::stringstream cmd;
  cmd << CmdName_ImplodeNodes();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringArg( FTL_STR("desiredName"), desiredName, cmd );
  encodeStringsArg( FTL_STR("node"), nodes, cmd );
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
  FTL::CStrRef desc,
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef name
  )
{
  std::stringstream cmd;
  cmd << CmdName_ExplodeNode();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringArg( FTL_STR("name"), name, cmd );
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
  FTL::CStrRef desc,
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef title,
  QPointF pos
  )
{
  std::stringstream cmd;
  cmd << CmdName_AddBackDrop();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringArg( FTL_STR("title"), title, cmd );
  encodePositionArg( FTL_STR("position"), pos, cmd );
  cmd << ';';

  MGlobal::executeCommand(
    cmd.str().c_str(),
    true, // displayEnabled
    true  // undoEnabled
    );
}

void DFGUICmdHandler_Maya::dfgDoSetNodeTitle(
  FTL::CStrRef desc,
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef node,
  FTL::CStrRef title
  )
{
  std::stringstream cmd;
  cmd << CmdName_SetNodeTitle();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringArg( FTL_STR("node"), node, cmd );
  encodeStringArg( FTL_STR("title"), title, cmd );
  cmd << ';';

  MGlobal::executeCommand(
    cmd.str().c_str(),
    true, // displayEnabled
    true  // undoEnabled
    );
}

void DFGUICmdHandler_Maya::dfgDoSetNodeComment(
  FTL::CStrRef desc,
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef node,
  FTL::CStrRef comment
  )
{
  std::stringstream cmd;
  cmd << CmdName_SetNodeComment();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringArg( FTL_STR("node"), node, cmd );
  encodeStringArg( FTL_STR("comment"), comment, cmd );
  cmd << ';';

  MGlobal::executeCommand(
    cmd.str().c_str(),
    true, // displayEnabled
    true  // undoEnabled
    );
}

void DFGUICmdHandler_Maya::dfgDoSetNodeCommentExpanded(
  FTL::CStrRef desc,
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef node,
  bool expanded
  )
{
  std::stringstream cmd;
  cmd << CmdName_SetNodeCommentExpanded();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringArg( FTL_STR("node"), node, cmd );
  encodeBooleanArg( FTL_STR("expanded"), expanded, cmd );
  cmd << ';';

  MGlobal::executeCommand(
    cmd.str().c_str(),
    true, // displayEnabled
    true  // undoEnabled
    );
}

void DFGUICmdHandler_Maya::dfgDoSetCode(
  FTL::CStrRef desc,
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef code
  )
{
  std::stringstream cmd;
  cmd << CmdName_SetCode();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringArg( FTL_STR("code"), code, cmd );
  cmd << ';';

  MGlobal::executeCommand(
    cmd.str().c_str(),
    true, // displayEnabled
    true  // undoEnabled
    );
}

std::string DFGUICmdHandler_Maya::dfgDoRenameExecPort(
  FTL::CStrRef desc,
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef name,
  FTL::CStrRef desiredName
  )
{
  std::stringstream cmd;
  cmd << CmdName_SetCode();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringArg( FTL_STR("name"), name, cmd );
  encodeStringArg( FTL_STR("desiredName"), desiredName, cmd );
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
  FTL::CStrRef desc,
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef json,
  QPointF cursorPos
  )
{
  std::stringstream cmd;
  cmd << CmdName_Paste();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringArg( FTL_STR("json"), json, cmd );
  encodePositionArg( FTL_STR("position"), cursorPos, cmd );
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
  FTL::CStrRef desc,
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef name,
  FTL::CStrRef type
  )
{
  std::stringstream cmd;
  cmd << CmdName_SetArgType();
  encodeBinding( binding, cmd );
  encodeStringArg( FTL_STR("name"), name, cmd );
  encodeStringArg( FTL_STR("type"), type, cmd );
  cmd << ';';

  MGlobal::executeCommand(
    cmd.str().c_str(),
    true, // displayEnabled
    true  // undoEnabled
    );
}

void DFGUICmdHandler_Maya::dfgDoSetArgValue(
  FTL::CStrRef desc,
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef name,
  FabricCore::RTVal const &value
  )
{
  std::stringstream cmd;
  cmd << CmdName_SetArgValue();
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
  FTL::CStrRef desc,
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef port,
  FabricCore::RTVal const &value
  )
{
  std::stringstream cmd;
  cmd << CmdName_SetPortDefaultValue();
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
  FTL::CStrRef desc,
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef node,
  FTL::CStrRef varPath
  )
{
  std::stringstream cmd;
  cmd << CmdName_SetRefVarPath();
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
