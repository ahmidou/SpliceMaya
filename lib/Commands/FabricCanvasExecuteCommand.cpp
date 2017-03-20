//
// Copyright (c) 2010-2017, Fabric Software Inc. All rights reserved.
//
 
#include <QMap>
#include <string>
#include <QString>
#include "FabricCanvasExecuteCommand.h"
#include <FabricUI/Commands/CommandManager.h>

void* FabricCanvasExecuteCommand::creator()
{
  return new FabricCanvasExecuteCommand;
}

bool FabricCanvasExecuteCommand::isUndoable() const
{
  return false;
}

MStatus FabricCanvasExecuteCommand::doIt(
  const MArgList &args)
{
  MStatus status;

  try
  {
    setHistoryOn(false);

    // Get the command name.
    MString name = args.asString(0, &status);

    // Get the command args.
    QMap<QString, QString > cmdArgs;
    for(unsigned int i=1; i<args.length(&status); ++i)
    {
      QString key = args.asString(i, &status).asChar();
      QString value = args.asString(++i, &status).asChar();
      cmdArgs[key] = value;
    }

    FabricUI::Commands::CommandManager *manager = 
      FabricUI::Commands::CommandManager::GetCommandManager();

    // Create the fabric command.
    manager->createCommand(
      name.asChar(),
      cmdArgs
    );
   
    status = MS::kSuccess;
  }

  catch (std::string &e) 
  {
    printf(
      "FabricCanvasExecuteCommand::doIt, exception: %s\n", 
      e.c_str());
  }

  return status;
}
