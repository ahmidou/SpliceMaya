//
// Copyright (c) 2010-2017, Fabric Software Inc. All rights reserved.
//
 
#include "Foundation.h"
#include "FabricCommand.h"
#include "FabricSpliceHelpers.h"
#include "FabricMayaException.h"
#include "FabricCommandManagerCallback.h"
#include <FabricUI/Commands/CommandManager.h>

using namespace FabricUI::Commands;

FabricCommand::FabricCommand()
  : m_isUndoable(false)
{
}

FabricCommand::~FabricCommand()
{
}

void* FabricCommand::creator()
{
  return new FabricCommand;
}

bool FabricCommand::isUndoable() const
{
  return m_isUndoable;
}

MStatus FabricCommand::doIt(
  const MArgList &args)
{
  MStatus status;

  FABRIC_MAYA_CATCH_BEGIN();

  // Create the maya command associated with the fabric command.
  // The maya command does nothing since all the logic is done
  // by the fabric command framework.
  if(FabricCommandManagerCallback::GetManagerCallback()->isCommandCreatedFromManagerCallback())
  {
    m_isUndoable = FabricCommandManagerCallback::GetManagerCallback()->isCommandCanUndo();
    setHistoryOn(true); 
    FabricCommandManagerCallback::GetManagerCallback()->commandCreatedFromManagerCallback(false);
  }
  
  // Create the fabric command. The maya command is created 
  // after by the CommandManager/CommandManagerCallback.
  else
  {
    m_isUndoable = false;
    setHistoryOn(false);

    // Get the command args.
    QMap<QString, QString > cmdArgs;
    for(unsigned int i=1; i<args.length(&status); ++i)
    {
      QString key = args.asString(i, &status).asChar();
      i += 1;
      QString arg = args.asString(i, &status).asChar();
      cmdArgs[key] = arg;
    }
    
    CommandManager::getCommandManager()->createCommand(
      args.asString(0, &status).asChar(), 
      cmdArgs);
  }
  
  status = MS::kSuccess;
  FABRIC_MAYA_CATCH_END("FabricCommand::doIt");
  return status;
}

MStatus FabricCommand::undoIt()
{
  FABRIC_MAYA_CATCH_BEGIN();
  CommandManager::getCommandManager()->undoCommand();
  return MS::kSuccess;
  FABRIC_MAYA_CATCH_END("FabricCommand::undoIt");
  return MS::kFailure;
}

MStatus FabricCommand::redoIt()
{
  FABRIC_MAYA_CATCH_BEGIN();
  CommandManager::getCommandManager()->redoCommand();
  return MS::kSuccess;
  FABRIC_MAYA_CATCH_END("FabricCommand::redoIt");
  return MS::kFailure;
}
