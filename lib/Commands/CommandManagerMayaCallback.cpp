//
// Copyright (c) 2010-2017, Fabric Software Inc. All rights reserved.
//

#include <sstream>
#include <maya/MGlobal.h>
#include "FabricSpliceHelpers.h"
#include "CommandManagerMayaCallback.h"
#include <FabricUI/Commands/CommandManager.h>
#include <FabricUI/Commands/BaseScriptableCommand.h>
#include <FabricUI/Commands/BaseRTValScriptableCommand.h>

bool CommandManagerMayaCallback::s_instanceFlag = false;
CommandManagerMayaCallback* CommandManagerMayaCallback::s_cmdManagerMayaCallback = 0;

CommandManagerMayaCallback::CommandManagerMayaCallback()
  : QObject()
{
  try
  {
    FabricUI::Commands::CommandManager *manager = 
      FabricUI::Commands::CommandManager::GetCommandManager();

    QObject::connect(
      manager,
      SIGNAL(commandPushedCallback(FabricUI::Commands::BaseCommand *)),
      this,
      SLOT(onCommandPushedCallback(FabricUI::Commands::BaseCommand *))
      );
  }
  catch (std::string &e) 
  {
    mayaLogErrorFunc(
      QString(
        QString("CommandManagerMayaCallback::CommandManagerMayaCallback, exception: ") + 
        e.c_str()
        ).toUtf8().constData()
      );
  }
}

CommandManagerMayaCallback::~CommandManagerMayaCallback()
{
  s_instanceFlag = false;
}

CommandManagerMayaCallback *CommandManagerMayaCallback::GetCommandManagerMayaCallback()
{
  if(!s_instanceFlag)
  {
    s_cmdManagerMayaCallback = new CommandManagerMayaCallback();
    s_instanceFlag = true;
  }
  return s_cmdManagerMayaCallback;
}

inline void encodeArg(
  const QString &arg,
  std::stringstream &cmdArgs)
{
  cmdArgs << ' ';
  cmdArgs << arg.toUtf8().constData();
}
 
inline void encodeRTValArg(
  const QString &arg,
  std::stringstream &cmdArgs)
{
  cmdArgs << ' ';
  cmdArgs << "\"" << arg.toUtf8().constData() << "\"";
}

void CommandManagerMayaCallback::onCommandPushedCallback(
  FabricUI::Commands::BaseCommand *cmd)
{
  // Construct a Maya 'FabricCommand'  
  // that represents the Fabric command.
  std::stringstream fabricCmd;

  // Maya command name.
  fabricCmd << "FabricCommand";

  // Fabric command name.
  encodeArg(
    cmd->getName(),
    fabricCmd
    );
   
  // Fabric command args.
  FabricUI::Commands::BaseScriptableCommand *scriptCmd = 
    dynamic_cast<FabricUI::Commands::BaseScriptableCommand *>(
      cmd
      );

  if(scriptCmd)
  {
    // Check if it's a BaseRTValScriptableCommand,
    // to know how to cast the string.
    FabricUI::Commands::BaseRTValScriptableCommand *rtValScriptCmd = 
      dynamic_cast<FabricUI::Commands::BaseRTValScriptableCommand *>(
        cmd
        );

    QMap<QString, QString> args = scriptCmd->getArgs();
    QMapIterator<QString, QString> argsIt(
      args
      );

    while(argsIt.hasNext()) 
    {
      argsIt.next();

      encodeArg(
        argsIt.key(),
        fabricCmd
        );

      if(rtValScriptCmd)
        encodeRTValArg(
          argsIt.value(),
          fabricCmd
          );

      else
        encodeArg(
          argsIt.value(),
          fabricCmd
          );
    }
  }

  // Create the maya command.
  MGlobal::executeCommandOnIdle(
    fabricCmd.str().c_str(),
    true
    );
}
