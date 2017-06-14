//
// Copyright (c) 2010-2017, Fabric Software Inc. All rights reserved.
//
 
#include <QMap>
#include <string>
#include <QString>
#include "FabricSpliceHelpers.h"
#include "FabricExecuteCommand.h"
#include <FabricUI/Commands/CommandManager.h>

FabricExecuteCommand::FabricExecuteCommand()
{
}

FabricExecuteCommand::~FabricExecuteCommand()
{
}

void* FabricExecuteCommand::creator()
{
  return new FabricExecuteCommand;
}

bool FabricExecuteCommand::isUndoable() const
{
  return false;
}

MStatus FabricExecuteCommand::doIt(
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
      FabricUI::Commands::CommandManager::getCommandManager();

    // Create the fabric command.
    manager->createCommand(name.asChar(), cmdArgs);
    status = MS::kSuccess;
  }

  catch (std::string &e) 
  {
    mayaLogErrorFunc(
      QString(
        QString("FabricExecuteCommand::doIt, exception: ") + 
        e.c_str()
        ).toUtf8().constData()
      );
  }

  return status;
}
