/*
 *  Copyright 2010-2015 Fabric Software Inc. All rights reserved.
 */

#include "DFGUICmdHandler_Maya.h"
#include "FabricDFGBaseInterface.h"

#include <maya/MGlobal.h>

#include <sstream>

static void EncodeMELString(
  FTL::StrRef str,
  std::stringstream &ss
  )
{
  ss << '"';
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
  ss << '"';
}

std::string DFGUICmdHandler_Maya::dfgDoAddInstFromPreset(
  FTL::CStrRef desc,
  FabricCore::DFGBinding const &binding,
  FTL::CStrRef execPath,
  FabricCore::DFGExec const &exec,
  FTL::CStrRef presetPath,
  QPointF pos
  )
{
  std::stringstream cmd;
  cmd << CmdName_AddInstFromPreset();
  cmd << " -mayaNode ";
  EncodeMELString( getNodeNameFromBinding( binding ).asChar(), cmd );
  cmd << " -execPath ";
  EncodeMELString( execPath, cmd );
  cmd << " -presetPath ";
  EncodeMELString( presetPath, cmd );
  cmd << " -position ";
  cmd << pos.x();
  cmd << " ";
  cmd << pos.y();
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
  FTL::CStrRef desiredNodeName,
  FTL::CStrRef dataType,
  FTL::CStrRef extDep,
  QPointF pos
  )
{
  std::stringstream cmd;
  cmd << CmdName_AddVar();
  cmd << " -mayaNode ";
  EncodeMELString( getNodeNameFromBinding( binding ).asChar(), cmd );
  cmd << " -execPath ";
  EncodeMELString( execPath, cmd );
  cmd << " -desiredNodeName ";
  EncodeMELString( desiredNodeName, cmd );
  cmd << " -dataType ";
  EncodeMELString( dataType, cmd );
  cmd << " -extDep ";
  EncodeMELString( extDep, cmd );
  cmd << " -position ";
  cmd << pos.x();
  cmd << " ";
  cmd << pos.y();
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
  FTL::CStrRef desiredNodeName,
  FTL::CStrRef varPath,
  QPointF pos
  )
{
  std::stringstream cmd;
  cmd << CmdName_AddGet();
  cmd << " -mayaNode ";
  EncodeMELString( getNodeNameFromBinding( binding ).asChar(), cmd );
  cmd << " -execPath ";
  EncodeMELString( execPath, cmd );
  cmd << " -desiredNodeName ";
  EncodeMELString( desiredNodeName, cmd );
  cmd << " -varPath ";
  EncodeMELString( varPath, cmd );
  cmd << " -position ";
  cmd << pos.x();
  cmd << " ";
  cmd << pos.y();
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
  FTL::CStrRef desiredNodeName,
  FTL::CStrRef varPath,
  QPointF pos
  )
{
  std::stringstream cmd;
  cmd << CmdName_AddSet();
  cmd << " -mayaNode ";
  EncodeMELString( getNodeNameFromBinding( binding ).asChar(), cmd );
  cmd << " -execPath ";
  EncodeMELString( execPath, cmd );
  cmd << " -desiredNodeName ";
  EncodeMELString( desiredNodeName, cmd );
  cmd << " -varPath ";
  EncodeMELString( varPath, cmd );
  cmd << " -position ";
  cmd << pos.x();
  cmd << " ";
  cmd << pos.y();
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
  cmd << " -mayaNode ";
  EncodeMELString( getNodeNameFromBinding( binding ).asChar(), cmd );
  cmd << " -execPath ";
  EncodeMELString( execPath, cmd );
  cmd << " -title ";
  EncodeMELString( title, cmd );
  cmd << " -position ";
  cmd << pos.x();
  cmd << " ";
  cmd << pos.y();
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
  FTL::CStrRef initialCode,
  QPointF pos
  )
{
  std::stringstream cmd;
  cmd << CmdName_AddInstWithEmptyFunc();
  cmd << " -mayaNode ";
  EncodeMELString( getNodeNameFromBinding( binding ).asChar(), cmd );
  cmd << " -execPath ";
  EncodeMELString( execPath, cmd );
  cmd << " -title ";
  EncodeMELString( title, cmd );
  cmd << " -initialCode ";
  EncodeMELString( initialCode, cmd );
  cmd << " -position ";
  cmd << pos.x();
  cmd << " ";
  cmd << pos.y();
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
  FTL::ArrayRef<FTL::CStrRef> nodeNames
  )
{
  std::stringstream cmd;
  cmd << CmdName_RemoveNodes();
  cmd << " -mayaNode ";
  EncodeMELString( getNodeNameFromBinding( binding ).asChar(), cmd );
  cmd << " -execPath ";
  EncodeMELString( execPath, cmd );
  for ( FTL::ArrayRef<FTL::StrRef>::IT it = nodeNames.begin();
    it != nodeNames.end(); ++it )
  {
    FTL::StrRef nodeName = *it;
    cmd << " -nodeName ";
    EncodeMELString( nodeName, cmd );
  }
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
  cmd << " -mayaNode ";
  EncodeMELString( getNodeNameFromBinding( binding ).asChar(), cmd );
  cmd << " -execPath ";
  EncodeMELString( execPath, cmd );
  cmd << " -srcPort ";
  EncodeMELString( srcPort, cmd );
  cmd << " -dstPort ";
  EncodeMELString( dstPort, cmd );
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
  cmd << " -mayaNode ";
  EncodeMELString( getNodeNameFromBinding( binding ).asChar(), cmd );
  cmd << " -execPath ";
  EncodeMELString( execPath, cmd );
  cmd << " -srcPort ";
  EncodeMELString( srcPort, cmd );
  cmd << " -dstPort ";
  EncodeMELString( dstPort, cmd );
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
  MString interfIdStr = binding.getExec().getMetadata("maya_id");
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
