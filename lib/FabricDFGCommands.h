
#ifndef _FABRICDFGCOMMANDS_H_
#define _FABRICDFGCOMMANDS_H_

// #include "FabricSpliceEditorWidget.h"

#include <iostream>

#include <maya/MArgList.h>
#include <maya/MArgParser.h>
#include <maya/MPxCommand.h>

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

template<class MayaDFGUICmdClass, class FabricDFGUICmdClass>
class MayaDFGUICmdWrapper : public MayaDFGUICmdClass
{
  typedef MayaDFGUICmdClass Parent;

public:

  static MString GetName()
    { return FabricDFGUICmdClass::CmdName().c_str(); }

  static MCreatorFunction GetCreator()
    { return &Creator; }

  static MCreateSyntaxFunction GetCreateSyntax()
    { return &CreateSyntax; }

  virtual MString getName()
    { return GetName(); }

protected:

  static void *Creator()
  {
    return new MayaDFGUICmdWrapper;
  }

  static MSyntax CreateSyntax()
  {
    MSyntax syntax;
    Parent::AddSyntax( syntax );
    return syntax;
  }
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

  static void AddSyntax( MSyntax &syntax );

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

  static void AddSyntax( MSyntax &syntax );

  struct Args
  {
    FabricCore::DFGBinding binding;
  };

  static void GetArgs(
    MArgParser &argParser,
    Args &args
    );
};

class FabricDFGExecCommand
  : public FabricDFGBindingCommand
{
  typedef FabricDFGBindingCommand Parent;

protected:

  static void AddSyntax( MSyntax &syntax );

  struct Args : Parent::Args
  {
    FTL::StrRef execPath;
    FabricCore::DFGExec exec;
  };

  static void GetArgs(
    MArgParser &argParser,
    Args &args
    );
};

class FabricDFGConnectCommand
  : public FabricDFGExecCommand
{
  typedef FabricDFGExecCommand Parent;
  
protected:

  static void AddSyntax( MSyntax &syntax );

  struct Args : Parent::Args
  {
    FTL::StrRef srcPort;
    FTL::StrRef dstPort;
  };

  static void GetArgs( MArgParser &argParser, Args &args );

  virtual FabricUI::DFG::DFGUICmd *executeDFGUICmd(
    MArgParser &argParser
    );
};

typedef MayaDFGUICmdWrapper<
  FabricDFGConnectCommand,
  FabricUI::DFG::DFGUICmd_Connect
  > MayaDFGUICmd_Connect;

class FabricDFGDisconnectCommand
  : public FabricDFGExecCommand
{
  typedef FabricDFGExecCommand Parent;
  
protected:

  static void AddSyntax( MSyntax &syntax );

  struct Args : Parent::Args
  {
    FTL::StrRef srcPort;
    FTL::StrRef dstPort;
  };

  static void GetArgs( MArgParser &argParser, Args &args );

  virtual FabricUI::DFG::DFGUICmd *executeDFGUICmd(
    MArgParser &argParser
    );
};

typedef MayaDFGUICmdWrapper<
  FabricDFGDisconnectCommand,
  FabricUI::DFG::DFGUICmd_Disconnect
  > MayaDFGUICmd_Disconnect;

class FabricDFGMoveNodesCommand
  : public FabricDFGExecCommand
{
  typedef FabricDFGExecCommand Parent;
  
protected:

  static void AddSyntax( MSyntax &syntax );

  struct Args : Parent::Args
  {
    std::vector<FTL::StrRef> nodeNames;
    std::vector<QPointF> poss;
  };

  static void GetArgs( MArgParser &argParser, Args &args );

  virtual FabricUI::DFG::DFGUICmd *executeDFGUICmd(
    MArgParser &argParser
    );
};

typedef MayaDFGUICmdWrapper<
  FabricDFGMoveNodesCommand,
  FabricUI::DFG::DFGUICmd_MoveNodes
  > MayaDFGUICmd_MoveNodes;

class FabricDFGRemoveNodesCommand
  : public FabricDFGExecCommand
{
  typedef FabricDFGExecCommand Parent;
  
protected:

  static void AddSyntax( MSyntax &syntax );

  struct Args : Parent::Args
  {
    std::vector<FTL::StrRef> nodeNames;
  };

  static void GetArgs( MArgParser &argParser, Args &args );

  virtual FabricUI::DFG::DFGUICmd *executeDFGUICmd(
    MArgParser &argParser
    );
};

typedef MayaDFGUICmdWrapper<
  FabricDFGRemoveNodesCommand,
  FabricUI::DFG::DFGUICmd_RemoveNodes
  > MayaDFGUICmd_RemoveNodes;

class FabricDFGAddPortCommand
  : public FabricDFGExecCommand
{
  typedef FabricDFGExecCommand Parent;
  
protected:

  static void AddSyntax( MSyntax &syntax );

  struct Args : Parent::Args
  {
    FTL::StrRef desiredPortName;
    FabricCore::DFGPortType portType;
    FTL::StrRef typeSpec;
    FTL::StrRef portToConnectWith;
  };

  static void GetArgs( MArgParser &argParser, Args &args );

  virtual FabricUI::DFG::DFGUICmd *executeDFGUICmd(
    MArgParser &argParser
    );
};

typedef MayaDFGUICmdWrapper<
  FabricDFGAddPortCommand,
  FabricUI::DFG::DFGUICmd_AddPort
  > MayaDFGUICmd_AddPort;

class FabricDFGSetArgTypeCommand
  : public FabricDFGBindingCommand
{
  typedef FabricDFGBindingCommand Parent;
  
protected:

  static void AddSyntax( MSyntax &syntax );

  struct Args : Parent::Args
  {
    FTL::StrRef argName;
    FTL::StrRef typeName;
  };

  static void GetArgs( MArgParser &argParser, Args &args );

  virtual FabricUI::DFG::DFGUICmd *executeDFGUICmd(
    MArgParser &argParser
    );
};

typedef MayaDFGUICmdWrapper<
  FabricDFGSetArgTypeCommand,
  FabricUI::DFG::DFGUICmd_SetArgType
  > MayaDFGUICmd_SetArgType;

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

class FabricDFGAddNodeCommand
  : public FabricDFGExecCommand
{
  typedef FabricDFGExecCommand Parent;
  
protected:

  static void AddSyntax( MSyntax &syntax );

  struct Args : Parent::Args
  {
    QPointF pos;
  };

  static void GetArgs( MArgParser &argParser, Args &args );
};

class FabricDFGInstPresetCommand
  : public FabricDFGAddNodeCommand
{
  typedef FabricDFGAddNodeCommand Parent;
  
protected:

  static void AddSyntax( MSyntax &syntax );

  struct Args : Parent::Args
  {
    std::string presetPath;
  };

  static void GetArgs( MArgParser &argParser, Args &args );

  virtual FabricUI::DFG::DFGUICmd *executeDFGUICmd(
    MArgParser &argParser
    );
};

typedef MayaDFGUICmdWrapper<
  FabricDFGInstPresetCommand,
  FabricUI::DFG::DFGUICmd_InstPreset
  > MayaDFGUICmd_InstPreset;

class FabricDFGAddGraphCommand
  : public FabricDFGAddNodeCommand
{
  typedef FabricDFGAddNodeCommand Parent;
  
protected:

  static void AddSyntax( MSyntax &syntax );

  struct Args : Parent::Args
  {
    std::string title;
  };

  static void GetArgs( MArgParser &argParser, Args &args );

  virtual FabricUI::DFG::DFGUICmd *executeDFGUICmd(
    MArgParser &argParser
    );
};

typedef MayaDFGUICmdWrapper<
  FabricDFGAddGraphCommand,
  FabricUI::DFG::DFGUICmd_AddGraph
  > MayaDFGUICmd_AddGraph;

class FabricDFGAddFuncCommand
  : public FabricDFGAddNodeCommand
{
  typedef FabricDFGAddNodeCommand Parent;
  
protected:

  static void AddSyntax( MSyntax &syntax );

  struct Args : Parent::Args
  {
    std::string title;
    std::string code;
  };

  static void GetArgs( MArgParser &argParser, Args &args );

  virtual FabricUI::DFG::DFGUICmd *executeDFGUICmd(
    MArgParser &argParser
    );
};

typedef MayaDFGUICmdWrapper<
  FabricDFGAddFuncCommand,
  FabricUI::DFG::DFGUICmd_AddFunc
  > MayaDFGUICmd_AddFunc;

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
