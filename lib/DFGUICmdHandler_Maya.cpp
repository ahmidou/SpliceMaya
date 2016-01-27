/*
 *  Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
 */

// [pzion 20150731] This needs to come first, otherwise macros will
// mess up Qt headers
#include <QtCore/QtCore>

#include "DFGUICmdHandler_Maya.h"
#include "FabricDFGBaseInterface.h"

#include <FabricUI/DFG/DFGUICmd/DFGUICmds.h>
#include <FabricUI/DFG/DFGController.h>

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

void DFGUICmdHandler_Maya::encodeMELStringChars(
  FTL::StrRef str,
  std::stringstream &ss
  )
{
  for ( FTL::StrRef::IT it = str.begin(); it != str.end(); ++it )
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
}

void DFGUICmdHandler_Maya::encodeMELString(
  FTL::StrRef str,
  std::stringstream &ss
  )
{
  ss << '"';
  encodeMELStringChars( str, ss );
  ss << '"';
}

void DFGUICmdHandler_Maya::encodeStringArg(
  FTL::CStrRef name,
  FTL::StrRef value,
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
  FTL::ArrayRef<FTL::StrRef> values,
  std::stringstream &cmd
  )
{
  cmd << " -";
  cmd << name.c_str();
  cmd << " \"";
  FTL::ArrayRef<FTL::StrRef>::IT itBegin = values.begin();
  FTL::ArrayRef<FTL::StrRef>::IT itEnd = values.end();
  for ( FTL::ArrayRef<FTL::StrRef>::IT it = itBegin; it != itEnd; ++it )
  {
    FTL::StrRef value = *it;
    if ( it != itBegin )
      cmd << '|';
    encodeMELStringChars( value, cmd );
  }
  cmd << '"';
}

void DFGUICmdHandler_Maya::encodeNamesArg(
  FTL::ArrayRef<FTL::StrRef> values,
  std::stringstream &cmd
  )
{
  encodeStringsArg( FTL_STR("n"), values, cmd );
}

void DFGUICmdHandler_Maya::encodeFloat64Arg(
  FTL::CStrRef name,
  double value,
  std::stringstream &cmd
  )
{
  cmd << " -";
  cmd << name.c_str();
  cmd << " \"";
  cmd << value;
  cmd << '"';
}

void DFGUICmdHandler_Maya::encodePositionArg(
  QPointF value,
  std::stringstream &cmd
  )
{
  encodeFloat64Arg( FTL_STR("x"), value.x(), cmd );
  encodeFloat64Arg( FTL_STR("y"), value.y(), cmd );
}

void DFGUICmdHandler_Maya::encodeSizeArg(
  QSizeF value,
  std::stringstream &cmd
  )
{
  encodeFloat64Arg( FTL_STR("w"), value.width(), cmd );
  encodeFloat64Arg( FTL_STR("h"), value.height(), cmd );
}

void DFGUICmdHandler_Maya::encodeFloat64sArg(
  FTL::CStrRef name,
  FTL::ArrayRef<double> values,
  std::stringstream &cmd
  )
{
  cmd << " -";
  cmd << name.c_str();
  cmd << " \"";
  FTL::ArrayRef<double>::IT itBegin = values.begin();
  FTL::ArrayRef<double>::IT itEnd = values.end();
  for ( FTL::ArrayRef<double>::IT it = itBegin; it != itEnd; ++it )
  {
    double value = *it;
    if ( it != itBegin )
      cmd << '|';
    cmd << value;
  }
  cmd << '"';
}

void DFGUICmdHandler_Maya::encodePositionsArg(
  FTL::ArrayRef<QPointF> values,
  std::stringstream &cmd
  )
{
  {
    std::vector<double> xValues;
    xValues.reserve( values.size() );
    for ( FTL::ArrayRef<QPointF>::IT it = values.begin();
      it != values.end(); ++it )
      xValues.push_back( it->x() );
    encodeFloat64sArg( FTL_STR("x"), xValues, cmd );
  }

  {
    std::vector<double> yValues;
    yValues.reserve( values.size() );
    for ( FTL::ArrayRef<QPointF>::IT it = values.begin();
      it != values.end(); ++it )
      yValues.push_back( it->y() );
    encodeFloat64sArg( FTL_STR("y"), yValues, cmd );
  }
}

void DFGUICmdHandler_Maya::encodeSizesArg(
  FTL::ArrayRef<QSizeF> values,
  std::stringstream &cmd
  )
{
  {
    std::vector<double> wValues;
    wValues.reserve( values.size() );
    for ( FTL::ArrayRef<QSizeF>::IT it = values.begin();
      it != values.end(); ++it )
      wValues.push_back( it->width() );
    encodeFloat64sArg( FTL_STR("w"), wValues, cmd );
  }

  {
    std::vector<double> hValues;
    hValues.reserve( values.size() );
    for ( FTL::ArrayRef<QSizeF>::IT it = values.begin();
      it != values.end(); ++it )
      hValues.push_back( it->height() );
    encodeFloat64sArg( FTL_STR("h"), hValues, cmd );
  }
}

void DFGUICmdHandler_Maya::encodeBinding(
  FabricCore::DFGBinding const &binding,
  std::stringstream &cmd
  )
{
  FTL::CStrRef mayaNodeName = getNodeNameFromBinding( binding ).asChar();
  encodeStringArg( FTL_STR("m"), mayaNodeName.c_str(), cmd );
}

void DFGUICmdHandler_Maya::encodeExec(
  FabricCore::DFGBinding const &binding,
  FTL::StrRef execPath,
  FabricCore::DFGExec const &exec,
  std::stringstream &cmd
  )
{
  encodeBinding( binding, cmd );
  encodeStringArg( FTL_STR("e"), execPath, cmd );
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
  encodeStringArg( FTL_STR("p"), preset, cmd );
  encodePositionArg( pos, cmd );
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
  encodeStringArg( FTL_STR("d"), desiredNodeName, cmd );
  encodeStringArg( FTL_STR("t"), type, cmd );
  encodeStringArg( FTL_STR("xd"), extDep, cmd );
  encodePositionArg( pos, cmd );
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
  encodeStringArg( FTL_STR("d"), desiredNodeName, cmd );
  encodeStringArg( FTL_STR("p"), varPath, cmd );
  encodePositionArg( pos, cmd );
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
  encodeStringArg( FTL_STR("d"), desiredNodeName, cmd );
  encodeStringArg( FTL_STR("p"), varPath, cmd );
  encodePositionArg( pos, cmd );
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
  encodeStringArg( FTL_STR("t"), title, cmd );
  encodePositionArg( pos, cmd );
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
  encodeStringArg( FTL_STR("t"), title, cmd );
  encodeStringArg( FTL_STR("c"), code, cmd );
  encodePositionArg( pos, cmd );
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
  FTL::ArrayRef<FTL::StrRef> nodes
  )
{
  std::stringstream cmd;
  cmd << FabricUI::DFG::DFGUICmd_RemoveNodes::CmdName();
  encodeExec( binding, execPath, exec, cmd );
  encodeNamesArg( nodes, cmd );
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
  encodeStringArg( FTL_STR("s"), srcPortPath, cmd );
  encodeStringArg( FTL_STR("d"), dstPortPath, cmd );
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
  encodeStringArg( FTL_STR("s"), srcPortPath, cmd );
  encodeStringArg( FTL_STR("d"), dstPortPath, cmd );
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
  FTL::CStrRef connectToPortPath,
  FTL::StrRef extDep,
  FTL::CStrRef uiMetadata
  )
{
  std::stringstream cmd;
  cmd << FabricUI::DFG::DFGUICmd_AddPort::CmdName();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringArg( FTL_STR("d"), desiredPortName, cmd );
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
  encodeStringArg( FTL_STR("p"), portTypeStr, cmd );
  encodeStringArg( FTL_STR("t"), typeSpec, cmd );
  if(!connectToPortPath.empty())
  encodeStringArg( FTL_STR("c"), connectToPortPath, cmd );
  if(!extDep.empty())
    encodeStringArg( FTL_STR("xd"), extDep, cmd );
  if(!uiMetadata.empty())
    encodeStringArg( FTL_STR("ui"), uiMetadata, cmd );
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

std::string DFGUICmdHandler_Maya::dfgDoCreatePreset(
  FabricCore::DFGBinding const &binding,
  FTL::StrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::StrRef nodeName,
  FTL::StrRef presetDirPath,
  FTL::StrRef presetName
  )
{
  std::stringstream cmd;
  cmd << FabricUI::DFG::DFGUICmd_CreatePreset::CmdName();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringArg( FTL_STR("n"), nodeName, cmd );
  encodeStringArg( FTL_STR("pd"), presetDirPath, cmd );
  encodeStringArg( FTL_STR("pn"), presetName, cmd );
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

std::string DFGUICmdHandler_Maya::dfgDoEditPort(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::StrRef oldPortName,
  FTL::StrRef desiredNewPortName,
  FTL::StrRef typeSpec,
  FTL::StrRef extDep,
  FTL::StrRef uiMetadata
  )
{
  std::stringstream cmd;
  cmd << FabricUI::DFG::DFGUICmd_EditPort::CmdName();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringArg( FTL_STR("n"), oldPortName, cmd );
  if(!desiredNewPortName.empty())
    encodeStringArg( FTL_STR("d"), desiredNewPortName, cmd );
  if(!typeSpec.empty())
    encodeStringArg( FTL_STR("t"), typeSpec, cmd );
  if(!extDep.empty())
    encodeStringArg( FTL_STR("xd"), extDep, cmd );
  if(!uiMetadata.empty())
    encodeStringArg( FTL_STR("ui"), uiMetadata, cmd );
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
  encodeStringArg( FTL_STR("n"), portName, cmd );
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
  FTL::ArrayRef<FTL::StrRef> nodes,
  FTL::ArrayRef<QPointF> poss
  )
{
  std::stringstream cmd;
  cmd << FabricUI::DFG::DFGUICmd_MoveNodes::CmdName();
  encodeExec( binding, execPath, exec, cmd );
  encodeNamesArg( nodes, cmd );
  encodePositionsArg( poss, cmd );
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
  FTL::CStrRef nodeName,
  QPointF newTopLeftPos,
  QSizeF newSize
  )
{
  std::stringstream cmd;
  cmd << FabricUI::DFG::DFGUICmd_ResizeBackDrop::CmdName();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringArg( FTL_STR("n"), nodeName, cmd );
  encodePositionArg( newTopLeftPos, cmd );
  encodeSizeArg( newSize, cmd );
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
  FTL::ArrayRef<FTL::StrRef> nodeNames,
  FTL::CStrRef desiredNodeName
  )
{
  std::stringstream cmd;
  cmd << FabricUI::DFG::DFGUICmd_ImplodeNodes::CmdName();
  encodeExec( binding, execPath, exec, cmd );
  encodeNamesArg( nodeNames, cmd );
  encodeStringArg( FTL_STR("d"), desiredNodeName, cmd );
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
  encodeStringArg( FTL_STR("t"), title, cmd );
  encodePositionArg( pos, cmd );
  cmd << ';';

  MGlobal::executeCommand(
    cmd.str().c_str(),
    true, // displayEnabled
    true  // undoEnabled
    );
}

void DFGUICmdHandler_Maya::dfgDoSetTitle(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef title
  )
{
  std::stringstream cmd;
  cmd << FabricUI::DFG::DFGUICmd_SetTitle::CmdName();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringArg( FTL_STR("t"), title, cmd );
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
  encodeStringArg( FTL_STR("n"), node, cmd );
  encodeStringArg( FTL_STR("c"), comment, cmd );
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
  encodeStringArg( FTL_STR("c"), code, cmd );
  cmd << ';';

  MGlobal::executeCommand(
    cmd.str().c_str(),
    true, // displayEnabled
    true  // undoEnabled
    );
}

std::string DFGUICmdHandler_Maya::dfgDoEditNode(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::StrRef oldNodeName,
  FTL::StrRef desiredNewNodeName,
  FTL::StrRef nodeMetadata,
  FTL::StrRef execMetadata
  )
{
  std::stringstream cmd;
  cmd << FabricUI::DFG::DFGUICmd_EditNode::CmdName();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringArg( FTL_STR("n"), oldNodeName, cmd );
  encodeStringArg( FTL_STR("d"), desiredNewNodeName, cmd );
  encodeStringArg( FTL_STR("nm"), nodeMetadata, cmd );
  encodeStringArg( FTL_STR("xm"), execMetadata, cmd );
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
  encodeStringArg( FTL_STR("n"), name, cmd );
  encodeStringArg( FTL_STR("d"), desiredName, cmd );
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
  FTL::CStrRef text,
  QPointF cursorPos
  )
{
  std::stringstream cmd;
  cmd << FabricUI::DFG::DFGUICmd_Paste::CmdName();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringArg( FTL_STR("t"), text, cmd );
  encodePositionArg( cursorPos, cmd );
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
  encodeStringArg( FTL_STR("n"), argName, cmd );
  encodeStringArg( FTL_STR("t"), typeName, cmd );
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
  encodeStringArg( FTL_STR("n"), name, cmd );
  encodeStringArg( FTL_STR("t"), value.getTypeNameCStr(), cmd );
  FabricCore::RTVal valueJSON = value.getJSON();
  encodeStringArg( FTL_STR("v"), valueJSON.getStringCString(), cmd );
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
  FTL::CStrRef portPath,
  FabricCore::RTVal const &value
  )
{
  std::stringstream cmd;
  cmd << FabricUI::DFG::DFGUICmd_SetPortDefaultValue::CmdName();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringArg( FTL_STR("p"), portPath, cmd );
  encodeStringArg( FTL_STR("t"), value.getTypeNameCStr(), cmd );

  FabricCore::Context context = binding.getHost().getContext();
  std::string json = encodeRTValToJSON(context, value);
  encodeStringArg( FTL_STR("v"), json.c_str(), cmd );
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
  FTL::CStrRef refName,
  FTL::CStrRef varPath
  )
{
  std::stringstream cmd;
  cmd << FabricUI::DFG::DFGUICmd_SetRefVarPath::CmdName();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringArg( FTL_STR("n"), refName, cmd );
  encodeStringArg( FTL_STR("p"), varPath, cmd );
  cmd << ';';

  MGlobal::executeCommand(
    cmd.str().c_str(),
    true, // displayEnabled
    true  // undoEnabled
    );
}

void DFGUICmdHandler_Maya::dfgDoReorderPorts(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  const std::vector<unsigned int> & indices
  )
{
  std::stringstream cmd;
  cmd << FabricUI::DFG::DFGUICmd_ReorderPorts::CmdName();
  encodeExec( binding, execPath, exec, cmd );

  cmd << " -i";
  cmd << " \"[";
  for(size_t i=0;i<indices.size();i++)
  {
    if(i > 0)
      cmd << ", ";
    cmd << (int)indices[i];
  }
  cmd << "]\"";
  cmd << ';';

  MGlobal::executeCommand(
    cmd.str().c_str(),
    true, // displayEnabled
    true  // undoEnabled
    );

}

void DFGUICmdHandler_Maya::dfgDoSetExtDeps(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::ArrayRef<FTL::StrRef> extDeps
  )
{
  std::stringstream cmd;
  cmd << FabricUI::DFG::DFGUICmd_SetExtDeps::CmdName();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringsArg( "xd", extDeps, cmd );
  cmd << ';';

  MGlobal::executeCommand(
    cmd.str().c_str(),
    true, // displayEnabled
    true  // undoEnabled
    );
}

void DFGUICmdHandler_Maya::dfgDoSplitFromPreset(
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec
  )
{
  std::stringstream cmd;
  cmd << FabricUI::DFG::DFGUICmd_SplitFromPreset::CmdName();
  encodeExec( binding, execPath, exec, cmd );
  cmd << ';';

  MGlobal::executeCommand(
    cmd.str().c_str(),
    true, // displayEnabled
    true  // undoEnabled
    );
}

FabricDFGBaseInterface * DFGUICmdHandler_Maya::getInterfFromBinding( FabricCore::DFGBinding const &binding )
{
  MString interfIdStr = binding.getMetadata("maya_id");
  if(interfIdStr.length() == 0)
    return NULL;
  unsigned int interfId = (unsigned int)interfIdStr.asInt();

  FabricDFGBaseInterface * interf =
    FabricDFGBaseInterface::getInstanceById(interfId);
  return interf;  
}

MString DFGUICmdHandler_Maya::getNodeNameFromBinding(
  FabricCore::DFGBinding const &binding
  )
{
  FabricDFGBaseInterface * interf = getInterfFromBinding(binding);
  if(interf == NULL)
    return "";

  MFnDependencyNode thisNode(interf->getThisMObject());
  return thisNode.name();
}
