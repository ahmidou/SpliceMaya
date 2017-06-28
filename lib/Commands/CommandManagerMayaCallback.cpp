//
// Copyright (c) 2010-2017, Fabric Software Inc. All rights reserved.
//

#include <sstream>
#include <iostream>
#include <maya/MGlobal.h>
#include "FabricCommand.h"
#include "FabricDFGWidget.h"
#include "FabricSpliceHelpers.h"
#include <FabricUI/Util/RTValUtil.h>
#include "CommandManagerMayaCallback.h"
#include <FabricUI/Commands/CommandManager.h>
#include <FabricUI/Commands/KLCommandManager.h>
#include <FabricUI/Commands/KLCommandRegistry.h>
#include <FabricUI/Commands/CommandArgHelpers.h>
#include <FabricUI/Commands/BaseScriptableCommand.h>
#include <FabricUI/Commands/BaseRTValScriptableCommand.h>
#include <FabricUI/Application/FabricApplicationStates.h>

using namespace FabricUI;
using namespace FabricUI::Util;
using namespace FabricUI::Commands;
using namespace FabricCore;

bool CommandManagerMayaCallback::s_instanceFlag = false;
CommandManagerMayaCallback* CommandManagerMayaCallback::s_cmdManagerMayaCallback = 0;

CommandManagerMayaCallback::CommandManagerMayaCallback()
  : QObject()
  , m_createdFromManagerCallback(false)
{
}

CommandManagerMayaCallback::~CommandManagerMayaCallback()
{
  s_instanceFlag = false;
}

CommandManagerMayaCallback *CommandManagerMayaCallback::GetManagerCallback()
{
  if(!s_instanceFlag)
  {
    s_cmdManagerMayaCallback = new CommandManagerMayaCallback();
    s_instanceFlag = true;
  }
  return s_cmdManagerMayaCallback;
}

inline QString encodeJSONChars(
  QString const&string)
{
  QString result = string;
  result = result.replace("\"", "'").replace("\\", "\\\\").replace(" ", "");
  return result.replace("\r", "").replace("\n", "").replace("\t", "");
}

inline QString encodeJSON(
  QString const&string)
{
  return "\"" + encodeJSONChars(string) + "\"";
}

inline void encodeArg(
  const QString &arg,
  std::stringstream &cmdArgs)
{
  cmdArgs << ' ';
  cmdArgs << arg.toUtf8().constData();
}
 
void CommandManagerMayaCallback::onCommandDone(
  BaseCommand *cmd,
  bool addToStack,
  bool replaceLog)
{
  if(addToStack)
  {
    std::stringstream fabricCmd;
    fabricCmd << "FabricCommand";
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
        if(!scriptCmd->hasArgFlag(key, CommandArgFlags::DONT_LOG_ARG))
        {
          encodeArg(key, fabricCmd);
          if(rtValScriptCmd)
          {
            QString path = rtValScriptCmd->getRTValArgPath(key).toUtf8().constData();
            if(!path.isEmpty())
              encodeArg("\"<" + path + ">\"", fabricCmd);
            else
              encodeArg(
                CommandArgHelpers::encodeJSON(RTValUtil::toJSON(rtValScriptCmd->getRTValArgValue(key))), 
                  fabricCmd
                  );
          }
          else
            encodeArg(
              scriptCmd->getArg(key), 
              fabricCmd);
        }
      }
    }

    // Indicates that the command has been created already.
    // so we don't re-create it when constructing the maya command.
    m_createdFromManagerCallback = true;
    MGlobal::executeCommandOnIdle(fabricCmd.str().c_str(), true);
  }
}

void CommandManagerMayaCallback::plug()
{
  try
  {
    FabricCore::Client client = FabricDFGWidget::GetCoreClient();
    new Application::FabricApplicationStates(client);
    
    KLCommandRegistry *registry = new KLCommandRegistry();
    registry->synchronizeKL();
    
    KLCommandManager *manager = new KLCommandManager();
    
    QObject::connect(
      manager,
      SIGNAL(commandDone(FabricUI::Commands::BaseCommand*, bool, bool)),
      this,
      SLOT(onCommandDone(FabricUI::Commands::BaseCommand*, bool, bool))
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

void CommandManagerMayaCallback::reset()
{
  CommandManager::getCommandManager()->clear();

  KLCommandRegistry *registry = qobject_cast<KLCommandRegistry*>(
    CommandRegistry::getCommandRegistry());
  registry->synchronizeKL();
}

void CommandManagerMayaCallback::unplug()
{
  CommandManager::getCommandManager()->clear();

  CommandManager *manager = CommandManager::getCommandManager();
  delete manager;
  manager = 0;

  CommandRegistry *registry =  CommandRegistry::getCommandRegistry();
  delete registry;
  registry = 0;
}

void CommandManagerMayaCallback::commandCreatedFromManagerCallback(
  bool createdFromManagerCallback)
{
  m_createdFromManagerCallback = createdFromManagerCallback;
}

bool CommandManagerMayaCallback::isCommandCreatedFromManagerCallback()
{
  return m_createdFromManagerCallback;
}