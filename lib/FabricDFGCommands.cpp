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

FabricDFGBaseCommand::~FabricDFGBaseCommand()
{
}

MStatus FabricDFGBaseCommand::doIt(const MArgList &args)
{
  m_cmdInfo = FabricDFGCommandStack::consumeCommandToIgnore(getName());

  MStatus status;
  MArgParser argData(syntax(), args, &status);
  if(!argData.isFlagSet("node"))
  {
    mayaLogErrorFunc(MString(getName()) + ": Node (-n, -node) not provided.");
    return mayaErrorOccured();
  }

  m_node = argData.flagArgumentString("node", 0);
  if(!getInterf())
    return MS::kNotFound;
  return MS::kSuccess;
}

bool FabricDFGBaseCommand::isUndoable() const
{
  return m_cmdInfo.undoable;
}

MStatus FabricDFGBaseCommand::undoIt()
{
  MString id;
  id.set((int)m_cmdInfo.id);
  mayaLogFunc(MString("dfg command ")+id+" undone");
  return FabricDFGCommandStack::getStack()->undo(m_cmdInfo.id) ? MS::kSuccess : MS::kNotFound;
}

MStatus FabricDFGBaseCommand::redoIt()
{
  return FabricDFGCommandStack::getStack()->redo(m_cmdInfo.id) ? MS::kSuccess : MS::kNotFound;
}

FabricDFGBaseInterface * FabricDFGBaseCommand::getInterf()
{
  FabricDFGBaseInterface * interf = FabricDFGBaseInterface::getInstanceByName(m_node.asChar());
  if(!interf)
  {
    mayaLogErrorFunc(MString(getName()) + ": Node '"+m_node+"' not found.");
  }
  return interf;
}

MSyntax FabricDFGRenameNodeCommand::newSyntax()
{
  MSyntax syntax;
  syntax.addFlag(kNodeFlag, kNodeFlagLong, MSyntax::kString);
  syntax.addFlag("-p", "-path", MSyntax::kString);
  syntax.addFlag("-n", "-name", MSyntax::kString);
  syntax.enableQuery(false);
  syntax.enableEdit(false);
  return syntax;
}

void* FabricDFGRenameNodeCommand::creator()
{
  return new FabricDFGRenameNodeCommand;
}

MStatus FabricDFGRenameNodeCommand::doIt(const MArgList &args)
{
  MStatus baseStatus = FabricDFGBaseCommand::doIt(args);
  if(baseStatus != MS::kSuccess)
    return baseStatus;
  if(m_cmdInfo.id != UINT_MAX)
    return MS::kSuccess;
  FabricDFGBaseInterface * interf = getInterf();
  if(!interf)
    return MS::kNotFound;

  MStatus status;
  MArgParser argData(syntax(), args, &status);
  if(!argData.isFlagSet("path"))
  {
    mayaLogErrorFunc(MString(getName()) + ": Path (-p, -path) not provided.");
    return mayaErrorOccured();
  }
  if(!argData.isFlagSet("name"))
  {
    mayaLogErrorFunc(MString(getName()) + ": Name (-n, -name) not provided.");
    return mayaErrorOccured();
  }

  MString path = argData.flagArgumentString("path", 0);
  MString name = argData.flagArgumentString("name", 0);
  
  FabricDFGCommandStack::enableMayaCommands(false);
  interf->getDFGController()->renameNodeByPath(path.asChar(), name.asChar());
  FabricDFGCommandStack::enableMayaCommands(true);
  m_cmdInfo = FabricDFGCommandStack::consumeCommandToIgnore(getName());

  return MS::kSuccess;
}

MSyntax FabricDFGAddEmptyFuncCommand::newSyntax()
{
  MSyntax syntax;
  syntax.addFlag(kNodeFlag, kNodeFlagLong, MSyntax::kString);
  syntax.addFlag("-p", "-path", MSyntax::kString);
  syntax.addFlag("-n", "-name", MSyntax::kString);
  syntax.addFlag("-x", "-xpos", MSyntax::kDouble);
  syntax.addFlag("-y", "-ypos", MSyntax::kDouble);
  syntax.enableQuery(false);
  syntax.enableEdit(false);
  return syntax;
}

void* FabricDFGAddEmptyFuncCommand::creator()
{
  return new FabricDFGAddEmptyFuncCommand;
}

MStatus FabricDFGAddEmptyFuncCommand::doIt(const MArgList &args)
{
  MStatus baseStatus = FabricDFGBaseCommand::doIt(args);
  if(baseStatus != MS::kSuccess)
    return baseStatus;
  if(m_cmdInfo.id != UINT_MAX)
    return MS::kSuccess;
  FabricDFGBaseInterface * interf = getInterf();
  if(!interf)
    return MS::kNotFound;

  MStatus status;
  MArgParser argData(syntax(), args, &status);
  if(!argData.isFlagSet("path"))
  {
    mayaLogErrorFunc(MString(getName()) + ": Path (-p, -path) not provided.");
    return mayaErrorOccured();
  }
  if(!argData.isFlagSet("name"))
  {
    mayaLogErrorFunc(MString(getName()) + ": Name (-n, -name) not provided.");
    return mayaErrorOccured();
  }

  MString path = argData.flagArgumentString("path", 0);
  MString name = argData.flagArgumentString("name", 0);
  float x = 0;
  float y = 0;
  
  if(argData.isFlagSet("x"))
    x = (float)argData.flagArgumentDouble("x", 0);
  if(argData.isFlagSet("y"))
    y = (float)argData.flagArgumentDouble("y", 0);

  FabricDFGCommandStack::enableMayaCommands(false);
  MString result = interf->getDFGController()->addEmptyFunc(name.asChar(), QPointF(x, y)).c_str();;
  FabricDFGCommandStack::enableMayaCommands(true);
  m_cmdInfo = FabricDFGCommandStack::consumeCommandToIgnore(getName());

  setResult(result);
  return MS::kSuccess;
}

MSyntax FabricDFGAddEmptyGraphCommand::newSyntax()
{
  MSyntax syntax;
  syntax.addFlag(kNodeFlag, kNodeFlagLong, MSyntax::kString);
  syntax.addFlag("-p", "-path", MSyntax::kString);
  syntax.addFlag("-n", "-name", MSyntax::kString);
  syntax.addFlag("-x", "-xpos", MSyntax::kDouble);
  syntax.addFlag("-y", "-ypos", MSyntax::kDouble);
  syntax.enableQuery(false);
  syntax.enableEdit(false);
  return syntax;
}

void* FabricDFGAddEmptyGraphCommand::creator()
{
  return new FabricDFGAddEmptyGraphCommand;
}

MStatus FabricDFGAddEmptyGraphCommand::doIt(const MArgList &args)
{
  MStatus baseStatus = FabricDFGBaseCommand::doIt(args);
  if(baseStatus != MS::kSuccess)
    return baseStatus;
  if(m_cmdInfo.id != UINT_MAX)
    return MS::kSuccess;
  FabricDFGBaseInterface * interf = getInterf();
  if(!interf)
    return MS::kNotFound;

  MStatus status;
  MArgParser argData(syntax(), args, &status);
  if(!argData.isFlagSet("path"))
  {
    mayaLogErrorFunc(MString(getName()) + ": Path (-p, -path) not provided.");
    return mayaErrorOccured();
  }
  if(!argData.isFlagSet("name"))
  {
    mayaLogErrorFunc(MString(getName()) + ": Name (-n, -name) not provided.");
    return mayaErrorOccured();
  }

  MString path = argData.flagArgumentString("path", 0);
  MString name = argData.flagArgumentString("name", 0);
  float x = 0;
  float y = 0;
  
  if(argData.isFlagSet("x"))
    x = (float)argData.flagArgumentDouble("x", 0);
  if(argData.isFlagSet("y"))
    y = (float)argData.flagArgumentDouble("y", 0);

  FabricDFGCommandStack::enableMayaCommands(false);
  MString result = interf->getDFGController()->addEmptyGraph(name.asChar(), QPointF(x, y)).c_str();
  FabricDFGCommandStack::enableMayaCommands(true);
  m_cmdInfo = FabricDFGCommandStack::consumeCommandToIgnore(getName());

  setResult(result);
  return MS::kSuccess;
}

MSyntax FabricDFGAddConnectionCommand::newSyntax()
{
  MSyntax syntax;
  syntax.addFlag(kNodeFlag, kNodeFlagLong, MSyntax::kString);
  syntax.addFlag("-s", "-srcPath", MSyntax::kString);
  syntax.addFlag("-d", "-dstPath", MSyntax::kString);
  syntax.enableQuery(false);
  syntax.enableEdit(false);
  return syntax;
}

void* FabricDFGAddConnectionCommand::creator()
{
  return new FabricDFGAddConnectionCommand;
}

MStatus FabricDFGAddConnectionCommand::doIt(const MArgList &args)
{
  MStatus baseStatus = FabricDFGBaseCommand::doIt(args);
  if(baseStatus != MS::kSuccess)
    return baseStatus;
  if(m_cmdInfo.id != UINT_MAX)
    return MS::kSuccess;
  FabricDFGBaseInterface * interf = getInterf();
  if(!interf)
    return MS::kNotFound;

  MStatus status;
  MArgParser argData(syntax(), args, &status);
  if(!argData.isFlagSet("srcPath"))
  {
    mayaLogErrorFunc(MString(getName()) + ": Source path (-s, -srcPath) not provided.");
    return mayaErrorOccured();
  }
  if(!argData.isFlagSet("dstPath"))
  {
    mayaLogErrorFunc(MString(getName()) + ": Destination path (-s, -dstPath) not provided.");
    return mayaErrorOccured();
  }

  MString srcPath = argData.flagArgumentString("srcPath", 0);
  MString dstPath = argData.flagArgumentString("dstPath", 0);

  FabricDFGCommandStack::enableMayaCommands(false);
  interf->getDFGController()->addConnection(srcPath.asChar(), dstPath.asChar());
  FabricDFGCommandStack::enableMayaCommands(true);
  m_cmdInfo = FabricDFGCommandStack::consumeCommandToIgnore(getName());

  return MS::kSuccess;
}

MSyntax FabricDFGRemoveConnectionCommand::newSyntax()
{
  MSyntax syntax;
  syntax.addFlag(kNodeFlag, kNodeFlagLong, MSyntax::kString);
  syntax.addFlag("-s", "-srcPath", MSyntax::kString);
  syntax.addFlag("-d", "-dstPath", MSyntax::kString);
  syntax.enableQuery(false);
  syntax.enableEdit(false);
  return syntax;
}

void* FabricDFGRemoveConnectionCommand::creator()
{
  return new FabricDFGRemoveConnectionCommand;
}

MStatus FabricDFGRemoveConnectionCommand::doIt(const MArgList &args)
{
  MStatus baseStatus = FabricDFGBaseCommand::doIt(args);
  if(baseStatus != MS::kSuccess)
    return baseStatus;
  if(m_cmdInfo.id != UINT_MAX)
    return MS::kSuccess;
  FabricDFGBaseInterface * interf = getInterf();
  if(!interf)
    return MS::kNotFound;

  MStatus status;
  MArgParser argData(syntax(), args, &status);
  if(!argData.isFlagSet("srcPath"))
  {
    mayaLogErrorFunc(MString(getName()) + ": Source path (-s, -srcPath) not provided.");
    return mayaErrorOccured();
  }
  if(!argData.isFlagSet("dstPath"))
  {
    mayaLogErrorFunc(MString(getName()) + ": Destination path (-s, -dstPath) not provided.");
    return mayaErrorOccured();
  }

  MString srcPath = argData.flagArgumentString("srcPath", 0);
  MString dstPath = argData.flagArgumentString("dstPath", 0);

  MStringArray srcPathParts, dstPathParts;
  srcPath.split('.', srcPathParts);
  dstPath.split('.', dstPathParts);

  FabricDFGCommandStack::enableMayaCommands(false);
  interf->getDFGController()->removeConnectionByPath(srcPath.asChar(), dstPath.asChar());
  FabricDFGCommandStack::enableMayaCommands(true);
  m_cmdInfo = FabricDFGCommandStack::consumeCommandToIgnore(getName());

  return MS::kSuccess;
}

MSyntax FabricDFGRemoveAllConnectionsCommand::newSyntax()
{
  MSyntax syntax;
  syntax.addFlag(kNodeFlag, kNodeFlagLong, MSyntax::kString);
  syntax.addFlag("-p", "-path", MSyntax::kString);
  syntax.enableQuery(false);
  syntax.enableEdit(false);
  return syntax;
}

void* FabricDFGRemoveAllConnectionsCommand::creator()
{
  return new FabricDFGRemoveAllConnectionsCommand;
}

MStatus FabricDFGRemoveAllConnectionsCommand::doIt(const MArgList &args)
{
  MStatus baseStatus = FabricDFGBaseCommand::doIt(args);
  if(baseStatus != MS::kSuccess)
    return baseStatus;
  if(m_cmdInfo.id != UINT_MAX)
    return MS::kSuccess;
  FabricDFGBaseInterface * interf = getInterf();
  if(!interf)
    return MS::kNotFound;

  MStatus status;
  MArgParser argData(syntax(), args, &status);
  if(!argData.isFlagSet("path"))
  {
    mayaLogErrorFunc(MString(getName()) + ": Source path (-p, -path) not provided.");
    return mayaErrorOccured();
  }

  MString path = argData.flagArgumentString("path", 0);

  FabricDFGCommandStack::enableMayaCommands(false);
  interf->getDFGController()->removeAllConnections(path.asChar());
  FabricDFGCommandStack::enableMayaCommands(true);
  m_cmdInfo = FabricDFGCommandStack::consumeCommandToIgnore(getName());

  return MS::kSuccess;
}

MSyntax FabricDFGAddPortCommand::newSyntax()
{
  MSyntax syntax;
  syntax.addFlag(kNodeFlag, kNodeFlagLong, MSyntax::kString);
  syntax.addFlag("-p", "-execPath", MSyntax::kString);
  syntax.addFlag("-n", "-name", MSyntax::kString);
  syntax.addFlag("-d", "-dataType", MSyntax::kString);
  syntax.addFlag("-t", "-portType", MSyntax::kString);
  syntax.enableQuery(false);
  syntax.enableEdit(false);
  return syntax;
}

void* FabricDFGAddPortCommand::creator()
{
  return new FabricDFGAddPortCommand;
}

MStatus FabricDFGAddPortCommand::doIt(const MArgList &args)
{
  MStatus baseStatus = FabricDFGBaseCommand::doIt(args);
  if(baseStatus != MS::kSuccess)
    return baseStatus;
  if(m_cmdInfo.id != UINT_MAX)
    return MS::kSuccess;
  FabricDFGBaseInterface * interf = getInterf();
  if(!interf)
    return MS::kNotFound;

  MStatus status;
  MArgParser argData(syntax(), args, &status);
  if(!argData.isFlagSet("execPath"))
  {
    mayaLogErrorFunc(MString(getName()) + ": Path (-p, -execPath) not provided.");
    return mayaErrorOccured();
  }
  if(!argData.isFlagSet("name"))
  {
    mayaLogErrorFunc(MString(getName()) + ": Name (-p, -name) not provided.");
    return mayaErrorOccured();
  }
  if(!argData.isFlagSet("dataType"))
  {
    mayaLogErrorFunc(MString(getName()) + ": Data type (-s, -dataType) not provided.");
    return mayaErrorOccured();
  }
  if(!argData.isFlagSet("portType"))
  {
    mayaLogErrorFunc(MString(getName()) + ": Port type (-s, -portType) not provided.");
    return mayaErrorOccured();
  }

  MString execPath = argData.flagArgumentString("execPath", 0);
  MString name = argData.flagArgumentString("name", 0);
  MString dataType = argData.flagArgumentString("dataType", 0);
  MString portTypeStr = argData.flagArgumentString("portType", 0).toLowerCase();

  GraphView::PortType portType = GraphView::PortType_Input;
  if(portTypeStr == "in") // this refers to the side of the panel, not the port type in the DFG
    portType = GraphView::PortType_Output;
  else if(portTypeStr == "io")
    portType = GraphView::PortType_IO;

  FabricDFGCommandStack::enableMayaCommands(false);
  MString result = interf->getDFGController()->addPortByPath(execPath.asChar(), name.asChar(), portType, dataType.asChar()).c_str();
  FabricDFGCommandStack::enableMayaCommands(true);
  m_cmdInfo = FabricDFGCommandStack::consumeCommandToIgnore(getName());

  setResult(result);
  return MS::kSuccess;
}

MSyntax FabricDFGRemovePortCommand::newSyntax()
{
  MSyntax syntax;
  syntax.addFlag(kNodeFlag, kNodeFlagLong, MSyntax::kString);
  syntax.addFlag("-p", "-path", MSyntax::kString);
  syntax.addFlag("-n", "-name", MSyntax::kString);
  syntax.enableQuery(false);
  syntax.enableEdit(false);
  return syntax;
}

void* FabricDFGRemovePortCommand::creator()
{
  return new FabricDFGRemovePortCommand;
}

MStatus FabricDFGRemovePortCommand::doIt(const MArgList &args)
{
  MStatus baseStatus = FabricDFGBaseCommand::doIt(args);
  if(baseStatus != MS::kSuccess)
    return baseStatus;
  if(m_cmdInfo.id != UINT_MAX)
    return MS::kSuccess;
  FabricDFGBaseInterface * interf = getInterf();
  if(!interf)
    return MS::kNotFound;

  MStatus status;
  MArgParser argData(syntax(), args, &status);
  if(!argData.isFlagSet("path"))
  {
    mayaLogErrorFunc(MString(getName()) + ": Path (-p, -path) not provided.");
    return mayaErrorOccured();
  }
  if(!argData.isFlagSet("name"))
  {
    mayaLogErrorFunc(MString(getName()) + ": Name (-p, -name) not provided.");
    return mayaErrorOccured();
  }

  MString path = argData.flagArgumentString("path", 0);
  MString name = argData.flagArgumentString("name", 0);

  FabricDFGCommandStack::enableMayaCommands(false);
  interf->getDFGController()->removePortByName(name.asChar());
  FabricDFGCommandStack::enableMayaCommands(true);
  m_cmdInfo = FabricDFGCommandStack::consumeCommandToIgnore(getName());

  return MS::kSuccess;
}

MSyntax FabricDFGRenamePortCommand::newSyntax()
{
  MSyntax syntax;
  syntax.addFlag(kNodeFlag, kNodeFlagLong, MSyntax::kString);
  syntax.addFlag("-p", "-path", MSyntax::kString);
  syntax.addFlag("-n", "-name", MSyntax::kString);
  syntax.enableQuery(false);
  syntax.enableEdit(false);
  return syntax;
}

void* FabricDFGRenamePortCommand::creator()
{
  return new FabricDFGRenamePortCommand;
}

MStatus FabricDFGRenamePortCommand::doIt(const MArgList &args)
{
  MStatus baseStatus = FabricDFGBaseCommand::doIt(args);
  if(baseStatus != MS::kSuccess)
    return baseStatus;
  if(m_cmdInfo.id != UINT_MAX)
    return MS::kSuccess;
  FabricDFGBaseInterface * interf = getInterf();
  if(!interf)
    return MS::kNotFound;

  MStatus status;
  MArgParser argData(syntax(), args, &status);
  if(!argData.isFlagSet("path"))
  {
    mayaLogErrorFunc(MString(getName()) + ": Path (-p, -path) not provided.");
    return mayaErrorOccured();
  }
  if(!argData.isFlagSet("name"))
  {
    mayaLogErrorFunc(MString(getName()) + ": Name (-p, -name) not provided.");
    return mayaErrorOccured();
  }

  MString path = argData.flagArgumentString("path", 0);
  MString name = argData.flagArgumentString("name", 0);

  FabricDFGCommandStack::enableMayaCommands(false);
  MString result = interf->getDFGController()->renamePortByPath(path.asChar(), name.asChar()).c_str();
  FabricDFGCommandStack::enableMayaCommands(true);
  m_cmdInfo = FabricDFGCommandStack::consumeCommandToIgnore(getName());

  return MS::kSuccess;
}

MSyntax FabricDFGSetArgCommand::newSyntax()
{
  MSyntax syntax;
  syntax.addFlag(kNodeFlag, kNodeFlagLong, MSyntax::kString);
  syntax.addFlag("-p", "-path", MSyntax::kString);
  syntax.addFlag("-d", "-dataType", MSyntax::kString);
  syntax.addFlag("-j", "-json", MSyntax::kString);
  syntax.enableQuery(false);
  syntax.enableEdit(false);
  return syntax;
}

void* FabricDFGSetArgCommand::creator()
{
  return new FabricDFGSetArgCommand;
}

MStatus FabricDFGSetArgCommand::doIt(const MArgList &args)
{
  MStatus baseStatus = FabricDFGBaseCommand::doIt(args);
  if(baseStatus != MS::kSuccess)
    return baseStatus;
  if(m_cmdInfo.id != UINT_MAX)
    return MS::kSuccess;
  FabricDFGBaseInterface * interf = getInterf();
  if(!interf)
    return MS::kNotFound;

  MStatus status;
  MArgParser argData(syntax(), args, &status);
  if(!argData.isFlagSet("path"))
  {
    mayaLogErrorFunc(MString(getName()) + ": Path (-p, -path) not provided.");
    return mayaErrorOccured();
  }
  if(!argData.isFlagSet("dataType"))
  {
    mayaLogErrorFunc(MString(getName()) + ": DataType (-d, -dataType) not provided.");
    return mayaErrorOccured();
  }

  MString path = argData.flagArgumentString("path", 0);
  MString dataType = argData.flagArgumentString("dataType", 0);
  MString json;
  const char * jsonPtr = NULL;
  if(argData.isFlagSet("json"))
  {
    json = argData.flagArgumentString("json", 0);
    jsonPtr = json.asChar();
  }

  FabricDFGCommandStack::enableMayaCommands(false);
  interf->getDFGController()->setArg(path.asChar(), dataType.asChar(), jsonPtr);
  FabricDFGCommandStack::enableMayaCommands(true);
  m_cmdInfo = FabricDFGCommandStack::consumeCommandToIgnore(getName());

  return MS::kSuccess;
}

MSyntax FabricDFGSetDefaultValueCommand::newSyntax()
{
  MSyntax syntax;
  syntax.addFlag(kNodeFlag, kNodeFlagLong, MSyntax::kString);
  syntax.addFlag("-p", "-path", MSyntax::kString);
  syntax.addFlag("-d", "-dataType", MSyntax::kString);
  syntax.addFlag("-j", "-json", MSyntax::kString);
  syntax.enableQuery(false);
  syntax.enableEdit(false);
  return syntax;
}

void* FabricDFGSetDefaultValueCommand::creator()
{
  return new FabricDFGSetDefaultValueCommand;
}

MStatus FabricDFGSetDefaultValueCommand::doIt(const MArgList &args)
{
  MStatus baseStatus = FabricDFGBaseCommand::doIt(args);
  if(baseStatus != MS::kSuccess)
    return baseStatus;
  if(m_cmdInfo.id != UINT_MAX)
    return MS::kSuccess;
  FabricDFGBaseInterface * interf = getInterf();
  if(!interf)
    return MS::kNotFound;

  MStatus status;
  MArgParser argData(syntax(), args, &status);
  if(!argData.isFlagSet("path"))
  {
    mayaLogErrorFunc(MString(getName()) + ": Path (-p, -path) not provided.");
    return mayaErrorOccured();
  }
  if(!argData.isFlagSet("dataType"))
  {
    mayaLogErrorFunc(MString(getName()) + ": DataType (-d, -dataType) not provided.");
    return mayaErrorOccured();
  }
  if(!argData.isFlagSet("json"))
  {
    mayaLogErrorFunc(MString(getName()) + ": Json (-j, -json) not provided.");
    return mayaErrorOccured();
  }

  MString path = argData.flagArgumentString("path", 0);
  MString dataType = argData.flagArgumentString("dataType", 0);
  MString json = argData.flagArgumentString("json", 0);

  FabricDFGCommandStack::enableMayaCommands(false);
  interf->getDFGController()->setDefaultValue(path.asChar(), dataType.asChar(), json.asChar());
  FabricDFGCommandStack::enableMayaCommands(true);
  m_cmdInfo = FabricDFGCommandStack::consumeCommandToIgnore(getName());

  return MS::kSuccess;
}

MSyntax FabricDFGSetCodeCommand::newSyntax()
{
  MSyntax syntax;
  syntax.addFlag(kNodeFlag, kNodeFlagLong, MSyntax::kString);
  syntax.addFlag("-p", "-path", MSyntax::kString);
  syntax.addFlag("-c", "-code", MSyntax::kString);
  syntax.enableQuery(false);
  syntax.enableEdit(false);
  return syntax;
}

void* FabricDFGSetCodeCommand::creator()
{
  return new FabricDFGSetCodeCommand;
}

MStatus FabricDFGSetCodeCommand::doIt(const MArgList &args)
{
  MStatus baseStatus = FabricDFGBaseCommand::doIt(args);
  if(baseStatus != MS::kSuccess)
    return baseStatus;
  if(m_cmdInfo.id != UINT_MAX)
    return MS::kSuccess;
  FabricDFGBaseInterface * interf = getInterf();
  if(!interf)
    return MS::kNotFound;

  MStatus status;
  MArgParser argData(syntax(), args, &status);
  if(!argData.isFlagSet("path"))
  {
    mayaLogErrorFunc(MString(getName()) + ": Path (-p, -path) not provided.");
    return mayaErrorOccured();
  }
  if(!argData.isFlagSet("code"))
  {
    mayaLogErrorFunc(MString(getName()) + ": Code (-c, -code) not provided.");
    return mayaErrorOccured();
  }

  MString path = argData.flagArgumentString("path", 0);
  MString code = argData.flagArgumentString("code", 0);

  FabricDFGCommandStack::enableMayaCommands(false);
  interf->getDFGController()->setCode(path.asChar(), code.asChar());
  FabricDFGCommandStack::enableMayaCommands(true);
  m_cmdInfo = FabricDFGCommandStack::consumeCommandToIgnore(getName());

  return MS::kSuccess;
}

MSyntax FabricDFGGetDescCommand::newSyntax()
{
  MSyntax syntax;
  syntax.addFlag(kNodeFlag, kNodeFlagLong, MSyntax::kString);
  syntax.enableQuery(false);
  syntax.enableEdit(false);
  return syntax;
}

void* FabricDFGGetDescCommand::creator()
{
  return new FabricDFGGetDescCommand;
}

MStatus FabricDFGGetDescCommand::doIt(const MArgList &args)
{
  MStatus baseStatus = FabricDFGBaseCommand::doIt(args);
  if(baseStatus != MS::kSuccess)
    return baseStatus;
  if(m_cmdInfo.id != UINT_MAX)
    return MS::kSuccess;
  FabricDFGBaseInterface * interf = getInterf();
  if(!interf)
    return MS::kNotFound;

  MStatus status;
  MArgParser argData(syntax(), args, &status);

  FabricDFGCommandStack::enableMayaCommands(false);

  FabricCore::DFGBinding binding = interf->getDFGController()->getBinding();
  FabricCore::DFGExec exec = binding.getExec();

  MString result = exec.getDesc().getCString();

  FabricDFGCommandStack::enableMayaCommands(true);
  
  setResult(result);

  return MS::kSuccess;
}

MSyntax FabricDFGImportJSONCommand::newSyntax()
{
  MSyntax syntax;
  syntax.addFlag(kNodeFlag, kNodeFlagLong, MSyntax::kString);
  syntax.addFlag("-p", "-path", MSyntax::kString);
  syntax.addFlag("-f", "-filePath", MSyntax::kString);
  syntax.addFlag("-j", "-json", MSyntax::kString);
  syntax.addFlag("-r", "-referenced", MSyntax::kBoolean);
  syntax.enableQuery(false);
  syntax.enableEdit(false);
  return syntax;
}

void* FabricDFGImportJSONCommand::creator()
{
  return new FabricDFGImportJSONCommand;
}

MStatus FabricDFGImportJSONCommand::doIt(const MArgList &args)
{
  MStatus baseStatus = FabricDFGBaseCommand::doIt(args);
  if(baseStatus != MS::kSuccess)
    return baseStatus;
  if(m_cmdInfo.id != UINT_MAX)
    return MS::kSuccess;
  FabricDFGBaseInterface * interf = getInterf();
  if(!interf)
    return MS::kNotFound;

  MStatus status;
  MArgParser argData(syntax(), args, &status);

  MString path; // for now we aren't using this.
  if(argData.isFlagSet("path"))
    path = argData.flagArgumentString("path", 0);

  if(!argData.isFlagSet("filePath") && !argData.isFlagSet("json"))
  {
    mayaLogErrorFunc(MString(getName()) + ": Either File path (-f, -filePath) or JSON (-j, -json) has to be provided.");
    return MS::kNotFound;
  }

  bool asReferenced = false;
  MString filePath;

  MString json;
  if(argData.isFlagSet("filePath"))
  {
    if(argData.isFlagSet("referenced"))
      asReferenced = argData.flagArgumentBool("referenced", 0);

    filePath = argData.flagArgumentString("filePath", 0);
    if(filePath.length() == 0)
    {
      QString qFileName = QFileDialog::getOpenFileName( 
        MQtUtil::mainWindow(), 
        "Choose DFG file", 
        QDir::currentPath(), 
        "DFG files (*.dfg.json);;All files (*.*)"
      );

      if(qFileName.isNull())
      {
        mayaLogErrorFunc("No filename specified.");
        return mayaErrorOccured();
      }

      filePath = qFileName.toUtf8().constData();      
    }

    FILE * file = fopen(filePath.asChar(), "rb");
    if(!file)
    {
      mayaLogErrorFunc(MString(getName()) + ": File path (-f, -filePath) '"+filePath+"' cannot be found.");
      return MS::kNotFound;
    }

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
  else if(argData.isFlagSet("json"))
  {
    json = argData.flagArgumentString("json", 0);
  }

  FabricDFGCommandStack::enableMayaCommands(false);
  interf->restoreFromJSON(json);
  if(asReferenced)
    interf->setReferencedFilePath(filePath);
  FabricDFGCommandStack::enableMayaCommands(true);
  
  // this command isn't issued through the UI
  // m_cmdInfo = FabricDFGCommandStack::consumeCommandToIgnore(getName());
  
  return MS::kSuccess;
}

MSyntax FabricDFGExportJSONCommand::newSyntax()
{
  MSyntax syntax;
  syntax.addFlag(kNodeFlag, kNodeFlagLong, MSyntax::kString);
  syntax.addFlag("-p", "-path", MSyntax::kString);
  syntax.addFlag("-f", "-filePath", MSyntax::kString);
  syntax.enableQuery(false);
  syntax.enableEdit(false);
  return syntax;
}

void* FabricDFGExportJSONCommand::creator()
{
  return new FabricDFGExportJSONCommand;
}

MStatus FabricDFGExportJSONCommand::doIt(const MArgList &args)
{
  MStatus baseStatus = FabricDFGBaseCommand::doIt(args);
  if(baseStatus != MS::kSuccess)
    return baseStatus;
  if(m_cmdInfo.id != UINT_MAX)
    return MS::kSuccess;
  FabricDFGBaseInterface * interf = getInterf();
  if(!interf)
    return MS::kNotFound;

  MStatus status;
  MArgParser argData(syntax(), args, &status);

  MString path;
  if(argData.isFlagSet("path"))
    path = argData.flagArgumentString("path", 0);
  MString filePath;
  if(argData.isFlagSet("filePath"))
  {
    filePath = argData.flagArgumentString("filePath", 0);
    if(filePath.length() == 0)
    {
      QString qFileName = QFileDialog::getSaveFileName( 
        MQtUtil::mainWindow(), 
        "Choose DFG file", 
        QDir::currentPath(), 
        "DFG files (*.dfg.json);;All files (*.*)"
      );

      if(qFileName.isNull())
      {
        mayaLogErrorFunc("No filename specified.");
        return mayaErrorOccured();
      }
      
      filePath = qFileName.toUtf8().constData();      
    }
  }

  FabricDFGCommandStack::enableMayaCommands(false);
  MString json = interf->getDFGController()->exportJSON(path.asChar()).c_str();
  FabricDFGCommandStack::enableMayaCommands(true);
  
  // this command isn't issued through the UI
  // m_cmdInfo = FabricDFGCommandStack::consumeCommandToIgnore(getName());
  
  setResult(json);

  // export to file
  if(filePath.length() > 0)
  {
    FILE * file = fopen(filePath.asChar(), "wb");
    if(!file)
    {
      mayaLogErrorFunc(MString(getName()) + ": File path (-f, -filePath) '"+filePath+"' is not accessible.");
    }
    else
    {
      fwrite(json.asChar(), json.length(), 1, file);
      fclose(file);
    }
  }

  return MS::kSuccess;
}

MSyntax FabricDFGReloadJSONCommand::newSyntax()
{
  MSyntax syntax;
  syntax.addFlag(kNodeFlag, kNodeFlagLong, MSyntax::kString);
  syntax.enableQuery(false);
  syntax.enableEdit(false);
  return syntax;
}

void* FabricDFGReloadJSONCommand::creator()
{
  return new FabricDFGReloadJSONCommand;
}

MStatus FabricDFGReloadJSONCommand::doIt(const MArgList &args)
{
  MStatus baseStatus = FabricDFGBaseCommand::doIt(args);
  if(baseStatus != MS::kSuccess)
    return baseStatus;
  if(m_cmdInfo.id != UINT_MAX)
    return MS::kSuccess;
  FabricDFGBaseInterface * interf = getInterf();
  if(!interf)
    return MS::kNotFound;

  FabricDFGCommandStack::enableMayaCommands(false);
  interf->reloadFromReferencedFilePath();
  FabricDFGCommandStack::enableMayaCommands(true);

  return MS::kSuccess;
}

MSyntax FabricDFGSetNodeCacheRuleCommand::newSyntax()
{
  MSyntax syntax;
  syntax.addFlag(kNodeFlag, kNodeFlagLong, MSyntax::kString);
  syntax.addFlag("-p", "-path", MSyntax::kString);
  syntax.addFlag("-r", "-cacheRule", MSyntax::kString);
  syntax.enableQuery(false);
  syntax.enableEdit(false);
  return syntax;
}

void* FabricDFGSetNodeCacheRuleCommand::creator()
{
  return new FabricDFGSetNodeCacheRuleCommand;
}

MStatus FabricDFGSetNodeCacheRuleCommand::doIt(const MArgList &args)
{
  MStatus baseStatus = FabricDFGBaseCommand::doIt(args);
  if(baseStatus != MS::kSuccess)
    return baseStatus;
  if(m_cmdInfo.id != UINT_MAX)
    return MS::kSuccess;
  FabricDFGBaseInterface * interf = getInterf();
  if(!interf)
    return MS::kNotFound;

  MStatus status;
  MArgParser argData(syntax(), args, &status);
  if(!argData.isFlagSet("path"))
  {
    mayaLogErrorFunc(MString(getName()) + ": Path (-p, -path) not provided.");
    return mayaErrorOccured();
  }
  if(!argData.isFlagSet("cacheRule"))
  {
    mayaLogErrorFunc(MString(getName()) + ": CacheRule (-r, -cacheRule) not provided.");
    return mayaErrorOccured();
  }

  MString path = argData.flagArgumentString("path", 0);
  MString cacheRuleName = argData.flagArgumentString("cacheRule", 0);

  FEC_DFGCacheRule cacheRule = FEC_DFGCacheRule_Unspecified;
  if(cacheRuleName ==  "Always")
    cacheRule = FEC_DFGCacheRule_Always;
  if(cacheRuleName == "Never")
    cacheRule = FEC_DFGCacheRule_Never;
  
  FabricDFGCommandStack::enableMayaCommands(false);
  interf->getDFGController()->setNodeCacheRule(path.asChar(), cacheRule);
  FabricDFGCommandStack::enableMayaCommands(true);
  m_cmdInfo = FabricDFGCommandStack::consumeCommandToIgnore(getName());

  return MS::kSuccess;
}

MSyntax FabricDFGCopyCommand::newSyntax()
{
  MSyntax syntax;
  syntax.addFlag(kNodeFlag, kNodeFlagLong, MSyntax::kString);
  syntax.addFlag("-p", "-paths", MSyntax::kString);
  syntax.enableQuery(false);
  syntax.enableEdit(false);
  return syntax;
}

void* FabricDFGCopyCommand::creator()
{
  return new FabricDFGCopyCommand;
}

MStatus FabricDFGCopyCommand::doIt(const MArgList &args)
{
  MStatus baseStatus = FabricDFGBaseCommand::doIt(args);
  if(baseStatus != MS::kSuccess)
    return baseStatus;
  if(m_cmdInfo.id != UINT_MAX)
    return MS::kSuccess;
  FabricDFGBaseInterface * interf = getInterf();
  if(!interf)
    return MS::kNotFound;

  MStatus status;
  MArgParser argData(syntax(), args, &status);
  if(!argData.isFlagSet("paths"))
  {
    mayaLogErrorFunc(MString(getName()) + ": Paths (-p, -paths) not provided.");
    return mayaErrorOccured();
  }

  QString paths = argData.flagArgumentString("paths", 0).asChar();
  
  FabricDFGCommandStack::enableMayaCommands(false);
  interf->getDFGController()->copy(paths.split('.'));
  FabricDFGCommandStack::enableMayaCommands(true);
  m_cmdInfo = FabricDFGCommandStack::consumeCommandToIgnore(getName());

  return MS::kSuccess;
}

MSyntax FabricDFGPasteCommand::newSyntax()
{
  MSyntax syntax;
  syntax.addFlag(kNodeFlag, kNodeFlagLong, MSyntax::kString);
  syntax.enableQuery(false);
  syntax.enableEdit(false);
  return syntax;
}

void* FabricDFGPasteCommand::creator()
{
  return new FabricDFGPasteCommand;
}

MStatus FabricDFGPasteCommand::doIt(const MArgList &args)
{
  MStatus baseStatus = FabricDFGBaseCommand::doIt(args);
  if(baseStatus != MS::kSuccess)
    return baseStatus;
  if(m_cmdInfo.id != UINT_MAX)
    return MS::kSuccess;
  FabricDFGBaseInterface * interf = getInterf();
  if(!interf)
    return MS::kNotFound;

  MStatus status;
  
  FabricDFGCommandStack::enableMayaCommands(false);
  interf->getDFGController()->paste();
  FabricDFGCommandStack::enableMayaCommands(true);
  m_cmdInfo = FabricDFGCommandStack::consumeCommandToIgnore(getName());

  return MS::kSuccess;
}

MSyntax FabricDFGImplodeNodesCommand::newSyntax()
{
  MSyntax syntax;
  syntax.addFlag(kNodeFlag, kNodeFlagLong, MSyntax::kString);
  syntax.addFlag("-n", "-name", MSyntax::kString);
  syntax.addFlag("-p", "-paths", MSyntax::kString);
  syntax.enableQuery(false);
  syntax.enableEdit(false);
  return syntax;
}

void* FabricDFGImplodeNodesCommand::creator()
{
  return new FabricDFGImplodeNodesCommand;
}

MStatus FabricDFGImplodeNodesCommand::doIt(const MArgList &args)
{
  MStatus baseStatus = FabricDFGBaseCommand::doIt(args);
  if(baseStatus != MS::kSuccess)
    return baseStatus;
  if(m_cmdInfo.id != UINT_MAX)
    return MS::kSuccess;
  FabricDFGBaseInterface * interf = getInterf();
  if(!interf)
    return MS::kNotFound;

  MStatus status;
  MArgParser argData(syntax(), args, &status);
  if(!argData.isFlagSet("name"))
  {
    mayaLogErrorFunc(MString(getName()) + ": Desired name (-n, -name) not provided.");
    return mayaErrorOccured();
  }
  if(!argData.isFlagSet("paths"))
  {
    mayaLogErrorFunc(MString(getName()) + ": Paths (-p, -paths) not provided.");
    return mayaErrorOccured();
  }

  MString name = argData.flagArgumentString("name", 0);
  QString paths = argData.flagArgumentString("paths", 0).asChar();
  
  FabricDFGCommandStack::enableMayaCommands(false);
  interf->getDFGController()->implodeNodes(name.asChar(), paths.split('.'));
  FabricDFGCommandStack::enableMayaCommands(true);
  m_cmdInfo = FabricDFGCommandStack::consumeCommandToIgnore(getName());

  return MS::kSuccess;
}

MSyntax FabricDFGExplodeNodeCommand::newSyntax()
{
  MSyntax syntax;
  syntax.addFlag(kNodeFlag, kNodeFlagLong, MSyntax::kString);
  syntax.addFlag("-p", "-path", MSyntax::kString);
  syntax.enableQuery(false);
  syntax.enableEdit(false);
  return syntax;
}

void* FabricDFGExplodeNodeCommand::creator()
{
  return new FabricDFGExplodeNodeCommand;
}

MStatus FabricDFGExplodeNodeCommand::doIt(const MArgList &args)
{
  MStatus baseStatus = FabricDFGBaseCommand::doIt(args);
  if(baseStatus != MS::kSuccess)
    return baseStatus;
  if(m_cmdInfo.id != UINT_MAX)
    return MS::kSuccess;
  FabricDFGBaseInterface * interf = getInterf();
  if(!interf)
    return MS::kNotFound;

  MStatus status;
  MArgParser argData(syntax(), args, &status);
  if(!argData.isFlagSet("path"))
  {
    mayaLogErrorFunc(MString(getName()) + ": Path (-p, -path) not provided.");
    return mayaErrorOccured();
  }

  MString path = argData.flagArgumentString("paths", 0).asChar();
  
  FabricDFGCommandStack::enableMayaCommands(false);
  interf->getDFGController()->explodeNode(path.asChar());
  FabricDFGCommandStack::enableMayaCommands(true);
  m_cmdInfo = FabricDFGCommandStack::consumeCommandToIgnore(getName());

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
}

MStatus FabricNewDFGBaseCommand::redoIt()
{
  FabricCore::DFGHost host = getBinding().getHost();
  for ( unsigned i = 0; i < m_coreUndoCount; ++i )
    host.maybeRedo();
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
