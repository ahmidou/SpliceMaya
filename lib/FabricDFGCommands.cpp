#include <QtGui/QFileDialog>  // [pzion 20150519] Must include first since something below defines macros that mess it up

#include "Foundation.h"
#include "FabricDFGCommands.h"
#include "FabricSpliceHelpers.h"

#include <maya/MStringArray.h>
#include <maya/MSyntax.h>
#include <maya/MArgDatabase.h>
#include <maya/MQtUtil.h>

#define kNodeFlag "-n"
#define kNodeFlagLong "-node"

MSyntax FabricDFGGetContextIDCommand::newSyntax()
{
  MSyntax syntax;
  syntax.enableQuery(false);
  syntax.enableEdit(false);
  return syntax;
}

void* FabricDFGGetContextIDCommand::creator()
{
  return new FabricDFGGetContextIDCommand;
}

MStatus FabricDFGGetContextIDCommand::doIt(const MArgList &args)
{
  MString result;
  try
  {
    FabricCore::Client client = FabricSplice::ConstructClient();
    result = client.getContextID();
  }
  catch(FabricSplice::Exception e)
  {
    mayaLogErrorFunc(MString(getName()) + ": "+e.what());
    return mayaErrorOccured();
  }

  setResult(result);
  return MS::kSuccess;
}

MSyntax FabricDFGGetBindingIDCommand::newSyntax()
{
  MSyntax syntax;
  syntax.addFlag(kNodeFlag, kNodeFlagLong, MSyntax::kString);
  syntax.enableQuery(false);
  syntax.enableEdit(false);
  return syntax;
}

void* FabricDFGGetBindingIDCommand::creator()
{
  return new FabricDFGGetBindingIDCommand;
}

MStatus FabricDFGGetBindingIDCommand::doIt(const MArgList &args)
{
  MStatus status;
  MArgParser argData(syntax(), args, &status);
  if(!argData.isFlagSet("node"))
  {
    mayaLogErrorFunc(MString(getName()) + ": Node (-n, -node) not provided.");
    return mayaErrorOccured();
  }

  MString node = argData.flagArgumentString("node", 0);
  FabricDFGBaseInterface * interf = FabricDFGBaseInterface::getInstanceByName(node.asChar());
  if(!interf)
  {
    mayaLogErrorFunc(MString(getName()) + ": Node '"+node+"' not found.");
    return mayaErrorOccured();
  }

  int result = interf->getDFGBinding().getBindingID();
  setResult(result);
  return MS::kSuccess;
}

// FabricNewDFGBaseCommand

void FabricNewDFGBaseCommand::addSyntax( MSyntax &syntax )
{
  syntax.enableQuery(false);
  syntax.enableEdit(false);
}

MStatus FabricNewDFGBaseCommand::doIt( const MArgList &args )
{
  MStatus status = MS::kSuccess;
  MArgParser argParser( syntax(), args, &status );
  if ( status == MS::kSuccess )
  {
    try
    {
      m_dfgUICmd = executeDFGUICmd( argParser );
    }
    catch ( ArgException e )
    {
      logError( e.getDesc() );
      status = e.getStatus();
    }
    catch ( FabricCore::Exception e )
    {
      logError( e.getDesc_cstr() );
      status = MS::kFailure;
    }
  }
  return status;
}

MStatus FabricNewDFGBaseCommand::undoIt()
{
  MStatus status = MS::kSuccess;
  try
  {
    m_dfgUICmd->undo();
  }
  catch ( FabricCore::Exception e )
  {
    logError( e.getDesc_cstr() );
    status = MS::kFailure;
  }
  return status;
}

MStatus FabricNewDFGBaseCommand::redoIt()
{
  MStatus status = MS::kSuccess;
  try
  {
    m_dfgUICmd->redo();
  }
  catch ( FabricCore::Exception e )
  {
    logError( e.getDesc_cstr() );
    status = MS::kFailure;
  }
  return status;
}

// FabricDFGBindingCommand

void FabricDFGBindingCommand::addSyntax( MSyntax &syntax )
{
  Parent::addSyntax( syntax );
  syntax.addFlag( "-mn", "-mayaNode", MSyntax::kString );
}

void FabricDFGBindingCommand::getArgs(
  MArgParser &argParser,
  Args &args
  )
{
  if ( !argParser.isFlagSet("mayaNode") )
    throw ArgException( MS::kFailure, "-mayaNode not provided." );
  MString mayaNodeName = argParser.flagArgumentString("mayaNode", 0);

  FabricDFGBaseInterface * interf =
    FabricDFGBaseInterface::getInstanceByName( mayaNodeName.asChar() );
  if ( !interf )
    throw ArgException( MS::kNotFound, "Maya node '" + mayaNodeName + "' not found." );
  args.binding = interf->getDFGBinding();
}

// FabricDFGExecCommand

void FabricDFGExecCommand::addSyntax( MSyntax &syntax )
{
  Parent::addSyntax( syntax );
  syntax.addFlag( "-e", "-exec", MSyntax::kString );
}

void FabricDFGExecCommand::getArgs(
  MArgParser &argParser,
  Args &args
  )
{
  Parent::getArgs( argParser, args );

  if ( !argParser.isFlagSet( "exec" ) )
    throw ArgException( MS::kFailure, "-exec not provided." );
  MString execPathMString = argParser.flagArgumentString( "exec", 0 );
  args.execPath = execPathMString.asChar();
  args.exec = args.binding.getExec().getSubExec( execPathMString.asChar() );
}

// FabricDFGAddNodeCommand

void FabricDFGAddNodeCommand::addSyntax( MSyntax &syntax )
{
  Parent::addSyntax( syntax );
  syntax.addFlag( "-xy", "-position", MSyntax::kDouble, MSyntax::kDouble );
}

void FabricDFGAddNodeCommand::getArgs( MArgParser &argParser, Args &args )
{
  Parent::getArgs( argParser, args );

  if ( argParser.isFlagSet("position") )
  {
    args.pos =
      QPointF(
        argParser.flagArgumentDouble("position", 0),
        argParser.flagArgumentDouble("position", 1)
        );
  }
  else args.pos = QPointF( 0, 0 );
}

// FabricDFGInstPresetCommand

void FabricDFGInstPresetCommand::addSyntax( MSyntax &syntax )
{
  Parent::addSyntax( syntax );
  syntax.addFlag("-p", "-preset", MSyntax::kString);
}

void FabricDFGInstPresetCommand::getArgs(
  MArgParser &argParser,
  Args &args
  )
{
  Parent::getArgs( argParser, args );

  if ( !argParser.isFlagSet( "preset" ) )
    throw ArgException( MS::kFailure, "-preset not provided." );
  args.presetPath = argParser.flagArgumentString( "preset", 0 ).asChar();
}

FabricUI::DFG::DFGUICmd *FabricDFGInstPresetCommand::executeDFGUICmd(
  MArgParser &argParser
  )
{
  Args args;
  getArgs( argParser, args );

  FabricUI::DFG::DFGUICmd_InstPreset *cmd =
    new FabricUI::DFG::DFGUICmd_InstPreset(
      args.binding,
      args.execPath,
      args.exec,
      args.presetPath,
      args.pos
      );
  cmd->doit();
  setResult( cmd->getActualNodeName().c_str() );
  return cmd;
}

// // FabricDFGAddGraphCommand

// MSyntax FabricDFGAddGraphCommand::newSyntax()
// {
//   MSyntax syntax;
//   Parent::addSyntax( syntax );
//   syntax.addFlag("-t", "-title", MSyntax::kString);
//   return syntax;
// }

// MStatus FabricDFGAddGraphCommand::invoke(
//   MArgParser &argParser,
//   FabricCore::DFGExec &exec
//   )
// {
//   if ( !argParser.isFlagSet( "title" ) )
//   {
//     logError( "-title not provided." );
//     return MS::kFailure;
//   }
//   MString title = argParser.flagArgumentString( "title", 0 );

//   FTL::CStrRef nodeName = exec.addInstWithNewGraph( title.asChar() );
//   incCoreUndoCount();

//   setPos( argParser, exec, nodeName );

//   setResult( MString( nodeName.c_str() ) );
//   return MS::kSuccess;
// }

// // FabricDFGAddFuncCommand

// MSyntax FabricDFGAddFuncCommand::newSyntax()
// {
//   MSyntax syntax;
//   Parent::addSyntax( syntax );
//   syntax.addFlag("-t", "-title", MSyntax::kString);
//   syntax.addFlag("-c", "-code", MSyntax::kString);
//   return syntax;
// }

// MStatus FabricDFGAddFuncCommand::invoke(
//   MArgParser &argParser,
//   FabricCore::DFGExec &exec
//   )
// {
//   if ( !argParser.isFlagSet( "title" ) )
//   {
//     logError( "-title not provided." );
//     return MS::kFailure;
//   }
//   MString title = argParser.flagArgumentString( "title", 0 );

//   FTL::CStrRef nodeName = exec.addInstWithNewFunc( title.asChar() );
//   incCoreUndoCount();

//   if ( argParser.isFlagSet( "code" ) )
//   {
//     MString initialCode = argParser.flagArgumentString( "code", 0 );
//     FabricCore::DFGExec subExec = exec.getSubExec( nodeName.c_str() );
//     subExec.setCode( initialCode.asChar() );
//     incCoreUndoCount();
//   }

//   setPos( argParser, exec, nodeName );

//   setResult( MString( nodeName.c_str() ) );
//   return MS::kSuccess;
// }

// // FabricDFGAddVarCommand

// MSyntax FabricDFGAddVarCommand::newSyntax()
// {
//   MSyntax syntax;
//   Parent::addSyntax( syntax );
//   syntax.addFlag("-dt", "-dataType", MSyntax::kString);
//   syntax.addFlag("-dn", "-desiredName", MSyntax::kString);
//   syntax.addFlag("-ed", "-extDep", MSyntax::kString);
//   return syntax;
// }

// MStatus FabricDFGAddVarCommand::invoke(
//   MArgParser &argParser,
//   FabricCore::DFGExec &exec
//   )
// {
//   MString desiredNodeName;
//   if ( argParser.isFlagSet( "desiredName" ) )
//     desiredNodeName = argParser.flagArgumentString( "desiredName", 0 );

//   if ( !argParser.isFlagSet( "dataType" ) )
//   {
//     logError( "-dataType not provided." );
//     return MS::kFailure;
//   }
//   MString dataType = argParser.flagArgumentString( "dataType", 0 );

//   MString extDep;
//   if ( argParser.isFlagSet( "extDep" ) )
//     extDep = argParser.flagArgumentString( "extDep", 0 );

//   FTL::CStrRef nodeName =
//     exec.addVar(
//       desiredNodeName.asChar(),
//       dataType.asChar(),
//       extDep.asChar()
//       );
//   incCoreUndoCount();

//   setPos( argParser, exec, nodeName );

//   setResult( MString( nodeName.c_str() ) );
//   return MS::kSuccess;
// }

// // FabricDFGAddGetCommand

// MSyntax FabricDFGAddGetCommand::newSyntax()
// {
//   MSyntax syntax;
//   Parent::addSyntax( syntax );
//   syntax.addFlag("-vp", "-varPath", MSyntax::kString);
//   syntax.addFlag("-dn", "-desiredName", MSyntax::kString);
//   return syntax;
// }

// MStatus FabricDFGAddGetCommand::invoke(
//   MArgParser &argParser,
//   FabricCore::DFGExec &exec
//   )
// {
//   MString desiredNodeName;
//   if ( argParser.isFlagSet( "desiredName" ) )
//     desiredNodeName = argParser.flagArgumentString( "desiredName", 0 );

//   if ( !argParser.isFlagSet( "varPath" ) )
//   {
//     logError( "-varPath not provided." );
//     return MS::kFailure;
//   }
//   MString varPath = argParser.flagArgumentString( "varPath", 0 );

//   FTL::CStrRef nodeName =
//     exec.addGet(
//       desiredNodeName.asChar(),
//       varPath.asChar()
//       );
//   incCoreUndoCount();

//   setPos( argParser, exec, nodeName );

//   setResult( MString( nodeName.c_str() ) );
//   return MS::kSuccess;
// }

// // FabricDFGAddSetCommand

// MSyntax FabricDFGAddSetCommand::newSyntax()
// {
//   MSyntax syntax;
//   Parent::addSyntax( syntax );
//   syntax.addFlag("-vp", "-varPath", MSyntax::kString);
//   syntax.addFlag("-dn", "-desiredName", MSyntax::kString);
//   return syntax;
// }

// MStatus FabricDFGAddSetCommand::invoke(
//   MArgParser &argParser,
//   FabricCore::DFGExec &exec
//   )
// {
//   MString desiredNodeName;
//   if ( argParser.isFlagSet( "desiredName" ) )
//     desiredNodeName = argParser.flagArgumentString( "desiredName", 0 );

//   if ( !argParser.isFlagSet( "varPath" ) )
//   {
//     logError( "-varPath not provided." );
//     return MS::kFailure;
//   }
//   MString varPath = argParser.flagArgumentString( "varPath", 0 );

//   FTL::CStrRef nodeName =
//     exec.addSet(
//       desiredNodeName.asChar(),
//       varPath.asChar()
//       );
//   incCoreUndoCount();

//   setPos( argParser, exec, nodeName );

//   setResult( MString( nodeName.c_str() ) );
//   return MS::kSuccess;
// }

// FabricDFGConnectCommand

void FabricDFGConnectCommand::addSyntax( MSyntax &syntax )
{
  Parent::addSyntax( syntax );
  syntax.addFlag("-sp", "-srcPort", MSyntax::kString);
  syntax.addFlag("-dp", "-dstPort", MSyntax::kString);
}

void FabricDFGConnectCommand::getArgs(
  MArgParser &argParser,
  Args &args
  )
{
  Parent::getArgs( argParser, args );

  if ( !argParser.isFlagSet( "srcPort" ) )
    throw ArgException( MS::kFailure, "-srcPort not provided." );
  args.srcPort = argParser.flagArgumentString( "srcPort", 0 ).asChar();

  if ( !argParser.isFlagSet( "dstPort" ) )
    throw ArgException( MS::kFailure, "-dstPort not provided." );
  args.dstPort = argParser.flagArgumentString( "dstPort", 0 ).asChar();
}

FabricUI::DFG::DFGUICmd *FabricDFGConnectCommand::executeDFGUICmd(
  MArgParser &argParser
  )
{
  Args args;
  getArgs( argParser, args );

  FabricUI::DFG::DFGUICmd_Connect *cmd =
    new FabricUI::DFG::DFGUICmd_Connect(
      args.binding,
      args.execPath,
      args.exec,
      args.srcPort,
      args.dstPort
      );
  cmd->doit();
  return cmd;
}

// FabricDFGMoveNodesCommand

void FabricDFGMoveNodesCommand::addSyntax( MSyntax &syntax )
{
  Parent::addSyntax( syntax );
  syntax.addFlag("-n", "-node", MSyntax::kString);
  syntax.makeFlagMultiUse("-node");
  syntax.addFlag("-xy", "-position", MSyntax::kDouble, MSyntax::kDouble);
  syntax.makeFlagMultiUse("-position");
}

void FabricDFGMoveNodesCommand::getArgs(
  MArgParser &argParser,
  Args &args
  )
{
  Parent::getArgs( argParser, args );

  for ( unsigned i = 0; ; ++i )
  {
    MArgList argList;
    if ( argParser.getFlagArgumentList(
      "node", i, argList
      ) != MS::kSuccess )
      break;
    MString nodeName;
    if ( argList.get( 0, nodeName ) != MS::kSuccess )
      throw ArgException( MS::kFailure, "-node not a string" );
    args.nodeNames.push_back( nodeName.asChar() );
  }

  for ( unsigned i = 0; ; ++i )
  {
    MArgList argList;
    if ( argParser.getFlagArgumentList(
      "position", i, argList
      ) != MS::kSuccess )
      break;
    args.poss.push_back(
      QPointF(
        argList.asDouble( 0 ),
        argList.asDouble( 1 )
        )
      );
  }
}

FabricUI::DFG::DFGUICmd *FabricDFGMoveNodesCommand::executeDFGUICmd(
  MArgParser &argParser
  )
{
  Args args;
  getArgs( argParser, args );

  FabricUI::DFG::DFGUICmd_MoveNodes *cmd =
    new FabricUI::DFG::DFGUICmd_MoveNodes(
      args.binding,
      args.execPath,
      args.exec,
      args.nodeNames,
      args.poss
      );
  cmd->doit();
  return cmd;
}

// FabricDFGAddPortCommand

void FabricDFGAddPortCommand::addSyntax( MSyntax &syntax )
{
  Parent::addSyntax( syntax );
  syntax.addFlag("-dpn", "-desiredPortName", MSyntax::kString);
  syntax.addFlag("-pt", "-portType", MSyntax::kString);
  syntax.addFlag("-ts", "-typeSpec", MSyntax::kString);
  syntax.addFlag("-ptc", "-portToConnect", MSyntax::kString);
}

void FabricDFGAddPortCommand::getArgs(
  MArgParser &argParser,
  Args &args
  )
{
  Parent::getArgs( argParser, args );

  if ( !argParser.isFlagSet( "desiredPortName" ) )
    throw ArgException( MS::kFailure, "-desiredPortName not provided." );
  args.desiredPortName = argParser.flagArgumentString( "desiredPortName", 0 ).asChar();

  if ( !argParser.isFlagSet( "portType" ) )
    throw ArgException( MS::kFailure, "-portType not provided." );
  MString portTypeString = argParser.flagArgumentString( "portType", 0 ).asChar();
  if ( portTypeString == "In" )
    args.portType = FabricCore::DFGPortType_In;
  else if ( portTypeString == "IO" )
    args.portType = FabricCore::DFGPortType_IO;
  else if ( portTypeString == "Out" )
    args.portType = FabricCore::DFGPortType_Out;
  else
    throw ArgException( MS::kFailure, "-portType value unrecognized" );

  if ( argParser.isFlagSet( "typeSpec" ) )
    args.typeSpec = argParser.flagArgumentString( "typeSpec", 0 ).asChar();

  if ( argParser.isFlagSet( "portToConnect" ) )
    args.portToConnectWith = argParser.flagArgumentString( "portToConnect", 0 ).asChar();
}

FabricUI::DFG::DFGUICmd *FabricDFGAddPortCommand::executeDFGUICmd(
  MArgParser &argParser
  )
{
  Args args;
  getArgs( argParser, args );

  FabricUI::DFG::DFGUICmd_AddPort *cmd =
    new FabricUI::DFG::DFGUICmd_AddPort(
      args.binding,
      args.execPath,
      args.exec,
      args.desiredPortName,
      args.portType,
      args.typeSpec,
      args.portToConnectWith
      );
  cmd->doit();
  setResult( cmd->getActualPortName().c_str() );
  return cmd;
}

// FabricDFGSetArgTypeCommand

void FabricDFGSetArgTypeCommand::addSyntax( MSyntax &syntax )
{
  Parent::addSyntax( syntax );
  syntax.addFlag("-n", "-name", MSyntax::kString);
  syntax.addFlag("-t", "-type", MSyntax::kString);
}

void FabricDFGSetArgTypeCommand::getArgs(
  MArgParser &argParser,
  Args &args
  )
{
  Parent::getArgs( argParser, args );
  
  if ( !argParser.isFlagSet( "name" ) )
    throw ArgException( MS::kFailure, "-name not provided." );
  args.argName = argParser.flagArgumentString( "name", 0 ).asChar();

  if ( !argParser.isFlagSet( "type" ) )
    throw ArgException( MS::kFailure, "-type not provided." );
  args.typeName = argParser.flagArgumentString( "type", 0 ).asChar();
}

FabricUI::DFG::DFGUICmd *FabricDFGSetArgTypeCommand::executeDFGUICmd(
  MArgParser &argParser
  )
{
  Args args;
  getArgs( argParser, args );

  FabricUI::DFG::DFGUICmd_SetArgType *cmd =
    new FabricUI::DFG::DFGUICmd_SetArgType(
      args.binding,
      args.argName,
      args.typeName
      );
  cmd->doit();
  return cmd;
}

// // FabricDFGDisconnectCommand

// MSyntax FabricDFGDisconnectCommand::newSyntax()
// {
//   MSyntax syntax;
//   Parent::addSyntax( syntax );
//   syntax.addFlag("-sp", "-srcPort", MSyntax::kString);
//   syntax.addFlag("-dp", "-dstPort", MSyntax::kString);
//   return syntax;
// }

// MStatus FabricDFGDisconnectCommand::invoke(
//   MArgParser &argParser,
//   FabricCore::DFGExec &exec
//   )
// {
//   if ( !argParser.isFlagSet( "srcPort" ) )
//   {
//     logError( "-srcPort not provided." );
//     return MS::kFailure;
//   }
//   MString srcPort = argParser.flagArgumentString( "srcPort", 0 );

//   if ( !argParser.isFlagSet( "dstPort" ) )
//   {
//     logError( "-dstPort not provided." );
//     return MS::kFailure;
//   }
//   MString dstPort = argParser.flagArgumentString( "dstPort", 0 );

//   exec.disconnectFrom( srcPort.asChar(), dstPort.asChar() );
//   incCoreUndoCount();

//   return MS::kSuccess;
// }

// // FabricDFGRemoveNodesCommand

// MSyntax FabricDFGRemoveNodesCommand::newSyntax()
// {
//   MSyntax syntax;
//   Parent::addSyntax( syntax );
//   syntax.addFlag("-n", "-node", MSyntax::kString);
//   syntax.makeFlagMultiUse("-node");
//   return syntax;
// }

// MStatus FabricDFGRemoveNodesCommand::invoke(
//   MArgParser &argParser,
//   FabricCore::DFGExec &exec
//   )
// {
//   std::vector<std::string> nodes;

//   for ( unsigned i = 0; ; ++i )
//   {
//     MArgList argList;
//     if ( argParser.getFlagArgumentList(
//       "node", i, argList
//       ) != MS::kSuccess )
//       break;
//     MString node;
//     if ( argList.get( 0, node ) != MS::kSuccess )
//     {
//       logError( "-node not a string" );
//       return MS::kFailure;
//     }
//     nodes.push_back( node.asChar() );
//   }

//   for ( std::vector<std::string>::const_iterator it = nodes.begin();
//     it != nodes.end(); ++it )
//   {
//     exec.removeNode( it->c_str() );
//     incCoreUndoCount();
//   }

//   return MS::kSuccess;
// }
