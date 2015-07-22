// Copyright 2010-2015 Fabric Software Inc. All rights reserved.

#ifndef __UI_DFG_DFGUICmdHandler_Maya__
#define __UI_DFG_DFGUICmdHandler_Maya__

#include <FabricUI/DFG/DFGUICmdHandler.h>

#include <maya/MString.h>

class DFGUICmdHandler_Maya : public FabricUI::DFG::DFGUICmdHandler
{
public:

  DFGUICmdHandler_Maya() {}

protected:

  virtual void dfgDoRemoveNodes(
    FTL::CStrRef desc,
    FabricCore::DFGBinding const &binding,
    FTL::CStrRef execPath,
    FabricCore::DFGExec const &exec,
    FTL::ArrayRef<FTL::CStrRef> nodeNames
    );

  virtual void dfgDoConnect(
    FTL::CStrRef desc,
    FabricCore::DFGBinding const &binding,
    FTL::CStrRef execPath,
    FabricCore::DFGExec const &exec,
    FTL::CStrRef srcPath, 
    FTL::CStrRef dstPath
    );

  virtual void dfgDoDisconnect(
    FTL::CStrRef desc,
    FabricCore::DFGBinding const &binding,
    FTL::CStrRef execPath,
    FabricCore::DFGExec const &exec,
    FTL::CStrRef srcPath, 
    FTL::CStrRef dstPath
    );

  virtual std::string dfgDoAddInstWithEmptyGraph(
    FTL::CStrRef desc,
    FabricCore::DFGBinding const &binding,
    FTL::CStrRef execPath,
    FabricCore::DFGExec const &exec,
    FTL::CStrRef title,
    QPointF pos
    );

  virtual std::string dfgDoAddInstWithEmptyFunc(
    FTL::CStrRef desc,
    FabricCore::DFGBinding const &binding,
    FTL::CStrRef execPath,
    FabricCore::DFGExec const &exec,
    FTL::CStrRef title,
    FTL::CStrRef initialCode,
    QPointF pos
    );

  virtual std::string dfgDoAddInstFromPreset(
    FTL::CStrRef desc,
    FabricCore::DFGBinding const &binding,
    FTL::CStrRef execPath,
    FabricCore::DFGExec const &exec,
    FTL::CStrRef presetPath,
    QPointF pos
    );

  virtual std::string dfgDoAddVar(
    FTL::CStrRef desc,
    FabricCore::DFGBinding const &binding,
    FTL::CStrRef execPath,
    FabricCore::DFGExec const &exec,
    FTL::CStrRef desiredNodeName,
    FTL::CStrRef dataType,
    FTL::CStrRef extDep,
    QPointF pos
    );

  virtual std::string dfgDoAddGet(
    FTL::CStrRef desc,
    FabricCore::DFGBinding const &binding,
    FTL::CStrRef execPath,
    FabricCore::DFGExec const &exec,
    FTL::CStrRef desiredNodeName,
    FTL::CStrRef varPath,
    QPointF pos
    );

  virtual std::string dfgDoAddSet(
    FTL::CStrRef desc,
    FabricCore::DFGBinding const &binding,
    FTL::CStrRef execPath,
    FabricCore::DFGExec const &exec,
    FTL::CStrRef desiredNodeName,
    FTL::CStrRef varPath,
    QPointF pos
    );

  virtual std::string dfgDoAddPort(
    FTL::CStrRef desc,
    FabricCore::DFGBinding const &binding,
    FTL::CStrRef execPath,
    FabricCore::DFGExec const &exec,
    FTL::CStrRef desiredPortName,
    FabricCore::DFGPortType portType,
    FTL::CStrRef typeSpec,
    FTL::CStrRef portToConnect
    );

  virtual void dfgDoRemovePort(
    FTL::CStrRef desc,
    FabricCore::DFGBinding const &binding,
    FTL::CStrRef execPath,
    FabricCore::DFGExec const &exec,
    FTL::CStrRef portName
    );

  virtual void dfgDoMoveNodes(
    FTL::CStrRef desc,
    FabricCore::DFGBinding const &binding,
    FTL::CStrRef execPath,
    FabricCore::DFGExec const &exec,
    FTL::ArrayRef<FTL::CStrRef> nodeNames,
    FTL::ArrayRef<QPointF> newTopLeftPoss
    );

  virtual void dfgDoResizeBackDropNode(
    FTL::CStrRef desc,
    FabricCore::DFGBinding const &binding,
    FTL::CStrRef execPath,
    FabricCore::DFGExec const &exec,
    FTL::CStrRef backDropNodeName,
    QPointF newTopLeftPos,
    QSizeF newSize
    );

  virtual std::string dfgDoImplodeNodes(
    FTL::CStrRef desc,
    FabricCore::DFGBinding const &binding,
    FTL::CStrRef execPath,
    FabricCore::DFGExec const &exec,
    FTL::CStrRef desiredNodeName,
    FTL::ArrayRef<FTL::CStrRef> nodeNames
    );

  virtual std::vector<std::string> dfgDoExplodeNode(
    FTL::CStrRef desc,
    FabricCore::DFGBinding const &binding,
    FTL::CStrRef execPath,
    FabricCore::DFGExec const &exec,
    FTL::CStrRef nodeName
    );

  virtual void dfgDoAddBackDrop(
    FTL::CStrRef desc,
    FabricCore::DFGBinding const &binding,
    FTL::CStrRef execPath,
    FabricCore::DFGExec const &exec,
    FTL::CStrRef title,
    QPointF pos
    );

  virtual void dfgDoSetNodeTitle(
    FTL::CStrRef desc,
    FabricCore::DFGBinding const &binding,
    FTL::CStrRef execPath,
    FabricCore::DFGExec const &exec,
    FTL::CStrRef node,
    FTL::CStrRef title
    );

  virtual void dfgDoSetNodeComment(
    FTL::CStrRef desc,
    FabricCore::DFGBinding const &binding,
    FTL::CStrRef execPath,
    FabricCore::DFGExec const &exec,
    FTL::CStrRef node,
    FTL::CStrRef comment,
    bool expanded
    );

  virtual void dfgDoSetCode(
    FTL::CStrRef desc,
    FabricCore::DFGBinding const &binding,
    FTL::CStrRef execPath,
    FabricCore::DFGExec const &exec,
    FTL::CStrRef code
    );

  virtual std::string dfgDoRenameExecPort(
    FTL::CStrRef desc,
    FabricCore::DFGBinding const &binding,
    FTL::CStrRef execPath,
    FabricCore::DFGExec const &exec,
    FTL::CStrRef oldName,
    FTL::CStrRef desiredNewName
    );

  virtual std::vector<std::string> dfgDoPaste(
    FTL::CStrRef desc,
    FabricCore::DFGBinding const &binding,
    FTL::CStrRef execPath,
    FabricCore::DFGExec const &exec,
    FTL::CStrRef json,
    QPointF cursorPos
    );

  virtual void dfgDoSetArgType(
    FTL::CStrRef desc,
    FabricCore::DFGBinding const &binding,
    FTL::CStrRef argName,
    FTL::CStrRef typeName
    );

  virtual void dfgDoSetArgValue(
    FTL::CStrRef desc,
    FabricCore::DFGBinding const &binding,
    FTL::CStrRef name,
    FabricCore::RTVal const &value
    );

  virtual void dfgDoSetPortDefaultValue(
    FTL::CStrRef desc,
    FabricCore::DFGBinding const &binding,
    FTL::CStrRef execPath,
    FabricCore::DFGExec const &exec,
    FTL::CStrRef port,
    FabricCore::RTVal const &value
    );

  virtual void dfgDoSetRefVarPath(
    FTL::CStrRef desc,
    FabricCore::DFGBinding const &binding,
    FTL::CStrRef execPath,
    FabricCore::DFGExec const &exec,
    FTL::CStrRef refName,
    FTL::CStrRef varPath
    );

protected:
    
  void encodeMELString(
    FTL::CStrRef str,
    std::stringstream &ss
    );

  void encodeBooleanArg(
    FTL::CStrRef name,
    bool value,
    std::stringstream &cmd
    );

  void encodeStringArg(
    FTL::CStrRef name,
    FTL::CStrRef value,
    std::stringstream &cmd
    );

  void encodeStringsArg(
    FTL::CStrRef name,
    FTL::ArrayRef<FTL::CStrRef> values,
    std::stringstream &cmd
    );

  void encodePositionArg(
    FTL::CStrRef name,
    QPointF value,
    std::stringstream &cmd
    );

  void encodePositionsArg(
    FTL::CStrRef name,
    FTL::ArrayRef<QPointF> values,
    std::stringstream &cmd
    );

  void encodeSizeArg(
    FTL::CStrRef name,
    QSizeF value,
    std::stringstream &cmd
    );

  void encodeSizesArg(
    FTL::CStrRef name,
    FTL::ArrayRef<QSizeF> values,
    std::stringstream &cmd
    );

  void encodeBinding(
    FabricCore::DFGBinding const &binding,
    std::stringstream &cmd
    );

  void encodeExec(
    FabricCore::DFGBinding const &binding,
    FTL::CStrRef execPath,
    FabricCore::DFGExec const &exec,
    std::stringstream &cmd
    );

  void encodePositions(
    FTL::ArrayRef<QPointF> poss,
    std::stringstream &cmd
    );

  MString getNodeNameFromBinding( FabricCore::DFGBinding const &binding );
};

#endif // __UI_DFG_DFGUICmdHandler_Maya__
