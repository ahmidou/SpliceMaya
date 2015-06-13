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

  virtual void dfgDoRemoveNodesImpl(
    FTL::CStrRef desc,
    FabricCore::DFGBinding &binding,
    FTL::CStrRef execPath,
    FabricCore::DFGExec &exec,
    FTL::ArrayRef<FTL::StrRef> nodeNames
    );
  virtual void dfgDoConnectImpl(
    FTL::CStrRef desc,
    FabricCore::DFGBinding &binding,
    FTL::CStrRef execPath,
    FabricCore::DFGExec &exec,
    FTL::StrRef srcPath, 
    FTL::StrRef dstPath
    );
  virtual void dfgDoDisconnectImpl(
    FTL::CStrRef desc,
    FabricCore::DFGBinding &binding,
    FTL::CStrRef execPath,
    FabricCore::DFGExec &exec,
    FTL::StrRef srcPath, 
    FTL::StrRef dstPath
    );
  virtual std::string dfgDoAddInstWithEmptyGraphImpl(
    FTL::CStrRef desc,
    FabricCore::DFGBinding &binding,
    FTL::CStrRef execPath,
    FabricCore::DFGExec &exec,
    FTL::CStrRef title,
    QPointF pos
    );
  virtual std::string dfgDoAddInstWithEmptyFuncImpl(
    FTL::CStrRef desc,
    FabricCore::DFGBinding &binding,
    FTL::CStrRef execPath,
    FabricCore::DFGExec &exec,
    FTL::CStrRef title,
    FTL::CStrRef initialCode,
    QPointF pos
    );
  virtual std::string dfgDoAddInstFromPresetImpl(
    FTL::CStrRef desc,
    FabricCore::DFGBinding &binding,
    FTL::CStrRef execPath,
    FabricCore::DFGExec &exec,
    FTL::CStrRef presetPath,
    QPointF pos
    );
  virtual std::string dfgDoAddVarImpl(
    FTL::CStrRef desc,
    FabricCore::DFGBinding &binding,
    FTL::CStrRef execPath,
    FabricCore::DFGExec &exec,
    FTL::CStrRef desiredNodeName,
    FTL::StrRef dataType,
    FTL::StrRef extDep,
    QPointF pos
    );
  virtual std::string dfgDoAddGetImpl(
    FTL::CStrRef desc,
    FabricCore::DFGBinding &binding,
    FTL::CStrRef execPath,
    FabricCore::DFGExec &exec,
    FTL::CStrRef desiredNodeName,
    FTL::StrRef varPath,
    QPointF pos
    );
  virtual std::string dfgDoAddSetImpl(
    FTL::CStrRef desc,
    FabricCore::DFGBinding &binding,
    FTL::CStrRef execPath,
    FabricCore::DFGExec &exec,
    FTL::CStrRef desiredNodeName,
    FTL::StrRef varPath,
    QPointF pos
    );

protected:
    
  MString getNodeNameFromBinding( FabricCore::DFGBinding &binding );
};

#endif // __UI_DFG_DFGUICmdHandler_Maya__
