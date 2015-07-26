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

// FabricDFGCoreCommand

void FabricDFGCoreCommand::AddSyntax( MSyntax &syntax )
{
  syntax.enableQuery(false);
  syntax.enableEdit(false);
}

MStatus FabricDFGCoreCommand::doIt( const MArgList &args )
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

MStatus FabricDFGCoreCommand::undoIt()
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

MStatus FabricDFGCoreCommand::redoIt()
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

void FabricDFGBindingCommand::AddSyntax( MSyntax &syntax )
{
  Parent::AddSyntax( syntax );
  syntax.addFlag( "-m", "-mayaNode", MSyntax::kString );
}

void FabricDFGBindingCommand::GetArgs(
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
    throw ArgException( MS::kNotFound, "-mayaNode '" + mayaNodeName + "' not found." );
  args.binding = interf->getDFGBinding();
}

// FabricDFGExecCommand

void FabricDFGExecCommand::AddSyntax( MSyntax &syntax )
{
  Parent::AddSyntax( syntax );
  syntax.addFlag( "-e", "-execPath", MSyntax::kString );
}

void FabricDFGExecCommand::GetArgs(
  MArgParser &argParser,
  Args &args
  )
{
  Parent::GetArgs( argParser, args );

  if ( !argParser.isFlagSet( "execPath" ) )
    throw ArgException( MS::kFailure, "-execPath not provided." );
  MString execPathMString = argParser.flagArgumentString( "execPath", 0 );
  args.execPath = execPathMString.asChar();
  args.exec = args.binding.getExec().getSubExec( execPathMString.asChar() );
}

// FabricDFGAddNodeCommand

void FabricDFGAddNodeCommand::AddSyntax( MSyntax &syntax )
{
  Parent::AddSyntax( syntax );
  syntax.addFlag( "-xy", "-position", MSyntax::kDouble, MSyntax::kDouble );
}

void FabricDFGAddNodeCommand::GetArgs( MArgParser &argParser, Args &args )
{
  Parent::GetArgs( argParser, args );

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

// FabricDFGAddBackDropCommand

void FabricDFGAddBackDropCommand::AddSyntax( MSyntax &syntax )
{
  Parent::AddSyntax( syntax );
  syntax.addFlag("-t", "-title", MSyntax::kString);
}

void FabricDFGAddBackDropCommand::GetArgs(
  MArgParser &argParser,
  Args &args
  )
{
  Parent::GetArgs( argParser, args );

  if ( !argParser.isFlagSet( "title" ) )
    throw ArgException( MS::kFailure, "-title not provided." );
  args.title = argParser.flagArgumentString( "title", 0 ).asChar();
}

FabricUI::DFG::DFGUICmd *FabricDFGAddBackDropCommand::executeDFGUICmd(
  MArgParser &argParser
  )
{
  Args args;
  GetArgs( argParser, args );

  FabricUI::DFG::DFGUICmd_AddBackDrop *cmd =
    new FabricUI::DFG::DFGUICmd_AddBackDrop(
      args.binding,
      args.execPath,
      args.exec,
      args.title,
      args.pos
      );
  cmd->doit();
  setResult( cmd->getActualNodeName().c_str() );
  return cmd;
}

// FabricDFGInstPresetCommand

void FabricDFGInstPresetCommand::AddSyntax( MSyntax &syntax )
{
  Parent::AddSyntax( syntax );
  syntax.addFlag("-p", "-presetPath", MSyntax::kString);
}

void FabricDFGInstPresetCommand::GetArgs(
  MArgParser &argParser,
  Args &args
  )
{
  Parent::GetArgs( argParser, args );

  if ( !argParser.isFlagSet( "presetPath" ) )
    throw ArgException( MS::kFailure, "-presetPath not provided." );
  args.presetPath = argParser.flagArgumentString( "presetPath", 0 ).asChar();
}

FabricUI::DFG::DFGUICmd *FabricDFGInstPresetCommand::executeDFGUICmd(
  MArgParser &argParser
  )
{
  Args args;
  GetArgs( argParser, args );

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

// FabricDFGAddGraphCommand

void FabricDFGAddGraphCommand::AddSyntax( MSyntax &syntax )
{
  Parent::AddSyntax( syntax );
  syntax.addFlag("-t", "-title", MSyntax::kString);
}

void FabricDFGAddGraphCommand::GetArgs(
  MArgParser &argParser,
  Args &args
  )
{
  Parent::GetArgs( argParser, args );

  if ( argParser.isFlagSet( "title" ) )
    args.title = argParser.flagArgumentString( "title", 0 ).asChar();
}

FabricUI::DFG::DFGUICmd *FabricDFGAddGraphCommand::executeDFGUICmd(
  MArgParser &argParser
  )
{
  Args args;
  GetArgs( argParser, args );

  FabricUI::DFG::DFGUICmd_AddGraph *cmd =
    new FabricUI::DFG::DFGUICmd_AddGraph(
      args.binding,
      args.execPath,
      args.exec,
      args.title,
      args.pos
      );
  cmd->doit();
  setResult( cmd->getActualNodeName().c_str() );
  return cmd;
}

// FabricDFGAddFuncCommand

void FabricDFGAddFuncCommand::AddSyntax( MSyntax &syntax )
{
  Parent::AddSyntax( syntax );
  syntax.addFlag("-t", "-title", MSyntax::kString);
  syntax.addFlag("-c", "-code", MSyntax::kString);
}

void FabricDFGAddFuncCommand::GetArgs(
  MArgParser &argParser,
  Args &args
  )
{
  Parent::GetArgs( argParser, args );

  if ( argParser.isFlagSet( "title" ) )
    args.title = argParser.flagArgumentString( "title", 0 ).asChar();

  if ( argParser.isFlagSet( "code" ) )
    args.code = argParser.flagArgumentString( "code", 0 ).asChar();
}

FabricUI::DFG::DFGUICmd *FabricDFGAddFuncCommand::executeDFGUICmd(
  MArgParser &argParser
  )
{
  Args args;
  GetArgs( argParser, args );

  FabricUI::DFG::DFGUICmd_AddFunc *cmd =
    new FabricUI::DFG::DFGUICmd_AddFunc(
      args.binding,
      args.execPath,
      args.exec,
      args.title,
      args.code,
      args.pos
      );
  cmd->doit();
  setResult( cmd->getActualNodeName().c_str() );
  return cmd;
}

// FabricDFGAddVarCommand

void FabricDFGAddVarCommand::AddSyntax( MSyntax &syntax )
{
  Parent::AddSyntax( syntax );
  syntax.addFlag("-d", "-desiredNodeName", MSyntax::kString);
  syntax.addFlag("-t", "-type", MSyntax::kString);
  syntax.addFlag("-ed", "-extDep", MSyntax::kString);
}

void FabricDFGAddVarCommand::GetArgs(
  MArgParser &argParser,
  Args &args
  )
{
  Parent::GetArgs( argParser, args );

  if ( argParser.isFlagSet( "desiredNodeName" ) )
    args.desiredName = argParser.flagArgumentString( "desiredNodeName", 0 ).asChar();

  if ( argParser.isFlagSet( "type" ) )
    args.type = argParser.flagArgumentString( "type", 0 ).asChar();

  if ( argParser.isFlagSet( "extDep" ) )
    args.extDep = argParser.flagArgumentString( "extDep", 0 ).asChar();
}

FabricUI::DFG::DFGUICmd *FabricDFGAddVarCommand::executeDFGUICmd(
  MArgParser &argParser
  )
{
  Args args;
  GetArgs( argParser, args );

  FabricUI::DFG::DFGUICmd_AddVar *cmd =
    new FabricUI::DFG::DFGUICmd_AddVar(
      args.binding,
      args.execPath,
      args.exec,
      args.desiredName,
      args.type,
      args.extDep,
      args.pos
      );
  cmd->doit();
  setResult( cmd->getActualNodeName().c_str() );
  return cmd;
}

// FabricDFGAddRefCommand

void FabricDFGAddRefCommand::AddSyntax( MSyntax &syntax )
{
  Parent::AddSyntax( syntax );
  syntax.addFlag("-d", "-desiredNodeName", MSyntax::kString);
  syntax.addFlag("-v", "-varPath", MSyntax::kString);
}

void FabricDFGAddRefCommand::GetArgs(
  MArgParser &argParser,
  Args &args
  )
{
  Parent::GetArgs( argParser, args );

  if ( argParser.isFlagSet( "desiredNodeName" ) )
    args.desiredName = argParser.flagArgumentString( "desiredNodeName", 0 ).asChar();

  if ( argParser.isFlagSet( "varPath" ) )
    args.varPath = argParser.flagArgumentString( "varPath", 0 ).asChar();
}

// FabricDFGAddGetCommand

FabricUI::DFG::DFGUICmd *FabricDFGAddGetCommand::executeDFGUICmd(
  MArgParser &argParser
  )
{
  Args args;
  GetArgs( argParser, args );

  FabricUI::DFG::DFGUICmd_AddGet *cmd =
    new FabricUI::DFG::DFGUICmd_AddGet(
      args.binding,
      args.execPath,
      args.exec,
      args.desiredName,
      args.varPath,
      args.pos
      );
  cmd->doit();
  setResult( cmd->getActualNodeName().c_str() );
  return cmd;
}

// FabricDFGAddSetCommand

FabricUI::DFG::DFGUICmd *FabricDFGAddSetCommand::executeDFGUICmd(
  MArgParser &argParser
  )
{
  Args args;
  GetArgs( argParser, args );

  FabricUI::DFG::DFGUICmd_AddSet *cmd =
    new FabricUI::DFG::DFGUICmd_AddSet(
      args.binding,
      args.execPath,
      args.exec,
      args.desiredName,
      args.varPath,
      args.pos
      );
  cmd->doit();
  setResult( cmd->getActualNodeName().c_str() );
  return cmd;
}

// // FabricDFGAddVarCommand

// MSyntax FabricDFGAddVarCommand::newSyntax()
// {
//   MSyntax syntax;
//   Parent::AddSyntax( syntax );
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
//   Parent::AddSyntax( syntax );
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
//   Parent::AddSyntax( syntax );
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

// FabricDFGCnxnCommand

void FabricDFGCnxnCommand::AddSyntax( MSyntax &syntax )
{
  Parent::AddSyntax( syntax );
  syntax.addFlag("-s", "-srcPortPath", MSyntax::kString);
  syntax.addFlag("-d", "-dstPortPath", MSyntax::kString);
}

void FabricDFGCnxnCommand::GetArgs(
  MArgParser &argParser,
  Args &args
  )
{
  Parent::GetArgs( argParser, args );

  if ( !argParser.isFlagSet( "srcPortPath" ) )
    throw ArgException( MS::kFailure, "-srcPortPath not provided." );
  args.srcPort = argParser.flagArgumentString( "srcPortPath", 0 ).asChar();

  if ( !argParser.isFlagSet( "dstPortPath" ) )
    throw ArgException( MS::kFailure, "-dstPortPath not provided." );
  args.dstPort = argParser.flagArgumentString( "dstPortPath", 0 ).asChar();
}

// FabricDFGConnectCommand

FabricUI::DFG::DFGUICmd *FabricDFGConnectCommand::executeDFGUICmd(
  MArgParser &argParser
  )
{
  Args args;
  GetArgs( argParser, args );

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

// FabricDFGDisconnectCommand

FabricUI::DFG::DFGUICmd *FabricDFGDisconnectCommand::executeDFGUICmd(
  MArgParser &argParser
  )
{
  Args args;
  GetArgs( argParser, args );

  FabricUI::DFG::DFGUICmd_Disconnect *cmd =
    new FabricUI::DFG::DFGUICmd_Disconnect(
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

void FabricDFGMoveNodesCommand::AddSyntax( MSyntax &syntax )
{
  Parent::AddSyntax( syntax );
  syntax.addFlag("-n", "-nodeName", MSyntax::kString);
  syntax.makeFlagMultiUse("-nodeName");
  syntax.addFlag("-xy", "-position", MSyntax::kDouble, MSyntax::kDouble);
  syntax.makeFlagMultiUse("-position");
}

void FabricDFGMoveNodesCommand::GetArgs(
  MArgParser &argParser,
  Args &args
  )
{
  Parent::GetArgs( argParser, args );

  for ( unsigned i = 0; ; ++i )
  {
    MArgList argList;
    if ( argParser.getFlagArgumentList(
      "nodeName", i, argList
      ) != MS::kSuccess )
      break;
    MString nodeName;
    if ( argList.get( 0, nodeName ) != MS::kSuccess )
      throw ArgException( MS::kFailure, "-nodeName not a string" );
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
  GetArgs( argParser, args );

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

// FabricDFGImplodeNodesCommand

void FabricDFGImplodeNodesCommand::AddSyntax( MSyntax &syntax )
{
  Parent::AddSyntax( syntax );
  syntax.addFlag("-n", "-nodeName", MSyntax::kString);
  syntax.makeFlagMultiUse("-nodeName");
  syntax.addFlag("-d", "-desiredImplodedNodeName", MSyntax::kString);
}

void FabricDFGImplodeNodesCommand::GetArgs(
  MArgParser &argParser,
  Args &args
  )
{
  Parent::GetArgs( argParser, args );

  for ( unsigned i = 0; ; ++i )
  {
    MArgList argList;
    if ( argParser.getFlagArgumentList(
      "nodeName", i, argList
      ) != MS::kSuccess )
      break;
    MString node;
    if ( argList.get( 0, node ) != MS::kSuccess )
      throw ArgException( MS::kFailure, "-nodeName not a string" );
    args.nodes.push_back( node.asChar() );
  }

  if ( argParser.isFlagSet( "desiredImplodedNodeName" ) )
    args.desiredName =
      argParser.flagArgumentString( "desiredImplodedNodeName", 0 ).asChar();;
}

FabricUI::DFG::DFGUICmd *FabricDFGImplodeNodesCommand::executeDFGUICmd(
  MArgParser &argParser
  )
{
  Args args;
  GetArgs( argParser, args );

  FabricUI::DFG::DFGUICmd_ImplodeNodes *cmd =
    new FabricUI::DFG::DFGUICmd_ImplodeNodes(
      args.binding,
      args.execPath,
      args.exec,
      args.desiredName,
      args.nodes
      );
  cmd->doit();
  setResult( cmd->getActualImplodedNodeName().c_str() );
  return cmd;
}

// FabricDFGExplodeNodeCommand

void FabricDFGExplodeNodeCommand::AddSyntax( MSyntax &syntax )
{
  Parent::AddSyntax( syntax );
  syntax.addFlag("-n", "-nodeName", MSyntax::kString);
}

void FabricDFGExplodeNodeCommand::GetArgs(
  MArgParser &argParser,
  Args &args
  )
{
  Parent::GetArgs( argParser, args );

  if ( !argParser.isFlagSet( "nodeName" ) )
    throw ArgException( MS::kFailure, "-nodeName not provided." );
  args.node = argParser.flagArgumentString( "nodeName", 0 ).asChar();
}

FabricUI::DFG::DFGUICmd *FabricDFGExplodeNodeCommand::executeDFGUICmd(
  MArgParser &argParser
  )
{
  Args args;
  GetArgs( argParser, args );

  FabricUI::DFG::DFGUICmd_ExplodeNode *cmd =
    new FabricUI::DFG::DFGUICmd_ExplodeNode(
      args.binding,
      args.execPath,
      args.exec,
      args.node
      );
  cmd->doit();

  FTL::ArrayRef<std::string> explodedNodeNames =
    cmd->getExplodedNodeNames();

  MStringArray mExplodedNodeNames;
  mExplodedNodeNames.setSizeIncrement( explodedNodeNames.size() );
  for ( FTL::ArrayRef<std::string>::IT it = explodedNodeNames.begin();
    it != explodedNodeNames.end(); ++it )
    mExplodedNodeNames.append( it->c_str() );
  setResult( mExplodedNodeNames );

  return cmd;
}

// FabricDFGPasteCommand

void FabricDFGPasteCommand::AddSyntax( MSyntax &syntax )
{
  Parent::AddSyntax( syntax );
  syntax.addFlag("-t", "-text", MSyntax::kString);
  syntax.addFlag( "-xy", "-position", MSyntax::kDouble, MSyntax::kDouble );
}

void FabricDFGPasteCommand::GetArgs(
  MArgParser &argParser,
  Args &args
  )
{
  Parent::GetArgs( argParser, args );

  if ( !argParser.isFlagSet( "text" ) )
    throw ArgException( MS::kFailure, "-text not provided." );
  args.text = argParser.flagArgumentString( "text", 0 ).asChar();

  if ( argParser.isFlagSet("position") )
  {
    args.xy =
      QPointF(
        argParser.flagArgumentDouble("position", 0),
        argParser.flagArgumentDouble("position", 1)
        );
  }
  else args.xy = QPointF( 0, 0 );
}

FabricUI::DFG::DFGUICmd *FabricDFGPasteCommand::executeDFGUICmd(
  MArgParser &argParser
  )
{
  Args args;
  GetArgs( argParser, args );

  FabricUI::DFG::DFGUICmd_Paste *cmd =
    new FabricUI::DFG::DFGUICmd_Paste(
      args.binding,
      args.execPath,
      args.exec,
      args.text,
      args.xy
      );
  cmd->doit();

  FTL::ArrayRef<std::string> pastedNodeNames =
    cmd->getPastedNodeNames();

  MStringArray mPastedNodeNames;
  mPastedNodeNames.setSizeIncrement( pastedNodeNames.size() );
  for ( FTL::ArrayRef<std::string>::IT it = pastedNodeNames.begin();
    it != pastedNodeNames.end(); ++it )
    mPastedNodeNames.append( it->c_str() );
  setResult( mPastedNodeNames );

  return cmd;
}

// FabricDFGResizeBackDropCommand

void FabricDFGResizeBackDropCommand::AddSyntax( MSyntax &syntax )
{
  Parent::AddSyntax( syntax );
  syntax.addFlag("-n", "-nodeName", MSyntax::kString);
  syntax.addFlag( "-xy", "-position", MSyntax::kDouble, MSyntax::kDouble );
  syntax.addFlag( "-wh", "-size", MSyntax::kDouble, MSyntax::kDouble );
}

void FabricDFGResizeBackDropCommand::GetArgs(
  MArgParser &argParser,
  Args &args
  )
{
  Parent::GetArgs( argParser, args );

  if ( !argParser.isFlagSet( "nodeName" ) )
    throw ArgException( MS::kFailure, "-nodeName not provided." );
  args.nodeName = argParser.flagArgumentString( "nodeName", 0 ).asChar();

  if ( !argParser.isFlagSet("position") )
    throw ArgException( MS::kFailure, "-position not provided." );
  args.xy =
    QPointF(
      argParser.flagArgumentDouble("position", 0),
      argParser.flagArgumentDouble("position", 1)
      );

  if ( !argParser.isFlagSet("size") )
    throw ArgException( MS::kFailure, "-size not provided." );
  args.wh =
    QSizeF(
      argParser.flagArgumentDouble("size", 0),
      argParser.flagArgumentDouble("size", 1)
      );
}

FabricUI::DFG::DFGUICmd *FabricDFGResizeBackDropCommand::executeDFGUICmd(
  MArgParser &argParser
  )
{
  Args args;
  GetArgs( argParser, args );

  FabricUI::DFG::DFGUICmd_ResizeBackDrop *cmd =
    new FabricUI::DFG::DFGUICmd_ResizeBackDrop(
      args.binding,
      args.execPath,
      args.exec,
      args.nodeName,
      args.xy,
      args.wh
      );
  cmd->doit();
  return cmd;
}

// FabricDFGRemoveNodesCommand

void FabricDFGRemoveNodesCommand::AddSyntax( MSyntax &syntax )
{
  Parent::AddSyntax( syntax );
  syntax.addFlag("-n", "-nodeName", MSyntax::kString);
  syntax.makeFlagMultiUse("-nodeName");
}

void FabricDFGRemoveNodesCommand::GetArgs(
  MArgParser &argParser,
  Args &args
  )
{
  Parent::GetArgs( argParser, args );

  for ( unsigned i = 0; ; ++i )
  {
    MArgList argList;
    if ( argParser.getFlagArgumentList(
      "nodeName", i, argList
      ) != MS::kSuccess )
      break;
    MString nodeName;
    if ( argList.get( 0, nodeName ) != MS::kSuccess )
      throw ArgException( MS::kFailure, "-nodeName not a string" );
    args.nodeNames.push_back( nodeName.asChar() );
  }
}

FabricUI::DFG::DFGUICmd *FabricDFGRemoveNodesCommand::executeDFGUICmd(
  MArgParser &argParser
  )
{
  Args args;
  GetArgs( argParser, args );

  FabricUI::DFG::DFGUICmd_RemoveNodes *cmd =
    new FabricUI::DFG::DFGUICmd_RemoveNodes(
      args.binding,
      args.execPath,
      args.exec,
      args.nodeNames
      );
  cmd->doit();
  return cmd;
}

// FabricDFGAddPortCommand

void FabricDFGAddPortCommand::AddSyntax( MSyntax &syntax )
{
  Parent::AddSyntax( syntax );
  syntax.addFlag("-d", "-desiredPortName", MSyntax::kString);
  syntax.addFlag("-p", "-portType", MSyntax::kString);
  syntax.addFlag("-t", "-typeSpec", MSyntax::kString);
  syntax.addFlag("-c", "-connectToPortPath", MSyntax::kString);
}

void FabricDFGAddPortCommand::GetArgs(
  MArgParser &argParser,
  Args &args
  )
{
  Parent::GetArgs( argParser, args );

  if ( !argParser.isFlagSet( "desiredPortName" ) )
    throw ArgException( MS::kFailure, "-desiredPortName not provided." );
  args.desiredPortName = argParser.flagArgumentString( "desiredPortName", 0 ).asChar();

  if ( !argParser.isFlagSet( "portType" ) )
    throw ArgException( MS::kFailure, "-portType not provided." );
  MString portTypeString = argParser.flagArgumentString( "portType", 0 ).asChar();
  portTypeString.toLowerCase(); 
  if ( portTypeString == "in" )
    args.portType = FabricCore::DFGPortType_In;
  else if ( portTypeString == "io" )
    args.portType = FabricCore::DFGPortType_IO;
  else if ( portTypeString == "out" )
    args.portType = FabricCore::DFGPortType_Out;
  else
    throw ArgException( MS::kFailure, "-portType value unrecognized" );

  if ( argParser.isFlagSet( "typeSpec" ) )
    args.typeSpec = argParser.flagArgumentString( "typeSpec", 0 ).asChar();

  if ( argParser.isFlagSet( "connectToPortPath" ) )
    args.portToConnectWith = argParser.flagArgumentString( "connectToPortPath", 0 ).asChar();
}

FabricUI::DFG::DFGUICmd *FabricDFGAddPortCommand::executeDFGUICmd(
  MArgParser &argParser
  )
{
  Args args;
  GetArgs( argParser, args );

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

void FabricDFGSetArgTypeCommand::AddSyntax( MSyntax &syntax )
{
  Parent::AddSyntax( syntax );
  syntax.addFlag("-a", "-argument", MSyntax::kString);
  syntax.addFlag("-t", "-type", MSyntax::kString);
}

void FabricDFGSetArgTypeCommand::GetArgs(
  MArgParser &argParser,
  Args &args
  )
{
  Parent::GetArgs( argParser, args );

  if ( !argParser.isFlagSet( "argument" ) )
    throw ArgException( MS::kFailure, "-argument not provided." );
  args.argName = argParser.flagArgumentString( "argument", 0 ).asChar();

  if ( !argParser.isFlagSet( "type" ) )
    throw ArgException( MS::kFailure, "-type not provided." );
  args.typeName = argParser.flagArgumentString( "type", 0 ).asChar();
}

FabricUI::DFG::DFGUICmd *FabricDFGSetArgTypeCommand::executeDFGUICmd(
  MArgParser &argParser
  )
{
  Args args;
  GetArgs( argParser, args );

  FabricUI::DFG::DFGUICmd_SetArgType *cmd =
    new FabricUI::DFG::DFGUICmd_SetArgType(
      args.binding,
      args.argName,
      args.typeName
      );
  cmd->doit();
  return cmd;
}

// FabricDFGSetArgValueCommand

void FabricDFGSetArgValueCommand::AddSyntax( MSyntax &syntax )
{
  Parent::AddSyntax( syntax );
  syntax.addFlag("-a", "-argument", MSyntax::kString);
  syntax.addFlag("-t", "-type", MSyntax::kString);
  syntax.addFlag("-v", "-value", MSyntax::kString);
}

void FabricDFGSetArgValueCommand::GetArgs(
  MArgParser &argParser,
  Args &args
  )
{
  Parent::GetArgs( argParser, args );

  if ( !argParser.isFlagSet( "argument" ) )
    throw ArgException( MS::kFailure, "-argument not provided." );
  args.argName = argParser.flagArgumentString( "argument", 0 ).asChar();

  if ( !argParser.isFlagSet( "type" ) )
    throw ArgException( MS::kFailure, "-type not provided." );
  MString type = argParser.flagArgumentString( "type", 0 ).asChar();

  if ( !argParser.isFlagSet( "value" ) )
    throw ArgException( MS::kFailure, "-value not provided." );
  MString valueJSON = argParser.flagArgumentString( "value", 0 ).asChar();
  FabricCore::DFGHost host = args.binding.getHost();
  FabricCore::Context context = host.getContext();
  args.value = FabricCore::RTVal::Construct( context, type.asChar(), 0, NULL );
  args.value.setJSON( valueJSON.asChar() );
}

FabricUI::DFG::DFGUICmd *FabricDFGSetArgValueCommand::executeDFGUICmd(
  MArgParser &argParser
  )
{
  Args args;
  GetArgs( argParser, args );

  FabricUI::DFG::DFGUICmd_SetArgValue *cmd =
    new FabricUI::DFG::DFGUICmd_SetArgValue(
      args.binding,
      args.argName,
      args.value
      );
  cmd->doit();
  return cmd;
}

// FabricDFGSetPortDefaultValueCommand

void FabricDFGSetPortDefaultValueCommand::AddSyntax( MSyntax &syntax )
{
  Parent::AddSyntax( syntax );
  syntax.addFlag("-p", "-portPath", MSyntax::kString);
  syntax.addFlag("-t", "-type", MSyntax::kString);
  syntax.addFlag("-v", "-value", MSyntax::kString);
}

void FabricDFGSetPortDefaultValueCommand::GetArgs(
  MArgParser &argParser,
  Args &args
  )
{
  Parent::GetArgs( argParser, args );

  if ( !argParser.isFlagSet( "portPath" ) )
    throw ArgException( MS::kFailure, "-portPath not provided." );
  args.portPath = argParser.flagArgumentString( "portPath", 0 ).asChar();

  if ( !argParser.isFlagSet( "type" ) )
    throw ArgException( MS::kFailure, "-type not provided." );
  MString type = argParser.flagArgumentString( "type", 0 ).asChar();

  if ( !argParser.isFlagSet( "value" ) )
    throw ArgException( MS::kFailure, "-value not provided." );
  MString valueJSON = argParser.flagArgumentString( "value", 0 ).asChar();
  FabricCore::DFGHost host = args.binding.getHost();
  FabricCore::Context context = host.getContext();
  args.value = FabricCore::RTVal::Construct( context, type.asChar(), 0, NULL );
  args.value.setJSON( valueJSON.asChar() );
}

FabricUI::DFG::DFGUICmd *FabricDFGSetPortDefaultValueCommand::executeDFGUICmd(
  MArgParser &argParser
  )
{
  Args args;
  GetArgs( argParser, args );

  FabricUI::DFG::DFGUICmd_SetPortDefaultValue *cmd =
    new FabricUI::DFG::DFGUICmd_SetPortDefaultValue(
      args.binding,
      args.execPath,
      args.exec,
      args.portPath,
      args.value
      );
  cmd->doit();
  return cmd;
}

// FabricDFGSetNodeTitleCommand

void FabricDFGSetNodeTitleCommand::AddSyntax( MSyntax &syntax )
{
  Parent::AddSyntax( syntax );
  syntax.addFlag("-n", "-nodeName", MSyntax::kString);
  syntax.addFlag("-t", "-title", MSyntax::kString);
}

void FabricDFGSetNodeTitleCommand::GetArgs(
  MArgParser &argParser,
  Args &args
  )
{
  Parent::GetArgs( argParser, args );

  if ( !argParser.isFlagSet( "nodeName" ) )
    throw ArgException( MS::kFailure, "-nodeName not provided." );
  args.nodeName = argParser.flagArgumentString( "nodeName", 0 ).asChar();

  if ( !argParser.isFlagSet( "title" ) )
    throw ArgException( MS::kFailure, "-title not provided." );
  args.title = argParser.flagArgumentString( "title", 0 ).asChar();
}

FabricUI::DFG::DFGUICmd *FabricDFGSetNodeTitleCommand::executeDFGUICmd(
  MArgParser &argParser
  )
{
  Args args;
  GetArgs( argParser, args );

  FabricUI::DFG::DFGUICmd_SetNodeTitle *cmd =
    new FabricUI::DFG::DFGUICmd_SetNodeTitle(
      args.binding,
      args.execPath,
      args.exec,
      args.nodeName,
      args.title
      );
  cmd->doit();
  return cmd;
}

// FabricDFGSetNodeCommentCommand

void FabricDFGSetNodeCommentCommand::AddSyntax( MSyntax &syntax )
{
  Parent::AddSyntax( syntax );
  syntax.addFlag("-n", "-nodeName", MSyntax::kString);
  syntax.addFlag("-c", "-comment", MSyntax::kString);
}

void FabricDFGSetNodeCommentCommand::GetArgs(
  MArgParser &argParser,
  Args &args
  )
{
  Parent::GetArgs( argParser, args );

  if ( !argParser.isFlagSet( "nodeName" ) )
    throw ArgException( MS::kFailure, "-nodeName not provided." );
  args.nodeName = argParser.flagArgumentString( "nodeName", 0 ).asChar();

  if ( !argParser.isFlagSet( "comment" ) )
    throw ArgException( MS::kFailure, "-comment not provided." );
  args.comment = argParser.flagArgumentString( "comment", 0 ).asChar();
}

FabricUI::DFG::DFGUICmd *FabricDFGSetNodeCommentCommand::executeDFGUICmd(
  MArgParser &argParser
  )
{
  Args args;
  GetArgs( argParser, args );

  FabricUI::DFG::DFGUICmd_SetNodeComment *cmd =
    new FabricUI::DFG::DFGUICmd_SetNodeComment(
      args.binding,
      args.execPath,
      args.exec,
      args.nodeName,
      args.comment
      );
  cmd->doit();
  return cmd;
}

// FabricDFGSetRefVarPathCommand

void FabricDFGSetRefVarPathCommand::AddSyntax( MSyntax &syntax )
{
  Parent::AddSyntax( syntax );
  syntax.addFlag("-r", "-refName", MSyntax::kString);
  syntax.addFlag("-v", "-varPath", MSyntax::kString);
}

void FabricDFGSetRefVarPathCommand::GetArgs(
  MArgParser &argParser,
  Args &args
  )
{
  Parent::GetArgs( argParser, args );

  if ( !argParser.isFlagSet( "refName" ) )
    throw ArgException( MS::kFailure, "-refName not provided." );
  args.refName = argParser.flagArgumentString( "refName", 0 ).asChar();

  if ( !argParser.isFlagSet( "varPath" ) )
    throw ArgException( MS::kFailure, "-varPath not provided." );
  args.varPath = argParser.flagArgumentString( "varPath", 0 ).asChar();
}

FabricUI::DFG::DFGUICmd *FabricDFGSetRefVarPathCommand::executeDFGUICmd(
  MArgParser &argParser
  )
{
  Args args;
  GetArgs( argParser, args );

  FabricUI::DFG::DFGUICmd_SetRefVarPath *cmd =
    new FabricUI::DFG::DFGUICmd_SetRefVarPath(
      args.binding,
      args.execPath,
      args.exec,
      args.refName,
      args.varPath
      );
  cmd->doit();
  return cmd;
}

// FabricDFGRenamePortCommand

void FabricDFGRenamePortCommand::AddSyntax( MSyntax &syntax )
{
  Parent::AddSyntax( syntax );
  syntax.addFlag("-n", "-oldPortName", MSyntax::kString);
  syntax.addFlag("-d", "-desiredNewPortName", MSyntax::kString);
}

void FabricDFGRenamePortCommand::GetArgs(
  MArgParser &argParser,
  Args &args
  )
{
  Parent::GetArgs( argParser, args );

  if ( !argParser.isFlagSet( "oldPortName" ) )
    throw ArgException( MS::kFailure, "-oldPortName not provided." );
  args.oldPortName = argParser.flagArgumentString( "oldPortName", 0 ).asChar();

  if ( !argParser.isFlagSet( "desiredNewPortName" ) )
    throw ArgException( MS::kFailure, "-desiredNewPortName not provided." );
  args.desiredNewPortName = argParser.flagArgumentString( "desiredNewPortName", 0 ).asChar();
}

FabricUI::DFG::DFGUICmd *FabricDFGRenamePortCommand::executeDFGUICmd(
  MArgParser &argParser
  )
{
  Args args;
  GetArgs( argParser, args );

  FabricUI::DFG::DFGUICmd_RenamePort *cmd =
    new FabricUI::DFG::DFGUICmd_RenamePort(
      args.binding,
      args.execPath,
      args.exec,
      args.oldPortName,
      args.desiredNewPortName
      );
  cmd->doit();
  setResult( cmd->getActualNewPortName().c_str() );
  return cmd;
}

// FabricDFGRemovePortCommand

void FabricDFGRemovePortCommand::AddSyntax( MSyntax &syntax )
{
  Parent::AddSyntax( syntax );
  syntax.addFlag("-p", "-portName", MSyntax::kString);
}

void FabricDFGRemovePortCommand::GetArgs(
  MArgParser &argParser,
  Args &args
  )
{
  Parent::GetArgs( argParser, args );

  if ( !argParser.isFlagSet( "portName" ) )
    throw ArgException( MS::kFailure, "-portName not provided." );
  args.portName = argParser.flagArgumentString( "portName", 0 ).asChar();
}

FabricUI::DFG::DFGUICmd *FabricDFGRemovePortCommand::executeDFGUICmd(
  MArgParser &argParser
  )
{
  Args args;
  GetArgs( argParser, args );

  FabricUI::DFG::DFGUICmd_RemovePort *cmd =
    new FabricUI::DFG::DFGUICmd_RemovePort(
      args.binding,
      args.execPath,
      args.exec,
      args.portName
      );
  cmd->doit();
  return cmd;
}

// FabricDFGSetCodeCommand

void FabricDFGSetCodeCommand::AddSyntax( MSyntax &syntax )
{
  Parent::AddSyntax( syntax );
  syntax.addFlag("-c", "-code", MSyntax::kString);
}

void FabricDFGSetCodeCommand::GetArgs(
  MArgParser &argParser,
  Args &args
  )
{
  Parent::GetArgs( argParser, args );

  if ( !argParser.isFlagSet( "code" ) )
    throw ArgException( MS::kFailure, "-code not provided." );
  args.code = argParser.flagArgumentString( "code", 0 ).asChar();
}

FabricUI::DFG::DFGUICmd *FabricDFGSetCodeCommand::executeDFGUICmd(
  MArgParser &argParser
  )
{
  Args args;
  GetArgs( argParser, args );

  FabricUI::DFG::DFGUICmd_SetCode *cmd =
    new FabricUI::DFG::DFGUICmd_SetCode(
      args.binding,
      args.execPath,
      args.exec,
      args.code
      );
  cmd->doit();
  return cmd;
}

// FabricDFGImportJSONCommand

MSyntax FabricDFGImportJSONCommand::newSyntax()
{
  MSyntax syntax;
  syntax.addFlag("-mn", "-mayaNode", MSyntax::kString);
  syntax.addFlag("-p", "-path", MSyntax::kString);
  syntax.addFlag("-f", "-filePath", MSyntax::kString);
  syntax.addFlag("-j", "-json", MSyntax::kString);
  syntax.addFlag("-r", "-referenced", MSyntax::kBoolean);
  syntax.enableQuery(false);
  syntax.enableEdit(false);
  return syntax;
}

MStatus FabricDFGImportJSONCommand::doIt(const MArgList &args)
{
  MStatus status;
  MArgParser argParser( syntax(), args, &status );
  if ( status != MS::kSuccess )
    return status;

  try
  {
    if ( !argParser.isFlagSet("mayaNode") )
      throw ArgException( MS::kFailure, "-mayaNode not provided." );
    MString mayaNodeName = argParser.flagArgumentString("mayaNode", 0);

    FabricDFGBaseInterface * interf =
      FabricDFGBaseInterface::getInstanceByName( mayaNodeName.asChar() );
    if ( !interf )
      throw ArgException( MS::kNotFound, "-mayaNode '" + mayaNodeName + "' not found." );

    MString path; // for now we aren't using this.
    if(argParser.isFlagSet("path"))
      path = argParser.flagArgumentString("path", 0);

    if(!argParser.isFlagSet("filePath") && !argParser.isFlagSet("json"))
      throw ArgException( MS::kFailure, MString(getName()) + ": Either File path (-f, -filePath) or JSON (-j, -json) has to be provided.");

    bool asReferenced = false;
    MString filePath;

    MString json;
    if(argParser.isFlagSet("filePath"))
    {
      if(argParser.isFlagSet("referenced"))
        asReferenced = argParser.flagArgumentBool("referenced", 0);

      filePath = argParser.flagArgumentString("filePath", 0);
      if(filePath.length() == 0)
      {
        QString qFileName = QFileDialog::getOpenFileName( 
          MQtUtil::mainWindow(), 
          "Choose DFG file", 
          QDir::currentPath(), 
          "DFG files (*.dfg.json);;All files (*.*)"
        );

        if(qFileName.isNull())
          throw ArgException( MS::kFailure, "No filename specified.");

        filePath = qFileName.toUtf8().constData();      
      }

      FILE * file = fopen(filePath.asChar(), "rb");
      if(!file)
        throw ArgException( MS::kNotFound, "File path (-f, -filePath) '"+filePath+"' cannot be found.");

      fseek( file, 0, SEEK_END );
      long fileSize = ftell( file );
      rewind( file );

      char * buffer = (char*) malloc(fileSize + 1);
      buffer[fileSize] = '\0';

      size_t readBytes = fread(buffer, 1, fileSize, file);
      assert(readBytes == size_t(fileSize));
      (void)readBytes;

      fclose(file);

      json = buffer;
      free(buffer);
    }
    else if(argParser.isFlagSet("json"))
    {
      json = argParser.flagArgumentString("json", 0);
    }

    interf->restoreFromJSON(json);
    if(asReferenced)
      interf->setReferencedFilePath(filePath);
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
  
  // this command isn't issued through the UI
  // m_cmdInfo = FabricDFGCommandStack::consumeCommandToIgnore(getName());
  
  return status;
}

// FabricDFGReloadJSONCommand

MSyntax FabricDFGReloadJSONCommand::newSyntax()
{
  MSyntax syntax;
  syntax.addFlag("-mn", "-mayaNode", MSyntax::kString);
  syntax.addFlag("-p", "-path", MSyntax::kString);
  syntax.addFlag("-f", "-filePath", MSyntax::kString);
  syntax.addFlag("-j", "-json", MSyntax::kString);
  syntax.addFlag("-r", "-referenced", MSyntax::kBoolean);
  syntax.enableQuery(false);
  syntax.enableEdit(false);
  return syntax;
}

MStatus FabricDFGReloadJSONCommand::doIt(const MArgList &args)
{
  MStatus status;
  MArgParser argParser( syntax(), args, &status );
  if ( status != MS::kSuccess )
    return status;

  try
  {
    if ( !argParser.isFlagSet("mayaNode") )
      throw ArgException( MS::kFailure, "-mayaNode not provided." );
    MString mayaNodeName = argParser.flagArgumentString("mayaNode", 0);

    FabricDFGBaseInterface * interf =
      FabricDFGBaseInterface::getInstanceByName( mayaNodeName.asChar() );
    if ( !interf )
      throw ArgException( MS::kNotFound, "-mayaNode '" + mayaNodeName + "' not found." );

    interf->reloadFromReferencedFilePath();
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
  
  // this command isn't issued through the UI
  // m_cmdInfo = FabricDFGCommandStack::consumeCommandToIgnore(getName());
  
  return status;
}

// FabricDFGExportJSONCommand

MSyntax FabricDFGExportJSONCommand::newSyntax()
{
  MSyntax syntax;
  syntax.addFlag("-mn", "-mayaNode", MSyntax::kString);
  syntax.addFlag("-p", "-path", MSyntax::kString);
  syntax.addFlag("-f", "-filePath", MSyntax::kString);
  syntax.enableQuery(false);
  syntax.enableEdit(false);
  return syntax;
}

MStatus FabricDFGExportJSONCommand::doIt(const MArgList &args)
{
  MStatus status;
  MArgParser argParser( syntax(), args, &status );
  if ( status != MS::kSuccess )
    return status;

  try
  {
    if ( !argParser.isFlagSet("mayaNode") )
      throw ArgException( MS::kFailure, "-mayaNode not provided." );
    MString mayaNodeName = argParser.flagArgumentString("mayaNode", 0);

    FabricDFGBaseInterface * interf =
      FabricDFGBaseInterface::getInstanceByName( mayaNodeName.asChar() );
    if ( !interf )
      throw ArgException( MS::kNotFound, "-mayaNode '" + mayaNodeName + "' not found." );

    MString path;
    if(argParser.isFlagSet("path"))
      path = argParser.flagArgumentString("path", 0);
    MString filePath;
    if(argParser.isFlagSet("filePath"))
    {
      filePath = argParser.flagArgumentString("filePath", 0);
      if(filePath.length() == 0)
      {
        QString qFileName = QFileDialog::getSaveFileName( 
          MQtUtil::mainWindow(), 
          "Choose DFG file", 
          QDir::currentPath(), 
          "DFG files (*.dfg.json);;All files (*.*)"
        );

        if(qFileName.isNull())
          throw ArgException( MS::kFailure, "No filename specified.");
        
        filePath = qFileName.toUtf8().constData();      
      }
    }

    MString json = interf->getDFGController()->exportJSON(path.asChar()).c_str();
    
    // this command isn't issued through the UI
    // m_cmdInfo = FabricDFGCommandStack::consumeCommandToIgnore(getName());
    
    setResult(json);

    // export to file
    if(filePath.length() > 0)
    {
      FILE * file = fopen(filePath.asChar(), "wb");
      if(!file)
        throw ArgException( MS::kFailure, "File path (-f, -filePath) '"+filePath+"' is not accessible.");
      else
      {
        fwrite(json.asChar(), json.length(), 1, file);
        fclose(file);
      }
    }
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
  
  // this command isn't issued through the UI
  // m_cmdInfo = FabricDFGCommandStack::consumeCommandToIgnore(getName());
  
  return status;
}
