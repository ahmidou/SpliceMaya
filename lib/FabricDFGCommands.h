
#ifndef _FABRICDFGCOMMANDS_H_
#define _FABRICDFGCOMMANDS_H_

// #include "FabricSpliceEditorWidget.h"

#include <iostream>
#include <maya/MPxCommand.h>
#include <maya/MArgList.h>
#include <maya/MArgParser.h>
#include "FabricDFGBaseInterface.h"
#include <DFG/DFGController.h>
#include <DFG/DFGUICmd/DFGUICmds.h>
#include <DFG/DFGUICmdHandler.h>
#include <FTL/OwnedPtr.h>

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

  FabricNewDFGBaseCommand() {}

  virtual FabricUI::DFG::DFGUICmd *executeDFGUICmd(
    MArgParser &argParser
    ) = 0;

  void logError( MString const &desc )
    { displayError( getName() + ": " + desc, true ); }

  static void addSyntax( MSyntax &syntax );

  class ArgException
  {
  public:

    ArgException( MStatus status, MString const &mString )
      : m_status( status )
      , m_desc( mString )
      {}

    MStatus getStatus()
      { return m_status; }

    MString const &getDesc()
      { return m_desc; }
  
  private:

    MStatus m_status;
    MString m_desc;
  };

private:

  FTL::OwnedPtr<FabricUI::DFG::DFGUICmd> m_dfgUICmd;
};

class FabricDFGBindingCommand : public FabricNewDFGBaseCommand
{
  typedef FabricNewDFGBaseCommand Parent;

protected:

  static void addSyntax( MSyntax &syntax );

  struct Args
  {
    FabricCore::DFGBinding binding;
  };

  void getArgs(
    MArgParser &argParser,
    Args &args
    );
};

class FabricDFGExecCommand : public FabricDFGBindingCommand
{
  typedef FabricDFGBindingCommand Parent;

protected:

  static void addSyntax( MSyntax &syntax );

  struct Args : Parent::Args
  {
    FTL::StrRef execPath;
    FabricCore::DFGExec exec;
  };

  void getArgs(
    MArgParser &argParser,
    Args &args
    );
};

class FabricDFGConnectCommand : public FabricDFGExecCommand
{
  typedef FabricDFGExecCommand Parent;
  
public:

  virtual MString getName()
    { return MString(
      FabricUI::DFG::DFGUICmd_Connect::CmdName().c_str()
      ); }

  static void* creator()
    { return new FabricDFGConnectCommand; }

  static MSyntax newSyntax()
  {
    MSyntax syntax;
    addSyntax( syntax );
    return syntax;
  }

protected:

  static void addSyntax( MSyntax &syntax );

  struct Args : Parent::Args
  {
    FTL::StrRef srcPort;
    FTL::StrRef dstPort;
  };

  void getArgs( MArgParser &argParser, Args &args );

  virtual FabricUI::DFG::DFGUICmd *executeDFGUICmd(
    MArgParser &argParser
    );
};

class FabricDFGMoveNodesCommand : public FabricDFGExecCommand
{
  typedef FabricDFGExecCommand Parent;
  
public:

  virtual MString getName()
    { return MString(
      FabricUI::DFG::DFGUICmd_MoveNodes::CmdName().c_str()
      ); }

  static void* creator()
    { return new FabricDFGMoveNodesCommand; }

  static MSyntax newSyntax()
  {
    MSyntax syntax;
    addSyntax( syntax );
    return syntax;
  }

protected:

  static void addSyntax( MSyntax &syntax );

  struct Args : Parent::Args
  {
    std::vector<FTL::StrRef> nodeNames;
    std::vector<QPointF> poss;
  };

  void getArgs( MArgParser &argParser, Args &args );

  virtual FabricUI::DFG::DFGUICmd *executeDFGUICmd(
    MArgParser &argParser
    );
};

class FabricDFGAddPortCommand : public FabricDFGExecCommand
{
  typedef FabricDFGExecCommand Parent;
  
public:

  virtual MString getName()
    { return MString(
      FabricUI::DFG::DFGUICmd_AddPort::CmdName().c_str()
      ); }

  static void* creator()
    { return new FabricDFGAddPortCommand; }

  static MSyntax newSyntax()
  {
    MSyntax syntax;
    addSyntax( syntax );
    return syntax;
  }

protected:

  static void addSyntax( MSyntax &syntax );

  struct Args : Parent::Args
  {
    FTL::StrRef desiredPortName;
    FabricCore::DFGPortType portType;
    FTL::StrRef typeSpec;
    FTL::StrRef portToConnectWith;
  };

  void getArgs( MArgParser &argParser, Args &args );

  virtual FabricUI::DFG::DFGUICmd *executeDFGUICmd(
    MArgParser &argParser
    );
};

class FabricDFGSetArgTypeCommand : public FabricDFGBindingCommand
{
  typedef FabricDFGBindingCommand Parent;
  
public:

  virtual MString getName()
    { return MString(
      FabricUI::DFG::DFGUICmd_SetArgType::CmdName().c_str()
      ); }

  static void* creator()
    { return new FabricDFGSetArgTypeCommand; }

  static MSyntax newSyntax()
  {
    MSyntax syntax;
    addSyntax( syntax );
    return syntax;
  }

protected:

  static void addSyntax( MSyntax &syntax );

  struct Args : Parent::Args
  {
    FTL::StrRef argName;
    FTL::StrRef typeName;
  };

  void getArgs( MArgParser &argParser, Args &args );

  virtual FabricUI::DFG::DFGUICmd *executeDFGUICmd(
    MArgParser &argParser
    );
};

// class FabricDFGDisconnectCommand : public FabricDFGExecCommand
// {
//   typedef FabricDFGExecCommand Parent;
  
// public:

//   virtual MString getName()
//     { return MString(
//       FabricUI::DFG::DFGUICmd_Disconnect::CmdName().c_str()
//       ); }

//   static void* creator()
//     { return new FabricDFGDisconnectCommand; }

//   static MSyntax newSyntax();

//   virtual MStatus invoke(
//     MArgParser &argParser,
//     FabricCore::DFGExec &exec
//     );
// };

// class FabricDFGRemoveNodesCommand : public FabricDFGExecCommand
// {
//   typedef FabricDFGExecCommand Parent;
  
// public:

//   virtual MString getName()
//     { return MString(
//       FabricUI::DFG::DFGUICmd_RemoveNodes::CmdName().c_str()
//       ); }

//   static void* creator()
//     { return new FabricDFGRemoveNodesCommand; }

//   static MSyntax newSyntax();

//   virtual MStatus invoke(
//     MArgParser &argParser,
//     FabricCore::DFGExec &exec
//     );
// };

class FabricDFGAddNodeCommand : public FabricDFGExecCommand
{
  typedef FabricDFGExecCommand Parent;
  
protected:

  static void addSyntax( MSyntax &syntax );

  struct Args : Parent::Args
  {
    QPointF pos;
  };

  void getArgs( MArgParser &argParser, Args &args );
};

class FabricDFGInstPresetCommand : public FabricDFGAddNodeCommand
{
  typedef FabricDFGAddNodeCommand Parent;
  
public:

  virtual MString getName()
    { return MString(
      FabricUI::DFG::DFGUICmd_InstPreset::CmdName().c_str()
      ); }

  static void* creator()
    { return new FabricDFGInstPresetCommand; }

  static MSyntax newSyntax()
  {
    MSyntax syntax;
    addSyntax( syntax );
    return syntax;
  }

protected:

  static void addSyntax( MSyntax &syntax );

  struct Args : Parent::Args
  {
    std::string presetPath;
  };

  void getArgs( MArgParser &argParser, Args &args );

  virtual FabricUI::DFG::DFGUICmd *executeDFGUICmd(
    MArgParser &argParser
    );
};

// class FabricDFGAddGraphCommand : public FabricDFGAddNodeCommand
// {
//   typedef FabricDFGAddNodeCommand Parent;
  
// public:

//   virtual MString getName()
//     { return MString(
//       FabricUI::DFG::DFGUICmd_AddGraph::CmdName().c_str()
//       ); }

//   static void* creator()
//     { return new FabricDFGAddGraphCommand; }

//   static MSyntax newSyntax();

//   virtual MStatus invoke(
//     MArgParser &argParser,
//     FabricCore::DFGExec &exec
//     );
// };

// class FabricDFGAddFuncCommand : public FabricDFGAddNodeCommand
// {
//   typedef FabricDFGAddNodeCommand Parent;
  
// public:

//   virtual MString getName()
//     { return MString(
//       FabricUI::DFG::DFGUICmd_AddFunc::CmdName().c_str()
//       ); }

//   static void* creator()
//     { return new FabricDFGAddFuncCommand; }

//   static MSyntax newSyntax();

//   virtual MStatus invoke(
//     MArgParser &argParser,
//     FabricCore::DFGExec &exec
//     );
// };

// class FabricDFGAddVarCommand : public FabricDFGAddNodeCommand
// {
//   typedef FabricDFGAddNodeCommand Parent;
  
// public:

//   virtual MString getName()
//     { return MString(
//       FabricUI::DFG::DFGUICmd_AddVar::CmdName().c_str()
//       ); }

//   static void* creator()
//     { return new FabricDFGAddVarCommand; }

//   static MSyntax newSyntax();

//   virtual MStatus invoke(
//     MArgParser &argParser,
//     FabricCore::DFGExec &exec
//     );
// };

// class FabricDFGAddGetCommand : public FabricDFGAddNodeCommand
// {
//   typedef FabricDFGAddNodeCommand Parent;
  
// public:

//   virtual MString getName()
//     { return MString(
//       FabricUI::DFG::DFGUICmd_AddGet::CmdName().c_str()
//       ); }

//   static void* creator()
//     { return new FabricDFGAddGetCommand; }

//   static MSyntax newSyntax();

//   virtual MStatus invoke(
//     MArgParser &argParser,
//     FabricCore::DFGExec &exec
//     );
// };

// class FabricDFGAddSetCommand : public FabricDFGAddNodeCommand
// {
//   typedef FabricDFGAddNodeCommand Parent;
  
// public:

//   virtual MString getName()
//     { return MString(
//       FabricUI::DFG::DFGUICmd_AddSet::CmdName().c_str()
//       ); }

//   static void* creator()
//     { return new FabricDFGAddSetCommand; }

//   static MSyntax newSyntax();

//   virtual MStatus invoke(
//     MArgParser &argParser,
//     FabricCore::DFGExec &exec
//     );
// };

#endif 
