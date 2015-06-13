
#ifndef _FABRICDFGCOMMANDS_H_
#define _FABRICDFGCOMMANDS_H_

// #include "FabricSpliceEditorWidget.h"

#include <iostream>
#include <maya/MPxCommand.h>
#include <maya/MArgList.h>
#include <maya/MArgParser.h>
#include "FabricDFGCommandStack.h"
#include "FabricDFGBaseInterface.h"
#include <DFG/DFGController.h>

class FabricDFGGetContextIDCommand: public MPxCommand
{
public:

  virtual const char * getName() { return "dfgGetContextID"; }
  static void* creator();
  static MSyntax newSyntax();
  virtual MStatus doIt(const MArgList &args);
  virtual bool isUndoable() const { return false; }
};

class FabricDFGGetBindingIDCommand: public MPxCommand
{
public:

  virtual const char * getName() { return "dfgGetBindingID"; }
  static void* creator();
  static MSyntax newSyntax();
  virtual MStatus doIt(const MArgList &args);
  virtual bool isUndoable() const { return false; }
};

class FabricDFGBaseCommand: public MPxCommand
{
public:

  ~FabricDFGBaseCommand();

  virtual const char * getName() = 0;
  virtual MStatus doIt(const MArgList &args);
  virtual MStatus undoIt();
  virtual MStatus redoIt();
  virtual bool isUndoable() const;

protected:

  FabricDFGBaseInterface * getInterf();

  MString m_node;
  FabricDFGCommandStack::Info m_cmdInfo;
};

class FabricDFGRenameNodeCommand: public FabricDFGBaseCommand
{
public:

  virtual const char * getName() { return "dfgRenameNode"; }
  static void* creator();
  static MSyntax newSyntax();
  virtual MStatus doIt(const MArgList &args);
};

class FabricDFGAddPortCommand: public FabricDFGBaseCommand
{
public:

  virtual const char * getName() { return "dfgAddPort"; }
  static void* creator();
  static MSyntax newSyntax();
  virtual MStatus doIt(const MArgList &args);
};

class FabricDFGRemovePortCommand: public FabricDFGBaseCommand
{
public:

  virtual const char * getName() { return "dfgRemovePort"; }
  static void* creator();
  static MSyntax newSyntax();
  virtual MStatus doIt(const MArgList &args);
};

class FabricDFGRenamePortCommand: public FabricDFGBaseCommand
{
public:

  virtual const char * getName() { return "dfgRenamePort"; }
  static void* creator();
  static MSyntax newSyntax();
  virtual MStatus doIt(const MArgList &args);
};

class FabricDFGSetArgCommand: public FabricDFGBaseCommand
{
public:

  virtual const char * getName() { return "dfgSetArg"; }
  static void* creator();
  static MSyntax newSyntax();
  virtual MStatus doIt(const MArgList &args);
};

class FabricDFGSetDefaultValueCommand: public FabricDFGBaseCommand
{
public:

  virtual const char * getName() { return "dfgSetDefaultValue"; }
  static void* creator();
  static MSyntax newSyntax();
  virtual MStatus doIt(const MArgList &args);
};

class FabricDFGSetCodeCommand: public FabricDFGBaseCommand
{
public:

  virtual const char * getName() { return "dfgSetCode"; }
  static void* creator();
  static MSyntax newSyntax();
  virtual MStatus doIt(const MArgList &args);
};

class FabricDFGGetDescCommand: public FabricDFGBaseCommand
{
public:

  virtual const char * getName() { return "dfgGetDesc"; }
  static void* creator();
  static MSyntax newSyntax();
  virtual MStatus doIt(const MArgList &args);
};

class FabricDFGImportJSONCommand: public FabricDFGBaseCommand
{
public:

  virtual const char * getName() { return "dfgImportJSON"; }
  static void* creator();
  static MSyntax newSyntax();
  virtual MStatus doIt(const MArgList &args);
};

class FabricDFGExportJSONCommand: public FabricDFGBaseCommand
{
public:

  virtual const char * getName() { return "dfgExportJSON"; }
  static void* creator();
  static MSyntax newSyntax();
  virtual MStatus doIt(const MArgList &args);
};

class FabricDFGSetNodeCacheRuleCommand: public FabricDFGBaseCommand
{
public:

  virtual const char * getName() { return "dfgSetNodeCacheRule"; }
  static void* creator();
  static MSyntax newSyntax();
  virtual MStatus doIt(const MArgList &args);
};

class FabricDFGCopyCommand: public FabricDFGBaseCommand
{
public:

  virtual const char * getName() { return "dfgCopy"; }
  static void* creator();
  static MSyntax newSyntax();
  virtual MStatus doIt(const MArgList &args);
};

class FabricDFGPasteCommand: public FabricDFGBaseCommand
{
public:

  virtual const char * getName() { return "dfgPaste"; }
  static void* creator();
  static MSyntax newSyntax();
  virtual MStatus doIt(const MArgList &args);
};

class FabricDFGImplodeNodesCommand: public FabricDFGBaseCommand
{
public:

  virtual const char * getName() { return "dfgImplodeNodes"; }
  static void* creator();
  static MSyntax newSyntax();
  virtual MStatus doIt(const MArgList &args);
};

class FabricDFGExplodeNodeCommand: public FabricDFGBaseCommand
{
public:

  virtual const char * getName() { return "dfgExplodeNode"; }
  static void* creator();
  static MSyntax newSyntax();
  virtual MStatus doIt(const MArgList &args);
};

class FabricNewDFGBaseCommand: public MPxCommand
{
public:

  virtual MString getName() = 0;

  virtual MStatus doIt( const MArgList &args );
  virtual MStatus undoIt();
  virtual MStatus redoIt();

  virtual bool isUndoable() const
    { return true; }

protected:

  FabricNewDFGBaseCommand()
    : m_coreUndoCount( 0 )
    {}

  void incCoreUndoCount()
    { ++m_coreUndoCount; }

  FabricCore::DFGBinding &getBinding()
    { return m_binding; }

  virtual MStatus invoke(
    MArgParser &argParser,
    FabricCore::DFGBinding &binding
    ) = 0;

  void logError( MString const &desc )
    { displayError( getName() + ": " + desc, true ); }

  static void addSyntax( MSyntax &syntax );

private:

  unsigned m_coreUndoCount;
  FabricCore::DFGBinding m_binding;
};

class FabricDFGExecCommand : public FabricNewDFGBaseCommand
{
  typedef FabricNewDFGBaseCommand Parent;

protected:

  virtual MStatus invoke(
    MArgParser &argParser,
    FabricCore::DFGBinding &binding
    );

  virtual MStatus invoke(
    MArgParser &argParser,
    FabricCore::DFGExec &exec
    ) = 0;

  static void addSyntax( MSyntax &syntax );
};

class FabricDFGConnectCommand : public FabricDFGExecCommand
{
  typedef FabricDFGExecCommand Parent;
  
public:

  virtual MString getName()
    { return MString("canvasConnect"); }

  static void* creator()
    { return new FabricDFGConnectCommand; }

  static MSyntax newSyntax();

  virtual MStatus invoke(
    MArgParser &argParser,
    FabricCore::DFGExec &exec
    );
};

class FabricDFGDisconnectCommand : public FabricDFGExecCommand
{
  typedef FabricDFGExecCommand Parent;
  
public:

  virtual MString getName()
    { return MString("canvasDisconnect"); }

  static void* creator()
    { return new FabricDFGDisconnectCommand; }

  static MSyntax newSyntax();

  virtual MStatus invoke(
    MArgParser &argParser,
    FabricCore::DFGExec &exec
    );
};

class FabricDFGRemoveNodesCommand : public FabricDFGExecCommand
{
  typedef FabricDFGExecCommand Parent;
  
public:

  virtual MString getName()
    { return MString("canvasRemoveNodes"); }

  static void* creator()
    { return new FabricDFGRemoveNodesCommand; }

  static MSyntax newSyntax();

  virtual MStatus invoke(
    MArgParser &argParser,
    FabricCore::DFGExec &exec
    );
};

class FabricDFGAddNodeCommand : public FabricDFGExecCommand
{
  typedef FabricDFGExecCommand Parent;
  
protected:

  static void addSyntax( MSyntax &syntax );

  void setPos(
    MArgParser &argParser,
    FabricCore::DFGExec &exec,
    FTL::CStrRef nodeName
    );
};

class FabricDFGAddInstFromPresetCommand : public FabricDFGAddNodeCommand
{
  typedef FabricDFGAddNodeCommand Parent;
  
public:

  virtual MString getName()
    { return MString("canvasAddInstFromPreset"); }

  static void* creator()
    { return new FabricDFGAddInstFromPresetCommand; }

  static MSyntax newSyntax();

  virtual MStatus invoke(
    MArgParser &argParser,
    FabricCore::DFGExec &exec
    );
};

class FabricDFGAddInstWithEmptyGraphCommand : public FabricDFGAddNodeCommand
{
  typedef FabricDFGAddNodeCommand Parent;
  
public:

  virtual MString getName()
    { return MString("canvasAddInstWithEmptyGraph"); }

  static void* creator()
    { return new FabricDFGAddInstWithEmptyGraphCommand; }

  static MSyntax newSyntax();

  virtual MStatus invoke(
    MArgParser &argParser,
    FabricCore::DFGExec &exec
    );
};

class FabricDFGAddInstWithEmptyFuncCommand : public FabricDFGAddNodeCommand
{
  typedef FabricDFGAddNodeCommand Parent;
  
public:

  virtual MString getName()
    { return MString("canvasAddInstWithEmptyFunc"); }

  static void* creator()
    { return new FabricDFGAddInstWithEmptyFuncCommand; }

  static MSyntax newSyntax();

  virtual MStatus invoke(
    MArgParser &argParser,
    FabricCore::DFGExec &exec
    );
};

class FabricDFGAddVarCommand : public FabricDFGAddNodeCommand
{
  typedef FabricDFGAddNodeCommand Parent;
  
public:

  virtual MString getName()
    { return MString("canvasAddVar"); }

  static void* creator()
    { return new FabricDFGAddVarCommand; }

  static MSyntax newSyntax();

  virtual MStatus invoke(
    MArgParser &argParser,
    FabricCore::DFGExec &exec
    );
};

class FabricDFGAddGetCommand : public FabricDFGAddNodeCommand
{
  typedef FabricDFGAddNodeCommand Parent;
  
public:

  virtual MString getName()
    { return MString("canvasAddGet"); }

  static void* creator()
    { return new FabricDFGAddGetCommand; }

  static MSyntax newSyntax();

  virtual MStatus invoke(
    MArgParser &argParser,
    FabricCore::DFGExec &exec
    );
};

class FabricDFGAddSetCommand : public FabricDFGAddNodeCommand
{
  typedef FabricDFGAddNodeCommand Parent;
  
public:

  virtual MString getName()
    { return MString("canvasAddSet"); }

  static void* creator()
    { return new FabricDFGAddSetCommand; }

  static MSyntax newSyntax();

  virtual MStatus invoke(
    MArgParser &argParser,
    FabricCore::DFGExec &exec
    );
};

class FabricDFGAddVarCommand: public FabricDFGBaseCommand
{
public:

  virtual const char * getName() { return "dfgAddVar"; }
  static void* creator();
  static MSyntax newSyntax();
  virtual MStatus doIt(const MArgList &args);
};

class FabricDFGAddGetCommand: public FabricDFGBaseCommand
{
public:

  virtual const char * getName() { return "dfgAddGet"; }
  static void* creator();
  static MSyntax newSyntax();
  virtual MStatus doIt(const MArgList &args);
};

class FabricDFGAddSetCommand: public FabricDFGBaseCommand
{
public:

  virtual const char * getName() { return "dfgAddSet"; }
  static void* creator();
  static MSyntax newSyntax();
  virtual MStatus doIt(const MArgList &args);
};

class FabricDFGSetRefVarPathCommand: public FabricDFGBaseCommand
{
public:

  virtual const char * getName() { return "dfgSetRefVarPath"; }
  static void* creator();
  static MSyntax newSyntax();
  virtual MStatus doIt(const MArgList &args);
};

#endif 
