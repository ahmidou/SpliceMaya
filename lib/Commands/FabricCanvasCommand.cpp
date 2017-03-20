//
// Copyright (c) 2010-2017, Fabric Software Inc. All rights reserved.
//
 
#include <string>
#include "FabricCanvasCommand.h"
#include <FabricUI/Commands/BaseCommand.h>
#include <FabricUI/Commands/CommandManager.h>

FabricCanvasCommand::FabricCanvasCommand()
  : m_isUndoable(false)
{
}

FabricCanvasCommand::~FabricCanvasCommand()
{
  // Synchronize the maya and fabric stacks.
  try
  {
    FabricUI::Commands::CommandManager *manager = 
      FabricUI::Commands::CommandManager::GetCommandManager();

    manager->clearRedoStack();
  }

  catch (std::string &e) 
  {
    printf(
      "FabricCanvasCommand::~FabricCanvasCommand, exception: %s\n", 
      e.c_str());
  }
}

void* FabricCanvasCommand::creator()
{
  return new FabricCanvasCommand;
}

bool FabricCanvasCommand::isUndoable() const
{
  return m_isUndoable;
}

MStatus FabricCanvasCommand::doIt(
  const MArgList &args)
{
  MStatus status;

  try
  {
    FabricUI::Commands::CommandManager *manager = 
      FabricUI::Commands::CommandManager::GetCommandManager();

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
    printf(
      "FabricCanvasCommand::doIt, exception: %s\n", 
      e.c_str());
  }

  return status;
}

MStatus FabricCanvasCommand::undoIt()
{
  MStatus status = MS::kFailure;

  try
  {
    FabricUI::Commands::CommandManager *manager = 
      FabricUI::Commands::CommandManager::GetCommandManager();

    manager->undoCommand();

    status = MS::kSuccess;
  }

  catch (std::string &e) 
  {
    printf(
      "FabricCanvasCommand::undoIt, exception: %s\n", 
      e.c_str());
  }

  return status;
}

MStatus FabricCanvasCommand::redoIt()
{
  MStatus status = MS::kFailure;

  try
  {
    FabricUI::Commands::CommandManager *manager = 
      FabricUI::Commands::CommandManager::GetCommandManager();

    manager->redoCommand();

    status = MS::kSuccess;
  }

  catch (std::string &e) 
  {
    printf(
      "FabricCanvasCommand::redoIt, exception: %s\n", 
      e.c_str());
  }

  return status;
}

 