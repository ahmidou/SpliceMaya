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
  syntax.addFlag( "-mn", "-mayaNode", MSyntax::kString );
}

MStatus FabricNewDFGBaseCommand::doIt( const MArgList &args )
{
  MStatus status;
  MArgParser argParser( syntax(), args, &status );
  if ( status != MS::kSuccess )
    return status;
  
  if ( !argParser.isFlagSet("mayaNode") )
  {
    logError( "-mayaNode not provided." );
    return MS::kFailure;
  }
  MString mayaNodeName = argParser.flagArgumentString("mayaNode", 0);

  FabricDFGBaseInterface * interf =
    FabricDFGBaseInterface::getInstanceByName( mayaNodeName.asChar() );
  if ( !interf )
  {
    logError( "Maya node '" + mayaNodeName + "' not found." );
    return MS::kNotFound;
  }
  m_binding = interf->getDFGBinding();

  try
  {
    return invoke( argParser, m_binding );
  }
  catch ( FabricCore::Exception e )
  {
    logError( e.getDesc_cstr() );
    return MS::kFailure;
  }
}

MStatus FabricNewDFGBaseCommand::undoIt()
{
  FabricCore::DFGHost host = getBinding().getHost();
  for ( unsigned i = 0; i < m_coreUndoCount; ++i )
    host.maybeUndo();
  return MS::kSuccess;
}

MStatus FabricNewDFGBaseCommand::redoIt()
{
  FabricCore::DFGHost host = getBinding().getHost();
  for ( unsigned i = 0; i < m_coreUndoCount; ++i )
    host.maybeRedo();
  return MS::kSuccess;
}

// FabricDFGExecCommand

void FabricDFGExecCommand::addSyntax( MSyntax &syntax )
{
  Parent::addSyntax( syntax );
  syntax.addFlag( "-ep", "-execPath", MSyntax::kString );
}

MStatus FabricDFGExecCommand::invoke(
  MArgParser &argParser,
  FabricCore::DFGBinding &binding
  )
{
  if ( !argParser.isFlagSet( "execPath" ) )
  {
    logError( "-execPath not provided." );
    return MS::kFailure;
  }
  MString execPath = argParser.flagArgumentString( "execPath", 0 );
  FabricCore::DFGExec exec =
    binding.getExec().getSubExec( execPath.asChar() );
  return invoke( argParser, exec );
}

// FabricDFGAddNodeCommand

void FabricDFGAddNodeCommand::addSyntax( MSyntax &syntax )
{
  Parent::addSyntax( syntax );
  syntax.addFlag( "-xy", "-position", MSyntax::kDouble, MSyntax::kDouble );
}

void FabricDFGAddNodeCommand::setPos(
  MArgParser &argParser,
  FabricCore::DFGExec &exec,
  FTL::CStrRef nodeName
  )
{
  if ( argParser.isFlagSet("position") )
  {
    FTL::OwnedPtr<FTL::JSONObject> uiGraphPosVal( new FTL::JSONObject );
    uiGraphPosVal->insert(
      FTL_STR("x"),
      new FTL::JSONFloat64( argParser.flagArgumentDouble("position", 0) )
      );
    uiGraphPosVal->insert(
      FTL_STR("y"),
      new FTL::JSONFloat64( argParser.flagArgumentDouble("position", 1) )
      );

    exec.setNodeMetadata(
      nodeName.c_str(),
      "uiGraphPos",
      uiGraphPosVal->encode().c_str(),
      false // canUndo
      );
  }
}

// FabricDFGAddInstFromPresetCommand

MSyntax FabricDFGAddInstFromPresetCommand::newSyntax()
{
  MSyntax syntax;
  Parent::addSyntax( syntax );
  syntax.addFlag("-pp", "-presetPath", MSyntax::kString);
  return syntax;
}

MStatus FabricDFGAddInstFromPresetCommand::invoke(
  MArgParser &argParser,
  FabricCore::DFGExec &exec
  )
{
  if ( !argParser.isFlagSet( "presetPath" ) )
  {
    logError( "-presetPath not provided." );
    return MS::kFailure;
  }
  MString presetPath = argParser.flagArgumentString( "presetPath", 0 );

  FTL::CStrRef nodeName = exec.addInstFromPreset( presetPath.asChar() );
  incCoreUndoCount();

  setPos( argParser, exec, nodeName );

  setResult( MString( nodeName.c_str() ) );
  return MS::kSuccess;
}

// FabricDFGAddInstWithEmptyGraphCommand

MSyntax FabricDFGAddInstWithEmptyGraphCommand::newSyntax()
{
  MSyntax syntax;
  Parent::addSyntax( syntax );
  syntax.addFlag("-t", "-title", MSyntax::kString);
  return syntax;
}

MStatus FabricDFGAddInstWithEmptyGraphCommand::invoke(
  MArgParser &argParser,
  FabricCore::DFGExec &exec
  )
{
  if ( !argParser.isFlagSet( "title" ) )
  {
    logError( "-title not provided." );
    return MS::kFailure;
  }
  MString title = argParser.flagArgumentString( "title", 0 );

  FTL::CStrRef nodeName = exec.addInstWithNewGraph( title.asChar() );
  incCoreUndoCount();

  setPos( argParser, exec, nodeName );

  setResult( MString( nodeName.c_str() ) );
  return MS::kSuccess;
}

// FabricDFGAddInstWithEmptyFuncCommand

MSyntax FabricDFGAddInstWithEmptyFuncCommand::newSyntax()
{
  MSyntax syntax;
  Parent::addSyntax( syntax );
  syntax.addFlag("-t", "-title", MSyntax::kString);
  syntax.addFlag("-c", "-initialCode", MSyntax::kString);
  return syntax;
}

MStatus FabricDFGAddInstWithEmptyFuncCommand::invoke(
  MArgParser &argParser,
  FabricCore::DFGExec &exec
  )
{
  if ( !argParser.isFlagSet( "title" ) )
  {
    logError( "-title not provided." );
    return MS::kFailure;
  }
  MString title = argParser.flagArgumentString( "title", 0 );

  FTL::CStrRef nodeName = exec.addInstWithNewFunc( title.asChar() );
  incCoreUndoCount();

  if ( argParser.isFlagSet( "initialCode" ) )
  {
    MString initialCode = argParser.flagArgumentString( "initialCode", 0 );
    FabricCore::DFGExec subExec = exec.getSubExec( nodeName.c_str() );
    subExec.setCode( initialCode.asChar() );
    incCoreUndoCount();
  }

  setPos( argParser, exec, nodeName );

  setResult( MString( nodeName.c_str() ) );
  return MS::kSuccess;
}

// FabricDFGAddVarCommand

MSyntax FabricDFGAddVarCommand::newSyntax()
{
  MSyntax syntax;
  Parent::addSyntax( syntax );
  syntax.addFlag("-t", "-dataType", MSyntax::kString);
  syntax.addFlag("-n", "-desiredNodeName", MSyntax::kString);
  syntax.addFlag("-ed", "-extDep", MSyntax::kString);
  return syntax;
}

MStatus FabricDFGAddVarCommand::invoke(
  MArgParser &argParser,
  FabricCore::DFGExec &exec
  )
{
  MString desiredNodeName;
  if ( argParser.isFlagSet( "desiredNodeName" ) )
    desiredNodeName = argParser.flagArgumentString( "desiredNodeName", 0 );

  if ( !argParser.isFlagSet( "dataType" ) )
  {
    logError( "-dataType not provided." );
    return MS::kFailure;
  }
  MString dataType = argParser.flagArgumentString( "dataType", 0 );

  MString extDep;
  if ( argParser.isFlagSet( "extDep" ) )
    extDep = argParser.flagArgumentString( "extDep", 0 );

  FTL::CStrRef nodeName =
    exec.addVar(
      desiredNodeName.asChar(),
      dataType.asChar(),
      extDep.asChar()
      );
  incCoreUndoCount();

  setPos( argParser, exec, nodeName );

  setResult( MString( nodeName.c_str() ) );
  return MS::kSuccess;
}

// FabricDFGAddGetCommand

MSyntax FabricDFGAddGetCommand::newSyntax()
{
  MSyntax syntax;
  Parent::addSyntax( syntax );
  syntax.addFlag("-vp", "-varPath", MSyntax::kString);
  syntax.addFlag("-n", "-desiredNodeName", MSyntax::kString);
  return syntax;
}

MStatus FabricDFGAddGetCommand::invoke(
  MArgParser &argParser,
  FabricCore::DFGExec &exec
  )
{
  MString desiredNodeName;
  if ( argParser.isFlagSet( "desiredNodeName" ) )
    desiredNodeName = argParser.flagArgumentString( "desiredNodeName", 0 );

  if ( !argParser.isFlagSet( "varPath" ) )
  {
    logError( "-varPath not provided." );
    return MS::kFailure;
  }
  MString varPath = argParser.flagArgumentString( "varPath", 0 );

  FTL::CStrRef nodeName =
    exec.addGet(
      desiredNodeName.asChar(),
      varPath.asChar()
      );
  incCoreUndoCount();

  setPos( argParser, exec, nodeName );

  setResult( MString( nodeName.c_str() ) );
  return MS::kSuccess;
}

// FabricDFGAddSetCommand

MSyntax FabricDFGAddSetCommand::newSyntax()
{
  MSyntax syntax;
  Parent::addSyntax( syntax );
  syntax.addFlag("-vp", "-varPath", MSyntax::kString);
  syntax.addFlag("-n", "-desiredNodeName", MSyntax::kString);
  return syntax;
}

MStatus FabricDFGAddSetCommand::invoke(
  MArgParser &argParser,
  FabricCore::DFGExec &exec
  )
{
  MString desiredNodeName;
  if ( argParser.isFlagSet( "desiredNodeName" ) )
    desiredNodeName = argParser.flagArgumentString( "desiredNodeName", 0 );

  if ( !argParser.isFlagSet( "varPath" ) )
  {
    logError( "-varPath not provided." );
    return MS::kFailure;
  }
  MString varPath = argParser.flagArgumentString( "varPath", 0 );

  FTL::CStrRef nodeName =
    exec.addSet(
      desiredNodeName.asChar(),
      varPath.asChar()
      );
  incCoreUndoCount();

  setPos( argParser, exec, nodeName );

  setResult( MString( nodeName.c_str() ) );
  return MS::kSuccess;
}

// FabricDFGConnectCommand

MSyntax FabricDFGConnectCommand::newSyntax()
{
  MSyntax syntax;
  Parent::addSyntax( syntax );
  syntax.addFlag("-sp", "-srcPort", MSyntax::kString);
  syntax.addFlag("-dp", "-dstPort", MSyntax::kString);
  return syntax;
}

MStatus FabricDFGConnectCommand::invoke(
  MArgParser &argParser,
  FabricCore::DFGExec &exec
  )
{
  if ( !argParser.isFlagSet( "srcPort" ) )
  {
    logError( "-srcPort not provided." );
    return MS::kFailure;
  }
  MString srcPort = argParser.flagArgumentString( "srcPort", 0 );

  if ( !argParser.isFlagSet( "dstPort" ) )
  {
    logError( "-dstPort not provided." );
    return MS::kFailure;
  }
  MString dstPort = argParser.flagArgumentString( "dstPort", 0 );

  exec.connectTo( srcPort.asChar(), dstPort.asChar() );
  incCoreUndoCount();

  return MS::kSuccess;
}

// FabricDFGDisconnectCommand

MSyntax FabricDFGDisconnectCommand::newSyntax()
{
  MSyntax syntax;
  Parent::addSyntax( syntax );
  syntax.addFlag("-sp", "-srcPort", MSyntax::kString);
  syntax.addFlag("-dp", "-dstPort", MSyntax::kString);
  return syntax;
}

MStatus FabricDFGDisconnectCommand::invoke(
  MArgParser &argParser,
  FabricCore::DFGExec &exec
  )
{
  if ( !argParser.isFlagSet( "srcPort" ) )
  {
    logError( "-srcPort not provided." );
    return MS::kFailure;
  }
  MString srcPort = argParser.flagArgumentString( "srcPort", 0 );

  if ( !argParser.isFlagSet( "dstPort" ) )
  {
    logError( "-dstPort not provided." );
    return MS::kFailure;
  }
  MString dstPort = argParser.flagArgumentString( "dstPort", 0 );

  exec.disconnectFrom( srcPort.asChar(), dstPort.asChar() );
  incCoreUndoCount();

  return MS::kSuccess;
}

// FabricDFGRemoveNodesCommand

MSyntax FabricDFGRemoveNodesCommand::newSyntax()
{
  MSyntax syntax;
  Parent::addSyntax( syntax );
  syntax.addFlag("-n", "-nodeName", MSyntax::kString);
  syntax.makeFlagMultiUse("-nodeName");
  return syntax;
}

MStatus FabricDFGRemoveNodesCommand::invoke(
  MArgParser &argParser,
  FabricCore::DFGExec &exec
  )
{
  std::vector<std::string> nodeNames;

  for ( unsigned i = 0; ; ++i )
  {
    MArgList argList;
    if ( argParser.getFlagArgumentList(
      "nodeName", i, argList
      ) != MS::kSuccess )
      break;
    MString nodeName;
    if ( argList.get( 0, nodeName ) != MS::kSuccess )
    {
      logError( "-nodeName not a string" );
      return MS::kFailure;
    }
    nodeNames.push_back( nodeName.asChar() );
  }

  for ( std::vector<std::string>::const_iterator it = nodeNames.begin();
    it != nodeNames.end(); ++it )
  {
    exec.removeNode( it->c_str() );
    incCoreUndoCount();
  }

  return MS::kSuccess;
}
