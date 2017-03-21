//
// Copyright (c) 2010-2017, Fabric Software Inc. All rights reserved.
//

#include <sstream>
#include <maya/MGlobal.h>
#include "CommandManagerMayaCallback.h"
#include <FabricUI/Commands/CommandManager.h>
#include <FabricUI/Commands/BaseScriptableCommand.h>

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
    printf(
      "CommandManagerMayaCallback::CommandManagerMayaCallback, exception: %s\n", 
      e.c_str());
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

void CommandManagerMayaCallback::onCommandPushedCallback(
  FabricUI::Commands::BaseCommand *cmd)
{
  // Construct a 'FabricCanvasCommand'  
  // that represents the Fabric command.
  std::stringstream cmdArgs;

  // Maya command name.
  cmdArgs << "FabricCanvasCommand";

  // Fabric command name.
  encodeArg(
    cmd->getName(),
    cmdArgs
    );
   
  // Fabric command args.
  FabricUI::Commands::BaseScriptableCommand *scriptCmd = 
    dynamic_cast<FabricUI::Commands::BaseScriptableCommand *>(
      cmd
      );

  // if(scriptCmd)
  // {
  //   QMap<QString, QString> args = scriptCmd->getArgs();
  //   QMapIterator<QString, QString> argsIt(
  //     args
  //     );

  //   while(argsIt.hasNext()) 
  //   {
  //     argsIt.next();

  //     encodeArg(
  //       argsIt.key(),
  //       cmdArgs
  //       );

  //     encodeArg(
  //       argsIt.value(),
  //       cmdArgs
  //       );
  //   }
  // }

  // Create the maya command.
  MGlobal::executeCommandOnIdle(
    cmdArgs.str().c_str(),
    true
    );
}
