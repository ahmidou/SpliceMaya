//
// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
//

#include <QtGui/QFileDialog>  // [pzion 20150519] Must include first since something below defines macros that mess it up

#include "Foundation.h"
#include "FabricDFGCommands.h"
#include "FabricSpliceHelpers.h"

#include <FabricUI/DFG/DFGUICmdHandler.h>

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

  if (status != MS::kFailure)
  {
    /* [FE-6195]
       Issue: when executing a DFG command the Canvas graph gets modified,
       however Maya is not aware of the changes (unless some plugs were
       dirtyfied.
       Fix: we call the MEL command "file -modified true". It dirtyfies the
       .ma/.mb file associated to the current scene, i.e. it sets the Maya
       scene status to "the scene contains unsaved changes".
    */
    MGlobal::executeCommandOnIdle("file -modified true", false /* displayEnabled*/);
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
    throw ArgException( MS::kFailure, "-m (-mayaNode) not provided." );
  MString mayaNodeName = argParser.flagArgumentString("mayaNode", 0);

  FabricDFGBaseInterface * interf =
    FabricDFGBaseInterface::getInstanceByName( mayaNodeName.asChar() );
  if ( !interf )
    throw ArgException( MS::kNotFound, "Maya node '" + mayaNodeName + "' not found." );
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
    throw ArgException( MS::kFailure, "-e (-execPath) not provided." );
  MString execPathMString = argParser.flagArgumentString( "execPath", 0 );
  args.execPath = execPathMString.asChar();
  args.exec = args.binding.getExec().getSubExec( execPathMString.asChar() );
}

// FabricDFGAddNodeCommand

void FabricDFGAddNodeCommand::AddSyntax( MSyntax &syntax )
{
  Parent::AddSyntax( syntax );
  syntax.addFlag( "-x", "-xPos", MSyntax::kString );
  syntax.addFlag( "-y", "-yPos", MSyntax::kString );
}

void FabricDFGAddNodeCommand::GetArgs( MArgParser &argParser, Args &args )
{
  Parent::GetArgs( argParser, args );

  args.pos = QPointF( 0, 0 );
  if ( argParser.isFlagSet("xPos") )
    args.pos.setX(
      FTL::CStrRef(
        argParser.flagArgumentString("xPos", 0).asChar()
        ).toFloat64()
      );
  if ( argParser.isFlagSet("yPos") )
    args.pos.setY(
      FTL::CStrRef(
        argParser.flagArgumentString("yPos", 0).asChar()
        ).toFloat64()
      );
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
    throw ArgException( MS::kFailure, "-t (-title) not provided." );
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
    throw ArgException( MS::kFailure, "-p (-presetPath) not provided." );
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
  syntax.addFlag("-xd", "-extDep", MSyntax::kString);
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
  syntax.addFlag("-p", "-varPath", MSyntax::kString);
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
    throw ArgException( MS::kFailure, "-s (-srcPortPath) not provided." );
  args.srcPort = argParser.flagArgumentString( "srcPortPath", 0 ).asChar();

  if ( !argParser.isFlagSet( "dstPortPath" ) )
    throw ArgException( MS::kFailure, "-d (-dstPortPath) not provided." );
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
  syntax.addFlag( "-n", "-nodeName", MSyntax::kString );
  syntax.addFlag( "-x", "-xPos", MSyntax::kString );
  syntax.addFlag( "-y", "-yPos", MSyntax::kString );
}

void FabricDFGMoveNodesCommand::GetArgs(
  MArgParser &argParser,
  Args &args
  )
{
  Parent::GetArgs( argParser, args );

  if ( !argParser.isFlagSet( "nodeName" ) )
    throw ArgException( MS::kFailure, "-n (-nodeName) not provided." );
  FTL::StrRef nodeNameStr =
    argParser.flagArgumentString( "nodeName", 0 ).asChar();
  while ( !nodeNameStr.empty() )
  {
    FTL::StrRef::Split split = nodeNameStr.trimSplit('|');
    args.nodeNames.push_back( split.first );
    nodeNameStr = split.second;
  }

  if ( !argParser.isFlagSet( "xPos" ) )
    throw ArgException( MS::kFailure, "-x (-xPos) not provided." );
  FTL::StrRef xPosStr =
    argParser.flagArgumentString( "xPos", 0 ).asChar();
  if ( !argParser.isFlagSet( "yPos" ) )
    throw ArgException( MS::kFailure, "-y (-yPos) not provided." );
  FTL::StrRef yPosStr =
    argParser.flagArgumentString( "yPos", 0 ).asChar();
  while ( !xPosStr.empty() && !yPosStr.empty() )
  {
    QPointF pos;
    FTL::StrRef::Split xPosSplit = xPosStr.trimSplit('|');
    if ( !xPosSplit.first.empty() )
      pos.setX( xPosSplit.first.toFloat64() );
    xPosStr = xPosSplit.second;
    FTL::StrRef::Split yPosSplit = yPosStr.trimSplit('|');
    if ( !yPosSplit.first.empty() )
      pos.setY( yPosSplit.first.toFloat64() );
    yPosStr = yPosSplit.second;
    args.poss.push_back( pos );
  }
}

FabricUI::DFG::DFGUICmd *FabricDFGMoveNodesCommand::executeDFGUICmd(
  MArgParser &argParser
  )
{
  Args args;
  GetArgs( argParser, args );

  std::vector<FTL::StrRef> nodeNames;
  nodeNames.insert(
    nodeNames.end(),
    args.nodeNames.begin(),
    args.nodeNames.end()
    );

  FabricUI::DFG::DFGUICmd_MoveNodes *cmd =
    new FabricUI::DFG::DFGUICmd_MoveNodes(
      args.binding,
      args.execPath,
      args.exec,
      nodeNames,
      args.poss
      );
  cmd->doit();
  return cmd;
}

// FabricDFGSetExtDepsCommand

void FabricDFGSetExtDepsCommand::AddSyntax( MSyntax &syntax )
{
  Parent::AddSyntax( syntax );
  syntax.addFlag( "-xd", "-extDep", MSyntax::kString );
}

void FabricDFGSetExtDepsCommand::GetArgs(
  MArgParser &argParser,
  Args &args
  )
{
  Parent::GetArgs( argParser, args );

  if ( !argParser.isFlagSet( "extDep" ) )
    throw ArgException( MS::kFailure, "-xd (-extDep) not provided." );
  FTL::StrRef extDepStr =
    argParser.flagArgumentString( "extDep", 0 ).asChar();
  while ( !extDepStr.empty() )
  {
    FTL::StrRef::Split split = extDepStr.trimSplit('|');
    args.extDeps.push_back( split.first );
    extDepStr = split.second;
  }
}

FabricUI::DFG::DFGUICmd *FabricDFGSetExtDepsCommand::executeDFGUICmd(
  MArgParser &argParser
  )
{
  Args args;
  GetArgs( argParser, args );

  std::vector<FTL::StrRef> extDeps;
  extDeps.insert(
    extDeps.end(),
    args.extDeps.begin(),
    args.extDeps.end()
    );

  FabricUI::DFG::DFGUICmd_SetExtDeps *cmd =
    new FabricUI::DFG::DFGUICmd_SetExtDeps(
      args.binding,
      args.execPath,
      args.exec,
      extDeps
      );
  cmd->doit();
  return cmd;
}

// FabricDFGSplitFromPresetCommand

void FabricDFGSplitFromPresetCommand::AddSyntax( MSyntax &syntax )
{
  Parent::AddSyntax( syntax );
}

void FabricDFGSplitFromPresetCommand::GetArgs(
  MArgParser &argParser,
  Args &args
  )
{
  Parent::GetArgs( argParser, args );
}

FabricUI::DFG::DFGUICmd *FabricDFGSplitFromPresetCommand::executeDFGUICmd(
  MArgParser &argParser
  )
{
  Args args;
  GetArgs( argParser, args );

  FabricUI::DFG::DFGUICmd_SplitFromPreset *cmd =
    new FabricUI::DFG::DFGUICmd_SplitFromPreset(
      args.binding,
      args.execPath,
      args.exec
      );
  cmd->doit();
  return cmd;
}

// FabricDFGImplodeNodesCommand

void FabricDFGImplodeNodesCommand::AddSyntax( MSyntax &syntax )
{
  Parent::AddSyntax( syntax );
  syntax.addFlag("-n", "-nodeName", MSyntax::kString);
  syntax.addFlag("-d", "-desiredImplodedNodeName", MSyntax::kString);
}

void FabricDFGImplodeNodesCommand::GetArgs(
  MArgParser &argParser,
  Args &args
  )
{
  Parent::GetArgs( argParser, args );

  if ( !argParser.isFlagSet( "nodeName" ) )
    throw ArgException( MS::kFailure, "-n (-nodeName) not provided." );
  FTL::StrRef nodeNameStr =
    argParser.flagArgumentString( "nodeName", 0 ).asChar();
  while ( !nodeNameStr.empty() )
  {
    FTL::StrRef::Split split = nodeNameStr.trimSplit('|');
    if ( !split.first.empty() )
      args.nodeNames.push_back( split.first );
    nodeNameStr = split.second;
  }

  if ( argParser.isFlagSet( "desiredImplodedNodeName" ) )
    args.desiredImplodedNodeName =
      argParser.flagArgumentString( "desiredImplodedNodeName", 0 ).asChar();;
}

FabricUI::DFG::DFGUICmd *FabricDFGImplodeNodesCommand::executeDFGUICmd(
  MArgParser &argParser
  )
{
  Args args;
  GetArgs( argParser, args );

  std::vector<FTL::StrRef> nodeNames;
  nodeNames.insert(
    nodeNames.end(),
    args.nodeNames.begin(),
    args.nodeNames.end()
    );

  FabricUI::DFG::DFGUICmd_ImplodeNodes *cmd =
    new FabricUI::DFG::DFGUICmd_ImplodeNodes(
      args.binding,
      args.execPath,
      args.exec,
      nodeNames,
      args.desiredImplodedNodeName
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
    throw ArgException( MS::kFailure, "-n (-nodeName) not provided." );
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

  MString mExplodedNodeNames;
  for ( FTL::ArrayRef<std::string>::IT it = explodedNodeNames.begin();
    it != explodedNodeNames.end(); ++it )
  {
    if ( it != explodedNodeNames.begin() )
      mExplodedNodeNames += "|";
    mExplodedNodeNames += it->c_str();
  }
  setResult( mExplodedNodeNames );

  return cmd;
}

// FabricDFGPasteCommand

void FabricDFGPasteCommand::AddSyntax( MSyntax &syntax )
{
  Parent::AddSyntax( syntax );
  syntax.addFlag( "-t", "-text", MSyntax::kString );
  syntax.addFlag( "-x", "-xPos", MSyntax::kString );
  syntax.addFlag( "-y", "-yPos", MSyntax::kString );
}

void FabricDFGPasteCommand::GetArgs(
  MArgParser &argParser,
  Args &args
  )
{
  Parent::GetArgs( argParser, args );

  if ( !argParser.isFlagSet( "text" ) )
    throw ArgException( MS::kFailure, "-t (-text) not provided." );
  args.text = argParser.flagArgumentString( "text", 0 ).asChar();

  args.xy = QPointF( 0, 0 );
  if ( argParser.isFlagSet("xPos") )
    args.xy.setX(
      FTL::CStrRef(
        argParser.flagArgumentString("xPos", 0).asChar()
        ).toFloat64()
      );
  if ( argParser.isFlagSet("yPos") )
    args.xy.setY(
      FTL::CStrRef(
        argParser.flagArgumentString("yPos", 0).asChar()
        ).toFloat64()
      );
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

  MString mPastedNodeNames;
  for ( FTL::ArrayRef<std::string>::IT it = pastedNodeNames.begin();
    it != pastedNodeNames.end(); ++it )
  {
    if ( it != pastedNodeNames.begin() )
      mPastedNodeNames += "|";
    mPastedNodeNames += it->c_str();
  }
  setResult( mPastedNodeNames );

  return cmd;
}

// FabricDFGResizeBackDropCommand

void FabricDFGResizeBackDropCommand::AddSyntax( MSyntax &syntax )
{
  Parent::AddSyntax( syntax );
  syntax.addFlag("-n", "-nodeName", MSyntax::kString);
  syntax.addFlag( "-x", "-xPos", MSyntax::kString );
  syntax.addFlag( "-y", "-yPos", MSyntax::kString );
  syntax.addFlag( "-w", "-width", MSyntax::kString );
  syntax.addFlag( "-h", "-height", MSyntax::kString );
}

void FabricDFGResizeBackDropCommand::GetArgs(
  MArgParser &argParser,
  Args &args
  )
{
  Parent::GetArgs( argParser, args );

  if ( !argParser.isFlagSet( "nodeName" ) )
    throw ArgException( MS::kFailure, "-n (-nodeName) not provided." );
  args.nodeName = argParser.flagArgumentString( "nodeName", 0 ).asChar();

  args.xy = QPointF( 0, 0 );
  if ( argParser.isFlagSet("xPos") )
    args.xy.setX(
      FTL::CStrRef(
        argParser.flagArgumentString("xPos", 0).asChar()
        ).toFloat64()
      );
  if ( argParser.isFlagSet("yPos") )
    args.xy.setY(
      FTL::CStrRef(
        argParser.flagArgumentString("yPos", 0).asChar()
        ).toFloat64()
      );

  args.wh = QSizeF( 0, 0 );
  if ( argParser.isFlagSet("width") )
    args.wh.setWidth(
      FTL::CStrRef(
        argParser.flagArgumentString("width", 0).asChar()
        ).toFloat64()
      );
  if ( argParser.isFlagSet("height") )
    args.wh.setHeight(
      FTL::CStrRef(
        argParser.flagArgumentString("height", 0).asChar()
        ).toFloat64()
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
}

void FabricDFGRemoveNodesCommand::GetArgs(
  MArgParser &argParser,
  Args &args
  )
{
  Parent::GetArgs( argParser, args );

  if ( !argParser.isFlagSet( "nodeName" ) )
    throw ArgException( MS::kFailure, "-n (-nodeName) not provided." );
  FTL::StrRef nodeNameStr =
    argParser.flagArgumentString( "nodeName", 0 ).asChar();
  while ( !nodeNameStr.empty() )
  {
    FTL::StrRef::Split split = nodeNameStr.trimSplit('|');
    if ( !split.first.empty() )
      args.nodeNames.push_back( split.first );
    nodeNameStr = split.second;
  }
}

FabricUI::DFG::DFGUICmd *FabricDFGRemoveNodesCommand::executeDFGUICmd(
  MArgParser &argParser
  )
{
  Args args;
  GetArgs( argParser, args );

  std::vector<FTL::StrRef> nodeNames;
  nodeNames.insert(
    nodeNames.end(),
    args.nodeNames.begin(),
    args.nodeNames.end()
    );

  FabricUI::DFG::DFGUICmd_RemoveNodes *cmd =
    new FabricUI::DFG::DFGUICmd_RemoveNodes(
      args.binding,
      args.execPath,
      args.exec,
      nodeNames
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
  syntax.addFlag("-xd", "-extDep", MSyntax::kString);
  syntax.addFlag("-ui", "-uiMetadata", MSyntax::kString);
}

void FabricDFGAddPortCommand::GetArgs(
  MArgParser &argParser,
  Args &args
  )
{
  Parent::GetArgs( argParser, args );

  if ( !argParser.isFlagSet( "desiredPortName" ) )
    throw ArgException( MS::kFailure, "-d (-desiredPortName) not provided." );
  args.desiredPortName = argParser.flagArgumentString( "desiredPortName", 0 ).asChar();

  if ( !argParser.isFlagSet( "portType" ) )
    throw ArgException( MS::kFailure, "-p (-portType) not provided." );
  MString portTypeString = argParser.flagArgumentString( "portType", 0 ).asChar();
  portTypeString.toLowerCase(); 
  if ( portTypeString == "in" )
    args.portType = FabricCore::DFGPortType_In;
  else if ( portTypeString == "io" )
    args.portType = FabricCore::DFGPortType_IO;
  else if ( portTypeString == "out" )
    args.portType = FabricCore::DFGPortType_Out;
  else
    throw ArgException( MS::kFailure, "-p (-portType) value unrecognized" );

  if ( argParser.isFlagSet( "typeSpec" ) )
    args.typeSpec = argParser.flagArgumentString( "typeSpec", 0 ).asChar();

  if ( argParser.isFlagSet( "connectToPortPath" ) )
    args.portToConnectWith = argParser.flagArgumentString( "connectToPortPath", 0 ).asChar();

  if ( argParser.isFlagSet( "extDep" ) )
    args.extDep = argParser.flagArgumentString( "extDep", 0 ).asChar();

  if ( argParser.isFlagSet( "uiMetadata" ) )
    args.uiMetadata = argParser.flagArgumentString( "uiMetadata", 0 ).asChar();
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
      args.portToConnectWith,
      args.extDep,
      args.uiMetadata
      );
  cmd->doit();
  setResult( cmd->getActualPortName().c_str() );
  return cmd;
}

// FabricDFGCreatePresetCommand

void FabricDFGCreatePresetCommand::AddSyntax( MSyntax &syntax )
{
  Parent::AddSyntax( syntax );
  syntax.addFlag("-n", "-nodeName", MSyntax::kString);
  syntax.addFlag("-pd", "-presetDirPath", MSyntax::kString);
  syntax.addFlag("-pn", "-presetName", MSyntax::kString);
}

void FabricDFGCreatePresetCommand::GetArgs(
  MArgParser &argParser,
  Args &args
  )
{
  Parent::GetArgs( argParser, args );

  if ( !argParser.isFlagSet( "nodeName" ) )
    throw ArgException( MS::kFailure, "-n (-nodeName) not provided." );
  args.nodeName = argParser.flagArgumentString( "nodeName", 0 ).asChar();

  if ( !argParser.isFlagSet( "presetDirPath" ) )
    throw ArgException( MS::kFailure, "-n (-presetDirPath) not provided." );
  args.presetDirPath = argParser.flagArgumentString( "presetDirPath", 0 ).asChar();

  if ( !argParser.isFlagSet( "presetName" ) )
    throw ArgException( MS::kFailure, "-n (-presetName) not provided." );
  args.presetName = argParser.flagArgumentString( "presetName", 0 ).asChar();
}

FabricUI::DFG::DFGUICmd *FabricDFGCreatePresetCommand::executeDFGUICmd(
  MArgParser &argParser
  )
{
  Args args;
  GetArgs( argParser, args );

  FabricUI::DFG::DFGUICmd_CreatePreset *cmd =
    new FabricUI::DFG::DFGUICmd_CreatePreset(
      args.binding,
      args.execPath,
      args.exec,
      args.nodeName,
      args.presetDirPath,
      args.presetName
      );
  cmd->doit();
  setResult( cmd->getPathname().c_str() );
  return cmd;
}

// FabricDFGEditPortCommand

void FabricDFGEditPortCommand::AddSyntax( MSyntax &syntax )
{
  Parent::AddSyntax( syntax );
  syntax.addFlag("-n", "-oldPortName", MSyntax::kString);
  syntax.addFlag("-d", "-desiredNewPortName", MSyntax::kString);
  syntax.addFlag("-t", "-typeSpec", MSyntax::kString);
  syntax.addFlag("-xd", "-extDep", MSyntax::kString);
  syntax.addFlag("-ui", "-uiMetadata", MSyntax::kString);
}

void FabricDFGEditPortCommand::GetArgs(
  MArgParser &argParser,
  Args &args
  )
{
  Parent::GetArgs( argParser, args );

  if ( !argParser.isFlagSet( "oldPortName" ) )
    throw ArgException( MS::kFailure, "-n (-oldPortName) not provided." );
  args.oldPortName = argParser.flagArgumentString( "oldPortName", 0 ).asChar();

  if ( argParser.isFlagSet( "desiredNewPortName" ) )
    args.desiredNewPortName = argParser.flagArgumentString( "desiredNewPortName", 0 ).asChar();
  else
    args.desiredNewPortName = args.oldPortName;

  if ( argParser.isFlagSet( "typeSpec" ) )
    args.typeSpec = argParser.flagArgumentString( "typeSpec", 0 ).asChar();

  if ( argParser.isFlagSet( "extDep" ) )
    args.extDep = argParser.flagArgumentString( "extDep", 0 ).asChar();

  if ( argParser.isFlagSet( "uiMetadata" ) )
    args.uiMetadata = argParser.flagArgumentString( "uiMetadata", 0 ).asChar();
}

FabricUI::DFG::DFGUICmd *FabricDFGEditPortCommand::executeDFGUICmd(
  MArgParser &argParser
  )
{
  Args args;
  GetArgs( argParser, args );

  FabricUI::DFG::DFGUICmd_EditPort *cmd =
    new FabricUI::DFG::DFGUICmd_EditPort(
      args.binding,
      args.execPath,
      args.exec,
      args.oldPortName,
      args.desiredNewPortName,
      args.typeSpec,
      args.extDep,
      args.uiMetadata
      );
  cmd->doit();
  setResult( cmd->getActualNewPortName().c_str() );
  return cmd;
}

// FabricDFGSetArgTypeCommand

void FabricDFGSetArgTypeCommand::AddSyntax( MSyntax &syntax )
{
  Parent::AddSyntax( syntax );
  syntax.addFlag("-n", "-argName", MSyntax::kString);
  syntax.addFlag("-t", "-type", MSyntax::kString);
}

void FabricDFGSetArgTypeCommand::GetArgs(
  MArgParser &argParser,
  Args &args
  )
{
  Parent::GetArgs( argParser, args );

  if ( !argParser.isFlagSet( "argName" ) )
    throw ArgException( MS::kFailure, "-n (-argName) not provided." );
  args.argName = argParser.flagArgumentString( "argName", 0 ).asChar();

  if ( !argParser.isFlagSet( "type" ) )
    throw ArgException( MS::kFailure, "-t (-type) not provided." );
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
  syntax.addFlag("-n", "-argName", MSyntax::kString);
  syntax.addFlag("-t", "-type", MSyntax::kString);
  syntax.addFlag("-v", "-value", MSyntax::kString);
}

void FabricDFGSetArgValueCommand::GetArgs(
  MArgParser &argParser,
  Args &args
  )
{
  Parent::GetArgs( argParser, args );

  if ( !argParser.isFlagSet( "argName" ) )
    throw ArgException( MS::kFailure, "-n (-argName) not provided." );
  args.argName = argParser.flagArgumentString( "argName", 0 ).asChar();

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
    throw ArgException( MS::kFailure, "-p (-portPath) not provided." );
  args.portPath = argParser.flagArgumentString( "portPath", 0 ).asChar();

  if ( !argParser.isFlagSet( "type" ) )
    throw ArgException( MS::kFailure, "-t (-type) not provided." );
  MString type = argParser.flagArgumentString( "type", 0 ).asChar();

  if ( !argParser.isFlagSet( "value" ) )
    throw ArgException( MS::kFailure, "-v (-value) not provided." );
  MString valueJSON = argParser.flagArgumentString( "value", 0 ).asChar();
  FabricCore::DFGHost host = args.binding.getHost();
  FabricCore::Context context = host.getContext();
  args.value = FabricCore::RTVal::Construct( context, type.asChar(), 0, NULL );
  FabricUI::DFG::DFGUICmdHandler::decodeRTValFromJSON(
    context,
    args.value,
    valueJSON.asChar()
    );
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

// FabricDFGSetTitleCommand

void FabricDFGSetTitleCommand::AddSyntax( MSyntax &syntax )
{
  Parent::AddSyntax( syntax );
  syntax.addFlag("-t", "-title", MSyntax::kString);
}

void FabricDFGSetTitleCommand::GetArgs(
  MArgParser &argParser,
  Args &args
  )
{
  Parent::GetArgs( argParser, args );

  if ( !argParser.isFlagSet( "title" ) )
    throw ArgException( MS::kFailure, "-t (-title) not provided." );
  args.title = argParser.flagArgumentString( "title", 0 ).asChar();
}

FabricUI::DFG::DFGUICmd *FabricDFGSetTitleCommand::executeDFGUICmd(
  MArgParser &argParser
  )
{
  Args args;
  GetArgs( argParser, args );

  FabricUI::DFG::DFGUICmd_SetTitle *cmd =
    new FabricUI::DFG::DFGUICmd_SetTitle(
      args.binding,
      args.execPath,
      args.exec,
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
    throw ArgException( MS::kFailure, "-n (-nodeName) not provided." );
  args.nodeName = argParser.flagArgumentString( "nodeName", 0 ).asChar();

  if ( !argParser.isFlagSet( "comment" ) )
    throw ArgException( MS::kFailure, "-c (-comment) not provided." );
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
  syntax.addFlag("-n", "-refName", MSyntax::kString);
  syntax.addFlag("-p", "-varPath", MSyntax::kString);
}

void FabricDFGSetRefVarPathCommand::GetArgs(
  MArgParser &argParser,
  Args &args
  )
{
  Parent::GetArgs( argParser, args );

  if ( !argParser.isFlagSet( "refName" ) )
    throw ArgException( MS::kFailure, "-n (-refName) not provided." );
  args.refName = argParser.flagArgumentString( "refName", 0 ).asChar();

  if ( !argParser.isFlagSet( "varPath" ) )
    throw ArgException( MS::kFailure, "-p (-varPath) not provided." );
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

// FabricDFGReorderPortsCommand

void FabricDFGReorderPortsCommand::AddSyntax( MSyntax &syntax )
{
  Parent::AddSyntax( syntax );
  syntax.addFlag("-i", "-indices", MSyntax::kString);
}

void FabricDFGReorderPortsCommand::GetArgs(
  MArgParser &argParser,
  Args &args
  )
{
  Parent::GetArgs( argParser, args );

  if ( !argParser.isFlagSet( "indices" ) )
    throw ArgException( MS::kFailure, "-i (-indices) not provided." );

  std::string jsonStr = argParser.flagArgumentString( "indices", 0 ).asChar();

  try
  {
    FTL::JSONStrWithLoc jsonStrWithLoc( jsonStr );
    FTL::OwnedPtr<FTL::JSONArray> jsonArray(
      FTL::JSONValue::Decode( jsonStrWithLoc )->cast<FTL::JSONArray>()
      );
    for( size_t i=0; i < jsonArray->size(); i++ )
    {
      args.indices.push_back ( jsonArray->get(i)->getSInt32Value() );
    }
  }
  catch ( FabricCore::Exception e )
  {
    throw ArgException( MS::kFailure, "-i (-indices) not valid json." );
  }
}

FabricUI::DFG::DFGUICmd *FabricDFGReorderPortsCommand::executeDFGUICmd(
  MArgParser &argParser
  )
{
  Args args;
  GetArgs( argParser, args );

  FabricUI::DFG::DFGUICmd_ReorderPorts *cmd =
    new FabricUI::DFG::DFGUICmd_ReorderPorts(
      args.binding,
      args.execPath,
      args.exec,
      args.indices
      );
  cmd->doit();
  return cmd;
}

// FabricDFGDismissLoadDiagsCommand

void FabricDFGDismissLoadDiagsCommand::AddSyntax( MSyntax &syntax )
{
  Parent::AddSyntax( syntax );
  syntax.addFlag("-di", "-diagIndices", MSyntax::kString);
}

void FabricDFGDismissLoadDiagsCommand::GetArgs(
  MArgParser &argParser,
  Args &args
  )
{
  Parent::GetArgs( argParser, args );

  if ( !argParser.isFlagSet( "diagIndices" ) )
    throw ArgException( MS::kFailure, "-di (-diagIndices) not provided." );

  std::string jsonStr = argParser.flagArgumentString( "diagIndices", 0 ).asChar();

  try
  {
    FTL::JSONStrWithLoc jsonStrWithLoc( jsonStr );
    FTL::OwnedPtr<FTL::JSONArray> jsonArray(
      FTL::JSONValue::Decode( jsonStrWithLoc )->cast<FTL::JSONArray>()
      );
    for( size_t i=0; i < jsonArray->size(); i++ )
    {
      args.indices.append( jsonArray->get(i)->getSInt32Value() );
    }
  }
  catch ( FabricCore::Exception e )
  {
    throw ArgException( MS::kFailure, "-di (-diagIndices) not valid json." );
  }
}

FabricUI::DFG::DFGUICmd *FabricDFGDismissLoadDiagsCommand::executeDFGUICmd(
  MArgParser &argParser
  )
{
  Args args;
  GetArgs( argParser, args );

  FabricUI::DFG::DFGUICmd_DismissLoadDiags *cmd =
    new FabricUI::DFG::DFGUICmd_DismissLoadDiags(
      args.binding,
      args.indices
      );
  cmd->doit();
  return cmd;
}

// FabricDFGEditNodeCommand

void FabricDFGEditNodeCommand::AddSyntax( MSyntax &syntax )
{
  Parent::AddSyntax( syntax );
  syntax.addFlag("-n", "-oldNodeName", MSyntax::kString);
  syntax.addFlag("-d", "-desiredNewNodeName", MSyntax::kString);
  syntax.addFlag("-nm", "-nodeMetadata", MSyntax::kString);
  syntax.addFlag("-xm", "-execMetadata", MSyntax::kString);
}

void FabricDFGEditNodeCommand::GetArgs(
  MArgParser &argParser,
  Args &args
  )
{
  Parent::GetArgs( argParser, args );

  if ( !argParser.isFlagSet( "oldNodeName" ) )
    throw ArgException( MS::kFailure, "-n (-oldNodeName) not provided." );
  args.oldNodeName = argParser.flagArgumentString( "oldNodeName", 0 ).asChar();

  if ( !argParser.isFlagSet( "desiredNewNodeName" ) )
    throw ArgException( MS::kFailure, "-d (-desiredNewNodeName) not provided." );
  args.desiredNewNodeName = argParser.flagArgumentString( "desiredNewNodeName", 0 ).asChar();

  if ( argParser.isFlagSet( "nodeMetadata" ) )
    args.nodeMetadata = argParser.flagArgumentString( "nodeMetadata", 0 ).asChar();

  if ( argParser.isFlagSet( "execMetadata" ) )
    args.execMetadata = argParser.flagArgumentString( "execMetadata", 0 ).asChar();
}

FabricUI::DFG::DFGUICmd *FabricDFGEditNodeCommand::executeDFGUICmd(
  MArgParser &argParser
  )
{
  Args args;
  GetArgs( argParser, args );

  FabricUI::DFG::DFGUICmd_EditNode *cmd =
    new FabricUI::DFG::DFGUICmd_EditNode(
      args.binding,
      args.execPath,
      args.exec,
      args.oldNodeName,
      args.desiredNewNodeName,
      args.nodeMetadata,
      args.execMetadata
      );
  cmd->doit();
  setResult( cmd->getActualNewNodeName().c_str() );
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
    throw ArgException( MS::kFailure, "-n (-oldPortName) not provided." );
  args.oldPortName = argParser.flagArgumentString( "oldPortName", 0 ).asChar();

  if ( !argParser.isFlagSet( "desiredNewPortName" ) )
    throw ArgException( MS::kFailure, "-d (-desiredNewPortName) not provided." );
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
  syntax.addFlag("-n", "-portName", MSyntax::kString);
}

void FabricDFGRemovePortCommand::GetArgs(
  MArgParser &argParser,
  Args &args
  )
{
  Parent::GetArgs( argParser, args );

  if ( !argParser.isFlagSet( "portName" ) )
    throw ArgException( MS::kFailure, "-n (-portName) not provided." );
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
    throw ArgException( MS::kFailure, "-c (-code) not provided." );
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
  syntax.addFlag("-m", "-mayaNode", MSyntax::kString);
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
      throw ArgException( MS::kFailure, "-m (-mayaNode) not provided." );
    MString mayaNodeName = argParser.flagArgumentString("mayaNode", 0);

    FabricDFGBaseInterface * interf =
      FabricDFGBaseInterface::getInstanceByName( mayaNodeName.asChar() );
    if ( !interf )
      throw ArgException( MS::kNotFound, "Maya node '" + mayaNodeName + "' not found." );

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
          "Choose canvas file", 
          QDir::currentPath(), 
          "Canvas files (*.canvas);;All files (*.*)"
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
  syntax.addFlag("-m", "-mayaNode", MSyntax::kString);
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
      throw ArgException( MS::kFailure, "-m (-mayaNode) not provided." );
    MString mayaNodeName = argParser.flagArgumentString("mayaNode", 0);

    FabricDFGBaseInterface * interf =
      FabricDFGBaseInterface::getInstanceByName( mayaNodeName.asChar() );
    if ( !interf )
      throw ArgException( MS::kNotFound, "Maya node '" + mayaNodeName + "' not found." );

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
  syntax.addFlag("-m", "-mayaNode", MSyntax::kString);
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
      throw ArgException( MS::kFailure, "-m (-mayaNode) not provided." );
    MString mayaNodeName = argParser.flagArgumentString("mayaNode", 0);

    FabricDFGBaseInterface * interf =
      FabricDFGBaseInterface::getInstanceByName( mayaNodeName.asChar() );
    if ( !interf )
      throw ArgException( MS::kNotFound, "Maya node '" + mayaNodeName + "' not found." );

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
          "Choose Canvas file", 
          QDir::currentPath(), 
          "Canvas files (*.canvas);;All files (*.*)"
        );

        if(qFileName.isNull())
          throw ArgException( MS::kFailure, "No filename specified.");
        
        filePath = qFileName.toUtf8().constData();      
      }
    }

    MString json = interf->getDFGBinding().exportJSON().getCString();
    
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

// FabricCanvasSetExecuteSharedCommand

MSyntax FabricCanvasSetExecuteSharedCommand::newSyntax()
{
  MSyntax syntax;
  syntax.addFlag("-m", "-mayaNode", MSyntax::kString);
  syntax.addFlag("-e", "-enable", MSyntax::kBoolean);
  syntax.enableQuery(false);
  syntax.enableEdit(false);
  return syntax;
}

MStatus FabricCanvasSetExecuteSharedCommand::doIt(const MArgList &args)
{
  MStatus status;
  MArgParser argParser( syntax(), args, &status );
  if ( status != MS::kSuccess )
    return status;

  try
  {
    if ( !argParser.isFlagSet("mayaNode") )
      throw ArgException( MS::kFailure, "-m (-mayaNode) not provided." );
    MString mayaNodeName = argParser.flagArgumentString("mayaNode", 0);

    m_interf =
      FabricDFGBaseInterface::getInstanceByName( mayaNodeName.asChar() );
    if ( !m_interf )
      throw ArgException(
        MS::kNotFound, "Maya node '" + mayaNodeName + "' not found."
        );
    FabricCore::DFGBinding binding = m_interf->getDFGBinding();

    if(!argParser.isFlagSet("enable"))
      throw ArgException( MS::kFailure, "-e (-enable) not provided." );
    bool enable = argParser.flagArgumentBool("enable", 0);
    
    char const *oldMetadataValueCStr =
      binding.getMetadata( "executeShared" );
    if ( oldMetadataValueCStr )
      m_oldMetadataValue = oldMetadataValueCStr;

    m_newMetadataValue = enable? "true": "false";

    binding.setMetadata(
      "executeShared",
      m_newMetadataValue.c_str(),
      false // canUndo
      );

    m_interf->setExecuteSharedDirty();

    status = MS::kSuccess;
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

MStatus FabricCanvasSetExecuteSharedCommand::undoIt()
{
  MStatus status = MS::kSuccess;

  try
  {
    m_interf->getDFGBinding().setMetadata(
      "executeShared",
      m_oldMetadataValue.c_str(),
      false // canUndo
      );

    m_interf->setExecuteSharedDirty();
  }
  catch ( FabricCore::Exception e )
  {
    logError( e.getDesc_cstr() );
    status = MS::kFailure;
  }

  return MS::kSuccess;
}

MStatus FabricCanvasSetExecuteSharedCommand::redoIt()
{
  MStatus status = MS::kSuccess;

  try
  {
    m_interf->getDFGBinding().setMetadata(
      "executeShared",
      m_newMetadataValue.c_str(),
      false // canUndo
      );

    m_interf->setExecuteSharedDirty();
  }
  catch ( FabricCore::Exception e )
  {
    logError( e.getDesc_cstr() );
    status = MS::kFailure;
  }

  return MS::kSuccess;
}
