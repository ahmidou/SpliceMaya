//
// Copyright (c) 2010-2017, Fabric Software Inc. All rights reserved.
//
 
#include <string>
#include "FabricCommand.h"
#include "FabricSpliceHelpers.h"
#include <FabricUI/Commands/BaseCommand.h>
#include <FabricUI/Commands/CommandManager.h>

FabricCommand::FabricCommand()
  : m_isUndoable(false)
{
}

FabricCommand::~FabricCommand()
{
  // Synchronize the maya and fabric stacks.
  // A command is destructed if it's has been undone
  // and a new command is created.
  try
  {
    FabricUI::Commands::CommandManager *manager = 
      FabricUI::Commands::CommandManager::getCommandManager();

    //manager->clearRedoStack();
  }

  catch (std::string &e) 
  {
    printf(
      "FabricCommand::~FabricCommand, exception: %s\n", 
      e.c_str());
  }
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

  try
  {
    FabricUI::Commands::CommandManager *manager = 
      FabricUI::Commands::CommandManager::getCommandManager();

    // Gets the last command pushed.
    FabricUI::Commands::BaseCommand *cmd = manager->getCommandAtIndex(
      manager->getStackIndex()
      );

    m_isUndoable = cmd->canUndo();
    setHistoryOn(true); 
  
    status = MS::kSuccess;
  }

  catch (std::string &e) 
  {
    mayaLogErrorFunc(
      QString(
        QString("FabricCommand::doIt, exception: ") + 
        e.c_str()
        ).toUtf8().constData()
      );
  }

  return status;
}

MStatus FabricCommand::undoIt()
{
  MStatus status = MS::kFailure;

  try
  {
    FabricUI::Commands::CommandManager *manager = 
      FabricUI::Commands::CommandManager::getCommandManager();

    manager->undoCommand();
    status = MS::kSuccess;
  }

  catch (std::string &e) 
  {
    mayaLogErrorFunc(
      QString(
        QString("FabricCommand::undoIt, exception: ") + 
        e.c_str()
        ).toUtf8().constData()
      );
  }

  return status;
}

MStatus FabricCommand::redoIt()
{
  MStatus status = MS::kFailure;

  try
  {
    FabricUI::Commands::CommandManager *manager = 
      FabricUI::Commands::CommandManager::getCommandManager();

    manager->redoCommand();
    status = MS::kSuccess;
  }

  catch (std::string &e) 
  {
    mayaLogErrorFunc(
      QString(
        QString("FabricCommand::redoIt, exception: ") + 
        e.c_str()
        ).toUtf8().constData()
      );
  }

  return status;
}
