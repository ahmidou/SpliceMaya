//
// Copyright (c) 2010-2017, Fabric Software Inc. All rights reserved.
//
 
#include "FabricCommand.h"
#include "FabricSpliceHelpers.h"
#include "CommandManagerMayaCallback.h"
#include <FabricUI/Commands/CommandManager.h>

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
    std::cout
      << "\nFabricCommand::doIt " 
      << CommandManagerMayaCallback::GetManagerCallback()->isCommandCreatedFromManagerCallback() 
      << std::endl;

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

      MString name = args.asString(0, &status);

      // Get the command args.
      QMap<QString, QString > cmdArgs;
      for(unsigned int i=1; i<args.length(&status); ++i)
      {
        QString key = args.asString(i, &status).asChar();
        QString value = args.asString(++i, &status).asChar();

        std::cout << "key " << key.toUtf8().constData() << std::endl;
        std::cout << "value " << value.toUtf8().constData() << std::endl;
        cmdArgs[key] = value;
      }

      FabricUI::Commands::CommandManager *manager = 
        FabricUI::Commands::CommandManager::getCommandManager();
      std::cout << "Create Command" << std::endl;

      manager->createCommand(name.asChar(), cmdArgs);
      std::cout << manager->getContent(false).toUtf8().constData() << std::endl;

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
