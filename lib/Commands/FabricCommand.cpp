//
// Copyright (c) 2010-2017, Fabric Software Inc. All rights reserved.
//
 
#include "FabricCommand.h"
#include "FabricSpliceHelpers.h"
#include "CommandManagerMayaCallback.h"
#include <FabricUI/Commands/CommandManager.h>

using namespace FabricUI;
using namespace FabricUI::Commands;
 
MString FabricCommand::createFromManagerCallback = "FabricMayaCommand_createFromManagerCallback";

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
  try
  {
    // Create the maya command.
    if(CommandManagerMayaCallback::GetManagerCallback()->isCommandCreatedFromManagerCallback())
    {
      m_isUndoable = true;
      setHistoryOn(true); 
      CommandManagerMayaCallback::GetManagerCallback()->commandCreatedFromManagerCallback(false);
    }
    
    // Create the fabric command.
    // The maya will be created after.
    else
    {
      m_isUndoable = false;
      setHistoryOn(false);

      // Get the command args.
      QMap<QString, QString > cmdArgs;
      for(unsigned int i=1; i<args.length(&status); ++i)
        cmdArgs[args.asString(i, &status).asChar()] = args.asString(++i, &status).asChar();

      CommandManager::getCommandManager()->createCommand(
        args.asString(0, &status).asChar(), 
        cmdArgs);
    }
    
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
    CommandManager::getCommandManager()->undoCommand();
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
    CommandManager::getCommandManager()->redoCommand();
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
