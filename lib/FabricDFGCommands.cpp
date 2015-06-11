#include <QtGui/QFileDialog>  // [pzion 20150519] Must include first since something below defines macros that mess it up

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

MSyntax FabricDFGAddNodeCommand::newSyntax()
{
  MSyntax syntax;
  syntax.addFlag(kNodeFlag, kNodeFlagLong, MSyntax::kString);
  syntax.addFlag("-p", "-path", MSyntax::kString);
  syntax.addFlag("-r", "-preset", MSyntax::kString);
  syntax.addFlag("-x", "-xpos", MSyntax::kDouble);
  syntax.addFlag("-y", "-ypos", MSyntax::kDouble);
  syntax.enableQuery(false);
  syntax.enableEdit(false);
  return syntax;
}

void* FabricDFGAddNodeCommand::creator()
{
  return new FabricDFGAddNodeCommand;
}

MStatus FabricDFGAddNodeCommand::doIt(const MArgList &args)
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
  if(!argData.isFlagSet("preset"))
  {
    mayaLogErrorFunc(MString(getName()) + ": Preset (-r, -preset) not provided.");
    return mayaErrorOccured();
  }

  MString path = argData.flagArgumentString("path", 0);
  MString preset = argData.flagArgumentString("preset", 0);
  float x = 0;
  float y = 0;
  
  if(argData.isFlagSet("x"))
    x = (float)argData.flagArgumentDouble("x", 0);
  if(argData.isFlagSet("y"))
    y = (float)argData.flagArgumentDouble("y", 0);

  FabricDFGCommandStack::enableMayaCommands(false);
  MString result = interf->getDFGController()->addDFGNodeFromPreset(preset.asChar(), QPointF(x, y)).c_str();
  FabricDFGCommandStack::enableMayaCommands(true);
  m_cmdInfo = FabricDFGCommandStack::consumeCommandToIgnore(getName());

  setResult(result);
  return MS::kSuccess;
}

MSyntax FabricDFGRemoveNodeCommand::newSyntax()
{
  MSyntax syntax;
  syntax.addFlag(kNodeFlag, kNodeFlagLong, MSyntax::kString);
  syntax.addFlag("-p", "-path", MSyntax::kString);
  syntax.enableQuery(false);
  syntax.enableEdit(false);
  return syntax;
}

void* FabricDFGRemoveNodeCommand::creator()
{
  return new FabricDFGRemoveNodeCommand;
}

MStatus FabricDFGRemoveNodeCommand::doIt(const MArgList &args)
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

  MString path = argData.flagArgumentString("path", 0);
  
  FabricDFGCommandStack::enableMayaCommands(false);
  interf->getDFGController()->removeNode(path.asChar());
  FabricDFGCommandStack::enableMayaCommands(true);
  m_cmdInfo = FabricDFGCommandStack::consumeCommandToIgnore(getName());

  return MS::kSuccess;
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
  interf->getDFGController()->renameNode(path.asChar(), name.asChar());
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

  MStringArray srcPathParts, dstPathParts;
  srcPath.split('.', srcPathParts);
  dstPath.split('.', dstPathParts);

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
  interf->getDFGController()->removeConnection(srcPath.asChar(), dstPath.asChar());
  FabricDFGCommandStack::enableMayaCommands(true);
  m_cmdInfo = FabricDFGCommandStack::consumeCommandToIgnore(getName());

  return MS::kSuccess;
}

MSyntax FabricDFGRemoveAllConnectionsCommand::newSyntax()
{
  MSyntax syntax;
  syntax.addFlag(kNodeFlag, kNodeFlagLong, MSyntax::kString);
  syntax.addFlag("-p", "-path", MSyntax::kString);
  syntax.addFlag("-i", "-isPin", MSyntax::kLong);
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

  MStringArray pathParts;
  path.split('.', pathParts);

  bool isPin = pathParts.length() <= pathParts.length();
  if(argData.isFlagSet("isPin"))
    isPin = argData.flagArgumentInt("isPin", 0) != 0;

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
  syntax.addFlag("-p", "-path", MSyntax::kString);
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

  MString path = argData.flagArgumentString("path", 0);
  MString name = argData.flagArgumentString("name", 0);
  MString dataType = argData.flagArgumentString("dataType", 0);
  MString portTypeStr = argData.flagArgumentString("portType", 0).toLowerCase();

  GraphView::PortType portType = GraphView::PortType_Input;
  if(portTypeStr == "in") // this refers to the side of the panel, not the port type in the DFG
    portType = GraphView::PortType_Output;
  else if(portTypeStr == "io")
    portType = GraphView::PortType_IO;

  FabricDFGCommandStack::enableMayaCommands(false);
  MString result = interf->getDFGController()->addPort(name.asChar(), portType, dataType.asChar()).c_str();
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
  interf->getDFGController()->removePort(name.asChar());
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
  MString result = interf->getDFGController()->renamePort(path.asChar(), name.asChar()).c_str();
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

  FabricCore::DFGBinding binding = interf->getDFGController()->getCoreDFGBinding();
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

  MString json;
  if(argData.isFlagSet("filePath"))
  {
    MString filePath = argData.flagArgumentString("filePath", 0);
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
    int fileSize = ftell( file );
    rewind( file );

    char * buffer = (char*) malloc(fileSize + 1);
    buffer[fileSize] = '\0';

    size_t readBytes = fread(buffer, 1, fileSize, file);
    assert(readBytes == fileSize);

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
