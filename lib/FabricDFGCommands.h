
#ifndef _FABRICDFGCOMMANDS_H_
#define _FABRICDFGCOMMANDS_H_

// #include "FabricSpliceEditorWidget.h"

#include <iostream>
#include <maya/MPxCommand.h>
#include <maya/MArgList.h>
#include <maya/MArgParser.h>
#include "FabricDFGBaseInterface.h"
#include <DFG/DFGController.h>
#include <DFG/DFGUICmdHandler.h>

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
    { return MString(
      FabricUI::DFG::DFGUICmdHandler::CmdName_Connect().c_str()
      ); }

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
    { return MString(
      FabricUI::DFG::DFGUICmdHandler::CmdName_Disconnect().c_str()
      ); }

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
    { return MString(
      FabricUI::DFG::DFGUICmdHandler::CmdName_RemoveNodes().c_str()
      ); }

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
    { return MString(
      FabricUI::DFG::DFGUICmdHandler::CmdName_AddInstFromPreset().c_str()
      ); }

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
    { return MString(
      FabricUI::DFG::DFGUICmdHandler::CmdName_AddInstWithEmptyGraph().c_str()
      ); }

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
    { return MString(
      FabricUI::DFG::DFGUICmdHandler::CmdName_AddInstWithEmptyFunc().c_str()
      ); }

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
    { return MString(
      FabricUI::DFG::DFGUICmdHandler::CmdName_AddVar().c_str()
      ); }

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
    { return MString(
      FabricUI::DFG::DFGUICmdHandler::CmdName_AddGet().c_str()
      ); }

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
    { return MString(
      FabricUI::DFG::DFGUICmdHandler::CmdName_AddSet().c_str()
      ); }

  static void* creator()
    { return new FabricDFGAddSetCommand; }

  static MSyntax newSyntax();

  virtual MStatus invoke(
    MArgParser &argParser,
    FabricCore::DFGExec &exec
    );
};

#endif 
