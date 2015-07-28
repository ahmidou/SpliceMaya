
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

class FabricDFGBaseCommand: public MPxCommand
{
protected:

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

  virtual MString getName() = 0;

  void logError( MString const &desc )
    { displayError( getName() + ": " + desc, true ); }
};

class FabricDFGCoreCommand: public FabricDFGBaseCommand
{
public:

  virtual MStatus doIt( const MArgList &args );
  virtual MStatus undoIt();
  virtual MStatus redoIt();

  virtual bool isUndoable() const
    { return true; }

protected:

  FabricDFGCoreCommand() {}

  virtual FabricUI::DFG::DFGUICmd *executeDFGUICmd(
    MArgParser &argParser
    ) = 0;

  static void AddSyntax( MSyntax &syntax );

private:

  FTL::OwnedPtr<FabricUI::DFG::DFGUICmd> m_dfgUICmd;
};

class FabricDFGBindingCommand : public FabricDFGCoreCommand
{
  typedef FabricDFGCoreCommand Parent;

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

class FabricDFGCnxnCommand
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
};

class FabricDFGConnectCommand
  : public FabricDFGCnxnCommand
{
  typedef FabricDFGCnxnCommand Parent;
  
protected:

  virtual FabricUI::DFG::DFGUICmd *executeDFGUICmd(
    MArgParser &argParser
    );
};

typedef MayaDFGUICmdWrapper<
  FabricDFGConnectCommand,
  FabricUI::DFG::DFGUICmd_Connect
  > MayaDFGUICmd_Connect;

class FabricDFGDisconnectCommand
  : public FabricDFGCnxnCommand
{
  typedef FabricDFGCnxnCommand Parent;
  
protected:

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

class FabricDFGImplodeNodesCommand
  : public FabricDFGExecCommand
{
  typedef FabricDFGExecCommand Parent;
  
protected:

  static void AddSyntax( MSyntax &syntax );

  struct Args : Parent::Args
  {
    std::vector<FTL::StrRef> nodeNames;
    FTL::StrRef desiredImplodedNodeName;
  };

  static void GetArgs( MArgParser &argParser, Args &args );

  virtual FabricUI::DFG::DFGUICmd *executeDFGUICmd(
    MArgParser &argParser
    );
};

typedef MayaDFGUICmdWrapper<
  FabricDFGImplodeNodesCommand,
  FabricUI::DFG::DFGUICmd_ImplodeNodes
  > MayaDFGUICmd_ImplodeNodes;

class FabricDFGExplodeNodeCommand
  : public FabricDFGExecCommand
{
  typedef FabricDFGExecCommand Parent;
  
protected:

  static void AddSyntax( MSyntax &syntax );

  struct Args : Parent::Args
  {
    FTL::StrRef node;
  };

  static void GetArgs( MArgParser &argParser, Args &args );

  virtual FabricUI::DFG::DFGUICmd *executeDFGUICmd(
    MArgParser &argParser
    );
};

typedef MayaDFGUICmdWrapper<
  FabricDFGExplodeNodeCommand,
  FabricUI::DFG::DFGUICmd_ExplodeNode
  > MayaDFGUICmd_ExplodeNode;

class FabricDFGPasteCommand
  : public FabricDFGExecCommand
{
  typedef FabricDFGExecCommand Parent;
  
protected:

  static void AddSyntax( MSyntax &syntax );

  struct Args : Parent::Args
  {
    FTL::StrRef text;
    QPointF xy;
  };

  static void GetArgs( MArgParser &argParser, Args &args );

  virtual FabricUI::DFG::DFGUICmd *executeDFGUICmd(
    MArgParser &argParser
    );
};

typedef MayaDFGUICmdWrapper<
  FabricDFGPasteCommand,
  FabricUI::DFG::DFGUICmd_Paste
  > MayaDFGUICmd_Paste;

class FabricDFGResizeBackDropCommand
  : public FabricDFGExecCommand
{
  typedef FabricDFGExecCommand Parent;
  
protected:

  static void AddSyntax( MSyntax &syntax );

  struct Args : Parent::Args
  {
    FTL::StrRef nodeName;
    QPointF xy;
    QSizeF wh;
  };

  static void GetArgs( MArgParser &argParser, Args &args );

  virtual FabricUI::DFG::DFGUICmd *executeDFGUICmd(
    MArgParser &argParser
    );
};

typedef MayaDFGUICmdWrapper<
  FabricDFGResizeBackDropCommand,
  FabricUI::DFG::DFGUICmd_ResizeBackDrop
  > MayaDFGUICmd_ResizeBackDrop;

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

class FabricDFGSetArgValueCommand
  : public FabricDFGBindingCommand
{
  typedef FabricDFGBindingCommand Parent;
  
protected:

  static void AddSyntax( MSyntax &syntax );

  struct Args : Parent::Args
  {
    FTL::StrRef argName;
    FabricCore::RTVal value;
  };

  static void GetArgs( MArgParser &argParser, Args &args );

  virtual FabricUI::DFG::DFGUICmd *executeDFGUICmd(
    MArgParser &argParser
    );
};

typedef MayaDFGUICmdWrapper<
  FabricDFGSetArgValueCommand,
  FabricUI::DFG::DFGUICmd_SetArgValue
  > MayaDFGUICmd_SetArgValue;

class FabricDFGSetPortDefaultValueCommand
  : public FabricDFGExecCommand
{
  typedef FabricDFGExecCommand Parent;
  
protected:

  static void AddSyntax( MSyntax &syntax );

  struct Args : Parent::Args
  {
    FTL::StrRef portPath;
    FabricCore::RTVal value;
  };

  static void GetArgs( MArgParser &argParser, Args &args );

  virtual FabricUI::DFG::DFGUICmd *executeDFGUICmd(
    MArgParser &argParser
    );
};

typedef MayaDFGUICmdWrapper<
  FabricDFGSetPortDefaultValueCommand,
  FabricUI::DFG::DFGUICmd_SetPortDefaultValue
  > MayaDFGUICmd_SetPortDefaultValue;

class FabricDFGSetNodeTitleCommand
  : public FabricDFGExecCommand
{
  typedef FabricDFGExecCommand Parent;
  
protected:

  static void AddSyntax( MSyntax &syntax );

  struct Args : Parent::Args
  {
    FTL::StrRef nodeName;
    FTL::StrRef title;
  };

  static void GetArgs( MArgParser &argParser, Args &args );

  virtual FabricUI::DFG::DFGUICmd *executeDFGUICmd(
    MArgParser &argParser
    );
};

typedef MayaDFGUICmdWrapper<
  FabricDFGSetNodeTitleCommand,
  FabricUI::DFG::DFGUICmd_SetNodeTitle
  > MayaDFGUICmd_SetNodeTitle;

class FabricDFGRemovePortCommand
  : public FabricDFGExecCommand
{
  typedef FabricDFGExecCommand Parent;
  
protected:

  static void AddSyntax( MSyntax &syntax );

  struct Args : Parent::Args
  {
    FTL::StrRef portName;
  };

  static void GetArgs( MArgParser &argParser, Args &args );

  virtual FabricUI::DFG::DFGUICmd *executeDFGUICmd(
    MArgParser &argParser
    );
};

typedef MayaDFGUICmdWrapper<
  FabricDFGRemovePortCommand,
  FabricUI::DFG::DFGUICmd_RemovePort
  > MayaDFGUICmd_RemovePort;

class FabricDFGSetCodeCommand
  : public FabricDFGExecCommand
{
  typedef FabricDFGExecCommand Parent;
  
protected:

  static void AddSyntax( MSyntax &syntax );

  struct Args : Parent::Args
  {
    FTL::StrRef code;
  };

  static void GetArgs( MArgParser &argParser, Args &args );

  virtual FabricUI::DFG::DFGUICmd *executeDFGUICmd(
    MArgParser &argParser
    );
};

typedef MayaDFGUICmdWrapper<
  FabricDFGSetCodeCommand,
  FabricUI::DFG::DFGUICmd_SetCode
  > MayaDFGUICmd_SetCode;

class FabricDFGSetRefVarPathCommand
  : public FabricDFGExecCommand
{
  typedef FabricDFGExecCommand Parent;
  
protected:

  static void AddSyntax( MSyntax &syntax );

  struct Args : Parent::Args
  {
    FTL::StrRef refName;
    FTL::StrRef varPath;
  };

  static void GetArgs( MArgParser &argParser, Args &args );

  virtual FabricUI::DFG::DFGUICmd *executeDFGUICmd(
    MArgParser &argParser
    );
};

typedef MayaDFGUICmdWrapper<
  FabricDFGSetRefVarPathCommand,
  FabricUI::DFG::DFGUICmd_SetRefVarPath
  > MayaDFGUICmd_SetRefVarPath;

class FabricDFGRenamePortCommand
  : public FabricDFGExecCommand
{
  typedef FabricDFGExecCommand Parent;
  
protected:

  static void AddSyntax( MSyntax &syntax );

  struct Args : Parent::Args
  {
    FTL::StrRef oldPortName;
    FTL::StrRef desiredNewPortName;
  };

  static void GetArgs( MArgParser &argParser, Args &args );

  virtual FabricUI::DFG::DFGUICmd *executeDFGUICmd(
    MArgParser &argParser
    );
};

typedef MayaDFGUICmdWrapper<
  FabricDFGRenamePortCommand,
  FabricUI::DFG::DFGUICmd_RenamePort
  > MayaDFGUICmd_RenamePort;

class FabricDFGSetNodeCommentCommand
  : public FabricDFGExecCommand
{
  typedef FabricDFGExecCommand Parent;
  
protected:

  static void AddSyntax( MSyntax &syntax );

  struct Args : Parent::Args
  {
    FTL::StrRef nodeName;
    FTL::StrRef comment;
    bool expanded;
  };

  static void GetArgs( MArgParser &argParser, Args &args );

  virtual FabricUI::DFG::DFGUICmd *executeDFGUICmd(
    MArgParser &argParser
    );
};

typedef MayaDFGUICmdWrapper<
  FabricDFGSetNodeCommentCommand,
  FabricUI::DFG::DFGUICmd_SetNodeComment
  > MayaDFGUICmd_SetNodeComment;

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

class FabricDFGAddBackDropCommand
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
  FabricDFGAddBackDropCommand,
  FabricUI::DFG::DFGUICmd_AddBackDrop
  > MayaDFGUICmd_AddBackDrop;

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

class FabricDFGAddVarCommand
  : public FabricDFGAddNodeCommand
{
  typedef FabricDFGAddNodeCommand Parent;
  
protected:

  static void AddSyntax( MSyntax &syntax );

  struct Args : Parent::Args
  {
    std::string desiredName;
    std::string type;
    std::string extDep;
  };

  static void GetArgs( MArgParser &argParser, Args &args );

  virtual FabricUI::DFG::DFGUICmd *executeDFGUICmd(
    MArgParser &argParser
    );
};

typedef MayaDFGUICmdWrapper<
  FabricDFGAddVarCommand,
  FabricUI::DFG::DFGUICmd_AddVar
  > MayaDFGUICmd_AddVar;

class FabricDFGAddRefCommand
  : public FabricDFGAddNodeCommand
{
  typedef FabricDFGAddNodeCommand Parent;
  
protected:

  static void AddSyntax( MSyntax &syntax );

  struct Args : Parent::Args
  {
    std::string desiredName;
    std::string varPath;
  };

  static void GetArgs( MArgParser &argParser, Args &args );
};

class FabricDFGAddGetCommand
  : public FabricDFGAddRefCommand
{
  typedef FabricDFGAddRefCommand Parent;
  
protected:

  virtual FabricUI::DFG::DFGUICmd *executeDFGUICmd(
    MArgParser &argParser
    );
};

typedef MayaDFGUICmdWrapper<
  FabricDFGAddGetCommand,
  FabricUI::DFG::DFGUICmd_AddGet
  > MayaDFGUICmd_AddGet;

class FabricDFGAddSetCommand
  : public FabricDFGAddRefCommand
{
  typedef FabricDFGAddRefCommand Parent;
  
protected:

  virtual FabricUI::DFG::DFGUICmd *executeDFGUICmd(
    MArgParser &argParser
    );
};

typedef MayaDFGUICmdWrapper<
  FabricDFGAddSetCommand,
  FabricUI::DFG::DFGUICmd_AddSet
  > MayaDFGUICmd_AddSet;

class FabricDFGImportJSONCommand
  : public FabricDFGBaseCommand
{
public:

  static void* creator()
    { return new FabricDFGImportJSONCommand; }

  virtual MString getName()
    { return "dfgImportJSON"; }

  static MSyntax newSyntax();
  virtual MStatus doIt( const MArgList &args );
  virtual bool isUndoable() const { return false; }
};

class FabricDFGReloadJSONCommand
  : public FabricDFGBaseCommand
{
public:

  static void* creator()
    { return new FabricDFGReloadJSONCommand; }

  virtual MString getName()
    { return "dfgReloadJSON"; }

  static MSyntax newSyntax();
  virtual MStatus doIt( const MArgList &args );
  virtual bool isUndoable() const { return false; }
};

class FabricDFGExportJSONCommand
  : public FabricDFGBaseCommand
{
public:

  static void* creator()
    { return new FabricDFGExportJSONCommand; }

  virtual MString getName()
    { return "dfgExportJSON"; }

  static MSyntax newSyntax();
  virtual MStatus doIt( const MArgList &args );
  virtual bool isUndoable() const { return false; }
};

#endif 
