//
// Copyright (c) 2010-2017, Fabric Software Inc. All rights reserved.
//

#include <sstream>
#include <maya/MGlobal.h>
#include "FabricCommand.h"
#include "FabricDFGWidget.h"
#include "FabricMayaException.h"
#include <FabricUI/Util/RTValUtil.h>
#include "FabricCommandManagerCallback.h"
#include <FabricUI/Commands/CommandHelpers.h>
#include <FabricUI/Commands/KLCommandManager.h>
#include <FabricUI/Commands/KLCommandRegistry.h>
#include <FabricUI/Commands/BaseScriptableCommand.h>
#include <FabricUI/Tools/ToolsCommandRegistration.h>
#include <FabricUI/Dialog/DialogCommandRegistration.h>
#include <FabricUI/Commands/BaseRTValScriptableCommand.h>
#include <FabricUI/Application/FabricApplicationStates.h>
#include <FabricUI/OptionsEditor/Commands/OptionEditorCommandRegistration.h>

using namespace FabricCore;
using namespace FabricUI::Util;
using namespace FabricUI::Commands;

bool FabricCommandManagerCallback::s_instanceFlag = false;
FabricCommandManagerCallback* FabricCommandManagerCallback::s_cmdManagerMayaCallback = 0;

FabricCommandManagerCallback::FabricCommandManagerCallback()
  : QObject()
  , m_commandCreatedFromManagerCallback(false)
{
}

FabricCommandManagerCallback::~FabricCommandManagerCallback()
{
  s_instanceFlag = false;
}

FabricCommandManagerCallback *FabricCommandManagerCallback::GetManagerCallback()
{
  if(!s_instanceFlag)
  {
    s_cmdManagerMayaCallback = new FabricCommandManagerCallback();
    s_instanceFlag = true;
  }
  return s_cmdManagerMayaCallback;
}

inline void encodeArg(
  QString const&arg,
  std::stringstream &cmdArgs)
{
  cmdArgs << ' ';
  cmdArgs << arg.toUtf8().constData();
}
 
void FabricCommandManagerCallback::onCommandDone(
  BaseCommand *cmd,
  bool addedToStack,
  int canMergeID,
  int merge)
{
  FABRIC_MAYA_CATCH_BEGIN();

  if(cmd->canLog())
  {
    // Maya creates log commands after they've been performed, while in Fabric, it's possible 
    // to log a command and then update it's content using the merging framework. We create 
    // the maya command if the fabric command isn't merging or the merging is done.
    if( ( canMergeID == CommandManager::NoCanMergeID && merge == CommandManager::NoCanMerge) || 
        ( canMergeID != CommandManager::NoCanMergeID && merge == CommandManager::MergeDone )
      )
    {
      std::stringstream fabricCmd;
      fabricCmd << "FabricCommand";
      encodeArg(cmd->getName(), fabricCmd);

      // Fabric command args.
      BaseScriptableCommand *scriptCmd = qobject_cast<BaseScriptableCommand*>(cmd);

      if(scriptCmd)
      {
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
                encodeArg(CommandHelpers::encodeJSON(CommandHelpers::castToPathValuePath(path)),
                  fabricCmd
                  );

              else
                encodeArg(
                  CommandHelpers::encodeJSON(RTValUtil::toJSON(rtValScriptCmd->getRTValArgValue(key))), 
                  fabricCmd
                  );
            }
            else
              encodeArg(
                scriptCmd->getArg(key), 
                fabricCmd
                );
          }
        }
      }

      // Indicates that the command has been created already.
      // so we don't re-create it when constructing the maya command.
      m_commandCreatedFromManagerCallback = true;
      m_commandCanUndo = ( canMergeID != CommandManager::NoCanMergeID && merge == CommandManager::MergeDone )
        ? true
        : addedToStack;
      
        MGlobal::executeCommandOnIdle(
          fabricCmd.str().c_str(), 
          true
          );
    }
  }

  FABRIC_MAYA_CATCH_END("FabricCommandManagerCallback::onCommandDone");
}

void FabricCommandManagerCallback::plug()
{
  FABRIC_MAYA_CATCH_BEGIN();

  FabricCore::Client client = FabricDFGWidget::GetCoreClient();
  new FabricUI::Application::FabricApplicationStates(client);
  
  KLCommandRegistry *registry = new KLCommandRegistry();
  registry->synchronizeKL();
  
  KLCommandManager *manager = new KLCommandManager();
  
  QObject::connect(
    manager,
    SIGNAL(commandDone(FabricUI::Commands::BaseCommand*, bool, int, int)),
    this,
    SLOT(onCommandDone(FabricUI::Commands::BaseCommand*, bool, int, int))
    );

  FabricUI::OptionsEditor::OptionEditorCommandRegistration::RegisterCommands();
  FabricUI::Dialog::DialogCommandRegistration::RegisterCommands();
  FabricUI::Tools::ToolsCommandRegistration::RegisterCommands();

  FABRIC_MAYA_CATCH_END("FabricCommandManagerCallback::plug");
}

void FabricCommandManagerCallback::unplug()
{  
  FABRIC_MAYA_CATCH_BEGIN();

  CommandManager::getCommandManager()->clear();

  CommandManager *manager = CommandManager::getCommandManager();
  delete manager;
  manager = 0;

  CommandRegistry *registry =  CommandRegistry::getCommandRegistry();
  delete registry;
  registry = 0;

  FABRIC_MAYA_CATCH_END("FabricCommandManagerCallback::unplug");
}

void FabricCommandManagerCallback::reset()
{
  FABRIC_MAYA_CATCH_BEGIN();

  CommandManager::getCommandManager()->clear();

  KLCommandRegistry *registry = qobject_cast<KLCommandRegistry*>(
    CommandRegistry::getCommandRegistry());
  registry->synchronizeKL();

  FABRIC_MAYA_CATCH_END("FabricCommandManagerCallback::reset");
}

void FabricCommandManagerCallback::commandCreatedFromManagerCallback(
  bool createdFromManagerCallback)
{
  m_commandCreatedFromManagerCallback = createdFromManagerCallback;
}

bool FabricCommandManagerCallback::isCommandCreatedFromManagerCallback()
{
  return m_commandCreatedFromManagerCallback;
}

bool FabricCommandManagerCallback::isCommandCanUndo()
{
  return m_commandCanUndo;
}
