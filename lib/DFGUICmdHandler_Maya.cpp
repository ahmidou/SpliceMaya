//
// Copyright (c) 2010-2017 Fabric Software Inc. All rights reserved.
//

// [pzion 20150731] This needs to come first, otherwise macros will
// mess up Qt headers
#include <QtCore>
#include <QtGui/qevent.h>

#include "DFGUICmdHandler_Maya.h"
#include "FabricDFGBaseInterface.h"

#include <FabricUI/DFG/DFGUICmd/DFGUICmds.h>
#include <FabricUI/DFG/DFGController.h>

#include <maya/MGlobal.h>

#include <sstream>

/*
    helpers
*/

void DFGUICmdHandler_Maya::encodeFlag(
  FTL::CStrRef name,
  std::stringstream &cmd
  )
{
  cmd << " -";
  cmd << name.c_str();
  cmd << ' ';
}

void DFGUICmdHandler_Maya::encodeBooleanArg(
  FTL::CStrRef name,
  bool value,
  std::stringstream &cmd
  )
{
  encodeFlag( name, cmd );
  cmd << (value? "true": "false");
}

void DFGUICmdHandler_Maya::encodeMELStringChars(
  QString str,
  std::stringstream &ss
  )
{
  QByteArray ba = str.toUtf8();
  for ( int i = 0; i < ba.size(); ++i )
  {
    switch ( ba[i] )
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
        ss << ba[i];
        break;
    }
  }
}

void DFGUICmdHandler_Maya::encodeMELString(
  QString str,
  std::stringstream &ss
  )
{
  ss << '"';
  encodeMELStringChars( str, ss );
  ss << '"';
}

void DFGUICmdHandler_Maya::encodeStringArg(
  FTL::CStrRef name,
  QString value,
  std::stringstream &cmd
  )
{
  encodeFlag( name, cmd );
  encodeMELString( value, cmd );
}

void DFGUICmdHandler_Maya::encodeStringsArg(
  FTL::CStrRef name,
  QStringList values,
  std::stringstream &cmd
  )
{
  encodeFlag( name, cmd );
  cmd << '"';
  QStringList::const_iterator itBegin = values.begin();
  QStringList::const_iterator itEnd = values.end();
  for ( QStringList::const_iterator it = itBegin; it != itEnd; ++it )
  {
    if ( it != itBegin )
      cmd << '|';
    encodeMELStringChars( *it, cmd );
  }
  cmd << '"';
}

void DFGUICmdHandler_Maya::encodeNamesArg(
  QStringList values,
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
  encodeFlag( name, cmd );
  cmd << '"';
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
  QList<double> values,
  std::stringstream &cmd
  )
{
  encodeFlag( name, cmd );
  cmd << '"';
  QList<double>::const_iterator itBegin = values.begin();
  QList<double>::const_iterator itEnd = values.end();
  for ( QList<double>::const_iterator it = itBegin; it != itEnd; ++it )
  {
    if ( it != itBegin )
      cmd << '|';
    cmd << *it;
  }
  cmd << '"';
}

void DFGUICmdHandler_Maya::encodePositionsArg(
  QList<QPointF> values,
  std::stringstream &cmd
  )
{
  {
    QList<double> xValues;
    xValues.reserve( values.size() );
    for ( QList<QPointF>::const_iterator it = values.begin();
      it != values.end(); ++it )
      xValues.append( it->x() );
    encodeFloat64sArg( FTL_STR("x"), xValues, cmd );
  }

  {
    QList<double> yValues;
    yValues.reserve( values.size() );
    for ( QList<QPointF>::const_iterator it = values.begin();
      it != values.end(); ++it )
      yValues.append( it->y() );
    encodeFloat64sArg( FTL_STR("y"), yValues, cmd );
  }
}

void DFGUICmdHandler_Maya::encodeSizesArg(
  QList<QSizeF> values,
  std::stringstream &cmd
  )
{
  {
    QList<double> wValues;
    wValues.reserve( values.size() );
    for ( QList<QSizeF>::const_iterator it = values.begin();
      it != values.end(); ++it )
      wValues.append( it->width() );
    encodeFloat64sArg( FTL_STR("w"), wValues, cmd );
  }

  {
    QList<double> hValues;
    hValues.reserve( values.size() );
    for ( QList<QSizeF>::const_iterator it = values.begin();
      it != values.end(); ++it )
      hValues.append( it->height() );
    encodeFloat64sArg( FTL_STR("h"), hValues, cmd );
  }
}

void DFGUICmdHandler_Maya::encodeBinding(
  FabricCore::DFGBinding const &binding,
  std::stringstream &cmd
  )
{
  QString mayaNodeName = getNodeNameFromBinding( binding ).asChar();
  encodeStringArg( FTL_STR("m"), mayaNodeName, cmd );
}

void DFGUICmdHandler_Maya::encodeExec(
  FabricCore::DFGBinding const &binding,
  QString execPath,
  FabricCore::DFGExec const &exec,
  std::stringstream &cmd
  )
{
  encodeBinding( binding, cmd );
  encodeStringArg( FTL_STR("e"), execPath, cmd );
}

/*
    commands
*/

void DFGUICmdHandler_Maya::dfgDoRemoveNodes(
  FabricCore::DFGBinding const &binding,
  QString execPath,
  FabricCore::DFGExec const &exec,
  QStringList nodes
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
  QString execPath,
  FabricCore::DFGExec const &exec,
  QStringList srcPortPaths,
  QStringList dstPortPaths
  )
{
  std::stringstream cmd;
  cmd << FabricUI::DFG::DFGUICmd_Connect::CmdName();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringsArg( FTL_STR("s"), srcPortPaths, cmd );
  encodeStringsArg( FTL_STR("d"), dstPortPaths, cmd );
  cmd << ';';

  MGlobal::executeCommand(
    cmd.str().c_str(),
    true, // displayEnabled
    true  // undoEnabled
    );
 }

void DFGUICmdHandler_Maya::dfgDoDisconnect(
  FabricCore::DFGBinding const &binding,
  QString execPath,
  FabricCore::DFGExec const &exec,
  QStringList srcPortPaths,
  QStringList dstPortPaths
  )
{
  std::stringstream cmd;
  cmd << FabricUI::DFG::DFGUICmd_Disconnect::CmdName();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringsArg( FTL_STR("s"), srcPortPaths, cmd );
  encodeStringsArg( FTL_STR("d"), dstPortPaths, cmd );
  cmd << ';';

  MGlobal::executeCommand(
    cmd.str().c_str(),
    true, // displayEnabled
    true  // undoEnabled
    );
}

QString DFGUICmdHandler_Maya::dfgDoAddGraph(
  FabricCore::DFGBinding const &binding,
  QString execPath,
  FabricCore::DFGExec const &exec,
  QString title,
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
  return QString::fromUtf8( result.asChar() );
}

QString DFGUICmdHandler_Maya::dfgDoImportNodeFromJSON(
    FabricCore::DFGBinding const &binding,
    QString execPath,
    FabricCore::DFGExec const &exec,
    QString nodeName,
    QString filePath,
    QPointF pos
  )
{
  std::stringstream cmd;
  cmd << FabricUI::DFG::DFGUICmd_ImportNodeFromJSON::CmdName();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringArg( FTL_STR("n"), nodeName, cmd );
  encodeStringArg( FTL_STR("p"), filePath, cmd );
  encodePositionArg( pos, cmd );
  cmd << ';';

  MString result;
  MGlobal::executeCommand(
    cmd.str().c_str(),
    result,
    true, // displayEnabled
    true  // undoEnabled
    );
  return QString::fromUtf8( result.asChar() );
}

QString DFGUICmdHandler_Maya::dfgDoAddFunc(
  FabricCore::DFGBinding const &binding,
  QString execPath,
  FabricCore::DFGExec const &exec,
  QString title,
  QString code,
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
  return QString::fromUtf8( result.asChar() );
}

QString DFGUICmdHandler_Maya::dfgDoInstPreset(
  FabricCore::DFGBinding const &binding,
  QString execPath,
  FabricCore::DFGExec const &exec,
  QString preset,
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
  return QString::fromUtf8( result.asChar() );
}

QString DFGUICmdHandler_Maya::dfgDoAddVar(
  FabricCore::DFGBinding const &binding,
  QString execPath,
  FabricCore::DFGExec const &exec,
  QString desiredNodeName,
  QString type,
  QString extDep,
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
  return QString::fromUtf8( result.asChar() );
}

QString DFGUICmdHandler_Maya::dfgDoAddGet(
  FabricCore::DFGBinding const &binding,
  QString execPath,
  FabricCore::DFGExec const &exec,
  QString desiredNodeName,
  QString varPath,
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
  return QString::fromUtf8( result.asChar() );
}

QString DFGUICmdHandler_Maya::dfgDoAddSet(
  FabricCore::DFGBinding const &binding,
  QString execPath,
  FabricCore::DFGExec const &exec,
  QString desiredNodeName,
  QString varPath,
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
  return QString::fromUtf8( result.asChar() );
}

QString DFGUICmdHandler_Maya::dfgDoAddPort(
  FabricCore::DFGBinding const &binding,
  QString execPath,
  FabricCore::DFGExec const &exec,
  QString desiredPortName,
  FabricCore::DFGPortType portType,
  QString typeSpec,
  QString connectToPortPath,
  QString extDep,
  QString uiMetadata
  )
{
  std::stringstream cmd;
  cmd << FabricUI::DFG::DFGUICmd_AddPort::CmdName();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringArg( FTL_STR("d"), desiredPortName, cmd );
  QString portTypeStr;
  switch ( portType )
  {
    case FabricCore::DFGPortType_In:
      portTypeStr = QString("In");
      break;
    case FabricCore::DFGPortType_IO:
      portTypeStr = QString("IO");
      break;
    case FabricCore::DFGPortType_Out:
      portTypeStr = QString("Out");
      break;
  }
  encodeStringArg( FTL_STR("p"), portTypeStr, cmd );
  encodeStringArg( FTL_STR("t"), typeSpec, cmd );
  if ( !connectToPortPath.isEmpty() )
  encodeStringArg( FTL_STR("c"), connectToPortPath, cmd );
  if ( !extDep.isEmpty() )
    encodeStringArg( FTL_STR("xd"), extDep, cmd );
  if ( !uiMetadata.isEmpty() )
    encodeStringArg( FTL_STR("ui"), uiMetadata, cmd );
  cmd << ';';

  MString mResult;
  MGlobal::executeCommand(
    cmd.str().c_str(),
    mResult,
    true, // displayEnabled
    true  // undoEnabled
    );

  /* FIXME [andrew 20160304]
  if(mResult.length() > 0)
  {
    FabricDFGBaseInterface * interf = getInterfFromBinding(binding);
    if(interf)
    {
      FabricCore::Client interfClient = interf->getCoreClient();
      FabricCore::DFGBinding interfBinding = interf->getDFGBinding();
      FabricUI::DFG::DFGController::bindUnboundRTVals(interfClient, interfBinding);
    }
  }
  */

  return QString::fromUtf8( mResult.asChar() );
}

QString DFGUICmdHandler_Maya::dfgDoAddInstPort(
  FabricCore::DFGBinding const &binding,
  QString execPath,
  FabricCore::DFGExec const &exec,
  QString instName,
  QString desiredPortName,
  FabricCore::DFGPortType portType,
  QString typeSpec,
  QString pathToConnect,
  FabricCore::DFGPortType connectType,
  QString extDep,
  QString metaData
  )
{
  std::stringstream cmd;
  cmd << FabricUI::DFG::DFGUICmd_AddInstPort::CmdName();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringArg( FTL_STR("n"), instName, cmd );
  encodeStringArg( FTL_STR("d"), desiredPortName, cmd );
  QString portTypeStr;
  switch ( portType )
  {
    case FabricCore::DFGPortType_In:
      portTypeStr = QString("In");
      break;
    case FabricCore::DFGPortType_IO:
      portTypeStr = QString("IO");
      break;
    case FabricCore::DFGPortType_Out:
      portTypeStr = QString("Out");
      break;
  }
  encodeStringArg( FTL_STR("p"), portTypeStr, cmd );
  encodeStringArg( FTL_STR("t"), typeSpec, cmd );
  if ( !pathToConnect.isEmpty() )
  encodeStringArg( FTL_STR("c"), pathToConnect, cmd );
  QString connectTypeStr;
  switch ( connectType )
  {
    case FabricCore::DFGPortType_In:
      connectTypeStr = QString("In");
      break;
    case FabricCore::DFGPortType_IO:
      connectTypeStr = QString("IO");
      break;
    case FabricCore::DFGPortType_Out:
      connectTypeStr = QString("Out");
      break;
  }
  encodeStringArg( FTL_STR("ct"), connectTypeStr, cmd );
  if ( !extDep.isEmpty() )
    encodeStringArg( FTL_STR("xd"), extDep, cmd );
  if ( !metaData.isEmpty() )
    encodeStringArg( FTL_STR("md"), metaData, cmd );
  cmd << ';';

  MString mResult;
  MGlobal::executeCommand(
    cmd.str().c_str(),
    mResult,
    true, // displayEnabled
    true  // undoEnabled
    );

  return QString::fromUtf8( mResult.asChar() );
}

QString DFGUICmdHandler_Maya::dfgDoAddInstBlockPort(
  FabricCore::DFGBinding const &binding,
  QString execPath,
  FabricCore::DFGExec const &exec,
  QString instName,
  QString blockName,
  QString desiredPortName,
  QString typeSpec,
  QString pathToConnect,
  QString extDep,
  QString metaData
  )
{
  std::stringstream cmd;
  cmd << FabricUI::DFG::DFGUICmd_AddInstBlockPort::CmdName();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringArg( FTL_STR("n"), instName, cmd );
  encodeStringArg( FTL_STR("b"), blockName, cmd );
  encodeStringArg( FTL_STR("d"), desiredPortName, cmd );
  encodeStringArg( FTL_STR("t"), typeSpec, cmd );
  if ( !pathToConnect.isEmpty() )
  encodeStringArg( FTL_STR("c"), pathToConnect, cmd );
  if ( !extDep.isEmpty() )
    encodeStringArg( FTL_STR("xd"), extDep, cmd );
  if ( !metaData.isEmpty() )
    encodeStringArg( FTL_STR("md"), metaData, cmd );
  cmd << ';';

  MString mResult;
  MGlobal::executeCommand(
    cmd.str().c_str(),
    mResult,
    true, // displayEnabled
    true  // undoEnabled
    );

  return QString::fromUtf8( mResult.asChar() );
}

QString DFGUICmdHandler_Maya::dfgDoCreatePreset(
  FabricCore::DFGBinding const &binding,
  QString execPath,
  FabricCore::DFGExec const &exec,
  QString nodeName,
  QString presetDirPath,
  QString presetName,
  bool updateOrigPreset
  )
{
  std::stringstream cmd;
  cmd << FabricUI::DFG::DFGUICmd_CreatePreset::CmdName();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringArg( FTL_STR("n"), nodeName, cmd );
  encodeStringArg( FTL_STR("pd"), presetDirPath, cmd );
  encodeStringArg( FTL_STR("pn"), presetName, cmd );
  encodeBooleanArg( FTL_STR("u"), updateOrigPreset, cmd );
  cmd << ';';

  MString mResult;
  MGlobal::executeCommand(
    cmd.str().c_str(),
    mResult,
    true, // displayEnabled
    true  // undoEnabled
    );
  return QString::fromUtf8( mResult.asChar() );
}

QString DFGUICmdHandler_Maya::dfgDoEditPort(
  FabricCore::DFGBinding const &binding,
  QString execPath,
  FabricCore::DFGExec const &exec,
  QString oldPortName,
  QString desiredNewPortName,
  FabricCore::DFGPortType portType,
  QString typeSpec,
  QString extDep,
  QString uiMetadata
  )
{
  std::stringstream cmd;
  cmd << FabricUI::DFG::DFGUICmd_EditPort::CmdName();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringArg( FTL_STR("n"), oldPortName, cmd );
  if ( !desiredNewPortName.isEmpty() )
    encodeStringArg( FTL_STR("d"), desiredNewPortName, cmd );
  QString portTypeStr;
  switch ( portType )
  {
    case FabricCore::DFGPortType_In:
      portTypeStr = QString("In");
      break;
    case FabricCore::DFGPortType_IO:
      portTypeStr = QString("IO");
      break;
    case FabricCore::DFGPortType_Out:
      portTypeStr = QString("Out");
      break;
  }
  if ( !portTypeStr.isEmpty() )
    encodeStringArg( FTL_STR("p"), portTypeStr, cmd );
  if ( !typeSpec.isEmpty() )
    encodeStringArg( FTL_STR("t"), typeSpec, cmd );
  if ( !extDep.isEmpty() )
    encodeStringArg( FTL_STR("xd"), extDep, cmd );
  if ( !uiMetadata.isEmpty() )
    encodeStringArg( FTL_STR("ui"), uiMetadata, cmd );
  cmd << ';';

  MString mResult;
  MGlobal::executeCommand(
    cmd.str().c_str(),
    mResult,
    true, // displayEnabled
    true  // undoEnabled
    );

  /* FIXME [andrew 20160304]
  if(mResult.length() > 0)
  {
    FabricDFGBaseInterface * interf = getInterfFromBinding(binding);
    if(interf)
    {
      FabricCore::Client interfClient = interf->getCoreClient();
      FabricCore::DFGBinding interfBinding = interf->getDFGBinding();
      FabricUI::DFG::DFGController::bindUnboundRTVals(interfClient, interfBinding);
    }
  }
  */

  return QString::fromUtf8( mResult.asChar() );
}

void DFGUICmdHandler_Maya::dfgDoRemovePort(
  FabricCore::DFGBinding const &binding,
  QString execPath,
  FabricCore::DFGExec const &exec,
  QStringList portNames
  )
{
  std::stringstream cmd;
  cmd << FabricUI::DFG::DFGUICmd_RemovePort::CmdName();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringsArg( FTL_STR("n"), portNames, cmd );
  cmd << ';';

  MGlobal::executeCommand(
    cmd.str().c_str(),
    true, // displayEnabled
    true  // undoEnabled
    );
}

void DFGUICmdHandler_Maya::dfgDoMoveNodes(
  FabricCore::DFGBinding const &binding,
  QString execPath,
  FabricCore::DFGExec const &exec,
  QStringList nodeNames,
  QList<QPointF> newTopLeftPoss
  )
{
  std::stringstream cmd;
  cmd << FabricUI::DFG::DFGUICmd_MoveNodes::CmdName();
  encodeExec( binding, execPath, exec, cmd );
  encodeNamesArg( nodeNames, cmd );
  encodePositionsArg( newTopLeftPoss, cmd );
  cmd << ';';

  MGlobal::executeCommand(
    cmd.str().c_str(),
    true, // displayEnabled
    true  // undoEnabled
    );
}

void DFGUICmdHandler_Maya::dfgDoResizeBackDrop(
  FabricCore::DFGBinding const &binding,
  QString execPath,
  FabricCore::DFGExec const &exec,
  QString nodeName,
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

QString DFGUICmdHandler_Maya::dfgDoImplodeNodes(
  FabricCore::DFGBinding const &binding,
  QString execPath,
  FabricCore::DFGExec const &exec,
  QStringList nodeNames,
  QString desiredNodeName
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
  return QString::fromUtf8( result.asChar() );
}

QStringList DFGUICmdHandler_Maya::dfgDoExplodeNode(
  FabricCore::DFGBinding const &binding,
  QString execPath,
  FabricCore::DFGExec const &exec,
  QString nodeName
  )
{
  std::stringstream cmd;
  cmd << FabricUI::DFG::DFGUICmd_ExplodeNode::CmdName();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringArg( FTL_STR("n"), nodeName, cmd );
  cmd << ';';

  MString mResult;
  MGlobal::executeCommand(
    cmd.str().c_str(),
    mResult,
    true, // displayEnabled
    true  // undoEnabled
    );

  FTL::StrRef explodedNodeNamesStr = mResult.asChar();
  QStringList result;
  while ( !explodedNodeNamesStr.empty() )
  {
    FTL::StrRef::Split split = explodedNodeNamesStr.split('|');
    result.append(
      QString::fromUtf8( split.first.data(), split.first.size() )
      );
    explodedNodeNamesStr = split.second;
  }
  return result;
}

QString DFGUICmdHandler_Maya::dfgDoAddBackDrop(
  FabricCore::DFGBinding const &binding,
  QString execPath,
  FabricCore::DFGExec const &exec,
  QString title,
  QPointF pos
  )
{
  std::stringstream cmd;
  cmd << FabricUI::DFG::DFGUICmd_AddBackDrop::CmdName();
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
  return QString::fromUtf8( result.asChar() );
}

void DFGUICmdHandler_Maya::dfgDoSetNodeComment(
  FabricCore::DFGBinding const &binding,
  QString execPath,
  FabricCore::DFGExec const &exec,
  QString node,
  QString comment
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
  QString execPath,
  FabricCore::DFGExec const &exec,
  QString code
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

QString DFGUICmdHandler_Maya::dfgDoEditNode(
  FabricCore::DFGBinding const &binding,
  QString execPath,
  FabricCore::DFGExec const &exec,
  QString oldNodeName,
  QString desiredNewNodeName,
  QString nodeMetadata,
  QString execMetadata
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

  return QString::fromUtf8( mResult.asChar() );
}

QString DFGUICmdHandler_Maya::dfgDoRenamePort(
  FabricCore::DFGBinding const &binding,
  QString execPath,
  FabricCore::DFGExec const &exec,
  QString portPath,
  QString desiredNewPortName
  )
{
  std::stringstream cmd;
  cmd << FabricUI::DFG::DFGUICmd_RenamePort::CmdName();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringArg( FTL_STR("n"), portPath, cmd );
  encodeStringArg( FTL_STR("d"), desiredNewPortName, cmd );
  cmd << ';';

  MString mResult;
  MGlobal::executeCommand(
    cmd.str().c_str(),
    mResult,
    true, // displayEnabled
    true  // undoEnabled
    );

  return QString::fromUtf8( mResult.asChar() );
}

QStringList DFGUICmdHandler_Maya::dfgDoPaste(
  FabricCore::DFGBinding const &binding,
  QString execPath,
  FabricCore::DFGExec const &exec,
  QString json,
  QPointF cursorPos
  )
{
  std::stringstream cmd;
  cmd << FabricUI::DFG::DFGUICmd_Paste::CmdName();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringArg( FTL_STR("t"), json, cmd );
  encodePositionArg( cursorPos, cmd );
  cmd << ';';

  MString mResult;
  MGlobal::executeCommand(
    cmd.str().c_str(),
    mResult,
    true, // displayEnabled
    true  // undoEnabled
    );

  FTL::StrRef pastedNodeNamesStr = mResult.asChar();
  QStringList result;
  while ( !pastedNodeNamesStr.empty() )
  {
    FTL::StrRef::Split split = pastedNodeNamesStr.split('|');
    result.append(
      QString::fromUtf8( split.first.data(), split.first.size() )
      );
    pastedNodeNamesStr = split.second;
  }
  return result;
}

void DFGUICmdHandler_Maya::dfgDoSetArgValue(
  FabricCore::DFGBinding const &binding,
  QString name,
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
  QString execPath,
  FabricCore::DFGExec const &exec,
  QString portPath,
  FabricCore::RTVal const &value
  )
{
  std::stringstream cmd;
  cmd << FabricUI::DFG::DFGUICmd_SetPortDefaultValue::CmdName();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringArg( FTL_STR("p"), portPath, cmd );
  encodeStringArg( FTL_STR("t"), value.getTypeNameCStr(), cmd );

  FabricCore::Context context = binding.getHost().getContext();
  QString json = encodeRTValToJSON( context, value );
  encodeStringArg( FTL_STR("v"), json, cmd );
  cmd << ';';

  MGlobal::executeCommand(
    cmd.str().c_str(),
    true, // displayEnabled
    true  // undoEnabled
    );
}

void DFGUICmdHandler_Maya::dfgDoSetRefVarPath(
  FabricCore::DFGBinding const &binding,
  QString execPath,
  FabricCore::DFGExec const &exec,
  QString refName,
  QString varPath
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
  QString execPath,
  FabricCore::DFGExec const &exec,
  QString itemPath,
  QList<int> indices
  )
{
  std::stringstream cmd;
  cmd << FabricUI::DFG::DFGUICmd_ReorderPorts::CmdName();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringArg( FTL_STR("p"), itemPath, cmd );

  cmd << " -i";
  cmd << " \"[";
  for(int i=0;i<indices.size();i++)
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
  QString execPath,
  FabricCore::DFGExec const &exec,
  QStringList extDeps
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
  QString execPath,
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

void DFGUICmdHandler_Maya::dfgDoDismissLoadDiags(
  FabricCore::DFGBinding const &binding,
  QList<int> diagIndices
  )
{
  std::stringstream cmd;
  cmd << FabricUI::DFG::DFGUICmd_DismissLoadDiags::CmdName();
  encodeBinding( binding, cmd );

  cmd << " -di";
  cmd << " \"[";
  for ( int i = 0; i < diagIndices.size(); ++i )
  {
    if ( i > 0 )
      cmd << ", ";
    cmd << diagIndices[i];
  }
  cmd << "]\"";
  cmd << ';';

  MGlobal::executeCommand(
    cmd.str().c_str(),
    true, // displayEnabled
    true  // undoEnabled
    );
}

QString DFGUICmdHandler_Maya::dfgDoAddBlock(
  FabricCore::DFGBinding const &binding,
  QString execPath,
  FabricCore::DFGExec const &exec,
  QString desiredName,
  QPointF pos
  )
{
  std::stringstream cmd;
  cmd << FabricUI::DFG::DFGUICmd_AddBlock::CmdName();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringArg( FTL_STR("d"), desiredName, cmd );
  encodePositionArg( pos, cmd );
  cmd << ';';

  MString result;
  MGlobal::executeCommand(
    cmd.str().c_str(),
    result,
    true, // displayEnabled
    true  // undoEnabled
    );
  return QString::fromUtf8( result.asChar() );
}

QString DFGUICmdHandler_Maya::dfgDoAddBlockPort(
  FabricCore::DFGBinding const &binding,
  QString execPath,
  FabricCore::DFGExec const &exec,
  QString blockName,
  QString desiredPortName,
  FabricCore::DFGPortType portType,
  QString typeSpec,
  QString pathToConnect,
  FabricCore::DFGPortType connectType,
  QString extDep,
  QString metaData
  )
{
  std::stringstream cmd;
  cmd << FabricUI::DFG::DFGUICmd_AddBlockPort::CmdName();
  encodeExec( binding, execPath, exec, cmd );
  encodeStringArg( FTL_STR("b"), blockName, cmd );
  encodeStringArg( FTL_STR("d"), desiredPortName, cmd );
  QString portTypeStr;
  switch ( portType )
  {
    case FabricCore::DFGPortType_In:
      portTypeStr = QString("In");
      break;
    case FabricCore::DFGPortType_IO:
      portTypeStr = QString("IO");
      break;
    case FabricCore::DFGPortType_Out:
      portTypeStr = QString("Out");
      break;
  }
  encodeStringArg( FTL_STR("p"), portTypeStr, cmd );
  encodeStringArg( FTL_STR("t"), typeSpec, cmd );
  if ( !pathToConnect.isEmpty() )
  encodeStringArg( FTL_STR("c"), pathToConnect, cmd );
  QString connectTypeStr;
  switch ( connectType )
  {
    case FabricCore::DFGPortType_In:
      connectTypeStr = QString("In");
      break;
    case FabricCore::DFGPortType_IO:
      connectTypeStr = QString("IO");
      break;
    case FabricCore::DFGPortType_Out:
      connectTypeStr = QString("Out");
      break;
  }
  encodeStringArg( FTL_STR("ct"), connectTypeStr, cmd );
  if ( !extDep.isEmpty() )
    encodeStringArg( FTL_STR("xd"), extDep, cmd );
  if ( !metaData.isEmpty() )
    encodeStringArg( FTL_STR("md"), metaData, cmd );
  cmd << ';';

  MString mResult;
  MGlobal::executeCommand(
    cmd.str().c_str(),
    mResult,
    true, // displayEnabled
    true  // undoEnabled
    );

  return QString::fromUtf8( mResult.asChar() );
}

/*
    misc
*/

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
