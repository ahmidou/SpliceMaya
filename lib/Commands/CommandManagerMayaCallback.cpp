//
// Copyright (c) 2010-2017, Fabric Software Inc. All rights reserved.
//

#include <sstream>
#include <iostream>
#include <maya/MGlobal.h>
#include "FabricDFGWidget.h"
#include "FabricSpliceHelpers.h"
#include "CommandManagerMayaCallback.h"
#include <FabricUI/Commands/CommandManager.h>
#include <FabricUI/Commands/KLCommandManager.h>
#include <FabricUI/Commands/KLCommandRegistry.h>
#include <FabricUI/Commands/BaseScriptableCommand.h>
#include <FabricUI/Commands/BaseRTValScriptableCommand.h>
#include <FabricUI/Application/FabricApplicationStates.h>

using namespace FabricUI;
using namespace FabricUI::Commands;
using namespace FabricCore;

bool CommandManagerMayaCallback::s_instanceFlag = false;
CommandManagerMayaCallback* CommandManagerMayaCallback::s_cmdManagerMayaCallback = 0;

CommandManagerMayaCallback::CommandManagerMayaCallback()
  : QObject()
{
  // std::cout 
  //   << "CommandManagerMayaCallback::CommandManagerMayaCallback "
  //   << std::endl;

  try
  {
    CommandManager *manager = CommandManager::getCommandManager();

    std::cout 
      << "CommandManagerMayaCallback::CommandManagerMayaCallback "
      << QObject::connect(
        manager,
        SIGNAL(commandDone(BaseCommand*, bool, bool)),
        this,
        SLOT(onCommandDone(BaseCommand*, bool, bool))
        )
        << std::endl;
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

void CommandManagerMayaCallback::onCommandDone(
  BaseCommand *cmd,
  bool addToStack,
  bool replaceLog)
{
  // std::cout 
  //   << "CommandManagerMayaCallback::onCommandDone "
  //   << cmd->getName().toUtf8().constData() 
  //   << std::endl;
  // Construct a Maya 'FabricCommand'  
  // that represents the Fabric command.
  std::stringstream fabricCmd;

  // Maya command name.
  fabricCmd << "FabricCommand";

  // Fabric command name.
  encodeArg(cmd->getName(), fabricCmd);
   
  // Fabric command args.
  BaseScriptableCommand *scriptCmd = qobject_cast<BaseScriptableCommand*>(cmd);

  if(scriptCmd)
  {
    // Check if it's a BaseRTValScriptableCommand,
    // to know how to cast the string.
    BaseRTValScriptableCommand *rtValScriptCmd = qobject_cast<BaseRTValScriptableCommand*>(cmd);
    foreach(QString key, scriptCmd->getArgKeys())
    {
      std::cout 
        << "CommandManagerMayaCallback::key "
        << key.toUtf8().constData() 
        << std::endl;

      encodeArg(key, fabricCmd);
      if(rtValScriptCmd)
      {
        std::cout 
          << "CommandManagerMayaCallback::path "
          << rtValScriptCmd->getRTValArgPath(key).toUtf8().constData() 
          << std::endl;
      }
      //   encodeRTValArg(scriptCmd->getArg(key), fabricCmd);
      // else
      //   encodeArg(scriptCmd->getArg(key), fabricCmd);
    }
  }

  std::cout 
    << "CommandManagerMayaCallback::onCommandDone "
    << fabricCmd.str().c_str() 
    << std::endl;

  // Create the maya command.
  if(addToStack)
    MGlobal::executeCommandOnIdle(fabricCmd.str().c_str(), true);
}

void CommandManagerMayaCallback::plug()
{
  FabricCore::Client client = FabricDFGWidget::GetCoreClient();
  new Application::FabricApplicationStates(client);
  
  KLCommandRegistry *registry = new KLCommandRegistry();
  registry->synchronizeKL();
  
  new KLCommandManager();
    
  CommandManagerMayaCallback::GetCommandManagerMayaCallback();
}

void CommandManagerMayaCallback::clear()
{
  CommandManager::getCommandManager()->clear();
}

void CommandManagerMayaCallback::unplug()
{
  clear();

  CommandManager *manager = CommandManager::getCommandManager();
  delete manager;
  manager = 0;

  CommandRegistry *registry =  CommandRegistry::getCommandRegistry();
  delete registry;
  registry = 0;

  CommandManagerMayaCallback *callback = GetCommandManagerMayaCallback();
  delete callback;
  callback = 0;
}