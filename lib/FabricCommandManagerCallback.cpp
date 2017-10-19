//
// Copyright (c) 2010-2017, Fabric Software Inc. All rights reserved.
//

#include <sstream>
#include <maya/MGlobal.h>
#include <maya/MString.h>
#include "FabricCommand.h"
#include "FabricDFGWidget.h"
#include "FabricMayaException.h"
#include "FabricRenderCallback.h"
#include <FabricUI/Util/RTValUtil.h>
#include "FabricCommandManagerCallback.h"
#include <FabricUI/Commands/CommandHelpers.h>
#include <FabricUI/Commands/KLCommandManager.h>
#include <FabricUI/Commands/KLCommandRegistry.h>
#include <FabricUI/Commands/CommandRegistration.h>
#include <FabricUI/Commands/BaseScriptableCommand.h>
#include <FabricUI/DFG/Tools/DFGToolsCommandRegistration.h>
#include <FabricUI/Dialog/DialogCommandRegistration.h>
#include <FabricUI/Commands/BaseRTValScriptableCommand.h>
#include <FabricUI/Application/FabricApplicationStates.h>
#include <FabricUI/OptionsEditor/Commands/OptionEditorCommandRegistration.h>

using namespace FabricCore;
using namespace FabricUI::Util;
using namespace FabricUI::Commands;

bool FabricCommandManagerCallback::s_instanceFlag = false;
FabricCommandManagerCallback* FabricCommandManagerCallback::s_cmdManagerMayaCallback = 0;

FabricDFGPVToolsNotifierCallBack::FabricDFGPVToolsNotifierCallBack()
  : QObject()
  , m_toolsDFGPVNotifierRegistry(0)
{
  m_toolsDFGPVNotifierRegistry = new FabricUI::DFG::DFGPVToolsNotifierRegistry();
  FabricUI::DFG::DFGToolsCommandRegistration::RegisterCommands(
    m_toolsDFGPVNotifierRegistry
    );

  QObject::connect(
    m_toolsDFGPVNotifierRegistry,
    SIGNAL(toolUpdated(QString const&)),
    this,
    SLOT(onToolUpdated(QString const&))
    );

  QObject::connect(
    m_toolsDFGPVNotifierRegistry,
    SIGNAL(toolRegistered(QString const&)),
    this,
    SLOT(onToolRegistered(QString const&))
    );
}

FabricDFGPVToolsNotifierCallBack::~FabricDFGPVToolsNotifierCallBack()
{
  delete m_toolsDFGPVNotifierRegistry;
  m_toolsDFGPVNotifierRegistry = 0;
}

void FabricDFGPVToolsNotifierCallBack::clear()
{
  m_toolsDFGPVNotifierRegistry->unregisterAllPathValueTools();
}

void FabricDFGPVToolsNotifierCallBack::onToolUpdated(
  QString const& toolPath)
{
  MGlobal::executeCommandOnIdle("refresh;", false);
}

void FabricDFGPVToolsNotifierCallBack::onToolRegistered(
  QString const& toolPath)
{
  MString cmd = "source \"FabricManipulationTool.mel\"; fabricManipulationActivateToolIfNot";
  MStatus commandStatus = MGlobal::executeCommandOnIdle(cmd, false);
    if (commandStatus != MStatus::kSuccess)
      MGlobal::displayError(
        "FabricDFGPVToolsNotifierCallBack::onToolRegistered, cannot execute 'FabricManipulationTool.mel::fabricManipulationActivateToolIfNot' command"
        );
}

FabricCommandManagerCallback::FabricCommandManagerCallback()
  : QObject()
  , m_dfgPVToolsNotifierCallBack(0)
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
  bool canUndo)
{
  FABRIC_MAYA_CATCH_BEGIN();

  if(cmd->canLog())
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
          if( rtValScriptCmd ) {
            QString path = rtValScriptCmd->getRTValArgPath( key ).toUtf8().constData();
            if( !path.isEmpty() ) {
              encodeArg( key, fabricCmd );
              encodeArg( CommandHelpers::encodeJSON( CommandHelpers::castToPathValuePath( path ) ),
                          fabricCmd );
            } else {
              RTVal val = rtValScriptCmd->getRTValArgValue( key );
              // Don't encode if null
              if( val.isValid() ) {
                encodeArg( key, fabricCmd );
                encodeArg(
                  CommandHelpers::encodeJSON( RTValUtil::toJSON( val ) ),
                  fabricCmd
                );
              }
            }
          }
          else {
            encodeArg( key, fabricCmd );
            encodeArg(
              scriptCmd->getArg( key ),
              fabricCmd
            );
          }
        }
      }
    }

    // Indicates that the command has been created already.
    // so we don't re-create it when constructing the maya command.
    m_commandCreatedFromManagerCallback = true;
    m_commandCanUndo = canUndo;
      
    MGlobal::executeCommandOnIdle(
      fabricCmd.str().c_str(), 
      true
      );
  }

  FABRIC_MAYA_CATCH_END("FabricCommandManagerCallback::onCommandDone");
}

void FabricCommandManagerCallback::init(
  FabricCore::Client client)
{
  FABRIC_MAYA_CATCH_BEGIN();

  new FabricUI::Application::FabricApplicationStates(client);
  
  KLCommandRegistry *registry = new KLCommandRegistry();
  CommandRegistry::setCommandRegistrySingleton(registry);
  registry->synchronizeKL();
   
  KLCommandManager *manager = new KLCommandManager();
  CommandManager::setCommandManagerSingleton(manager);

  QObject::connect(
    manager,
    SIGNAL(commandDone(FabricUI::Commands::BaseCommand*, bool)),
    this,
    SLOT(onCommandDone(FabricUI::Commands::BaseCommand*, bool))
    );

  FabricUI::Commands::CommandRegistration::RegisterCommands();
  FabricUI::OptionsEditor::OptionEditorCommandRegistration::RegisterCommands();
  FabricUI::Dialog::DialogCommandRegistration::RegisterCommands();
  // Support tool commands
  m_dfgPVToolsNotifierCallBack = new FabricDFGPVToolsNotifierCallBack();

  FABRIC_MAYA_CATCH_END("FabricCommandManagerCallback::init");
}

void FabricCommandManagerCallback::clear()
{  
  FABRIC_MAYA_CATCH_BEGIN();

  if(CommandManager::isInitalized())
  {
    CommandManager::getCommandManager()->clear();

    FabricUI::Application::FabricApplicationStates *appStates =  FabricUI::Application::FabricApplicationStates::GetAppStates();
    delete appStates;
    appStates = 0;

    CommandManager *manager = CommandManager::getCommandManager();
    delete manager;
    manager = 0;

    CommandRegistry *registry =  CommandRegistry::getCommandRegistry();
    delete registry;
    registry = 0;

    delete m_dfgPVToolsNotifierCallBack;
    m_dfgPVToolsNotifierCallBack = 0;
  }

  FABRIC_MAYA_CATCH_END("FabricCommandManagerCallback::clear");
}

void FabricCommandManagerCallback::reset()
{
  FABRIC_MAYA_CATCH_BEGIN();

  if(CommandManager::isInitalized())
  {
    CommandManager::getCommandManager()->clear();

    KLCommandRegistry *registry = qobject_cast<KLCommandRegistry*>(
      CommandRegistry::getCommandRegistry());
    registry->synchronizeKL();

    m_dfgPVToolsNotifierCallBack->clear();
  }
  
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

const char * FabricDFGDeleteAllPVToolsCommand::getName() 
{ 
  return "FabricDFGDeleteAllPVToolsCommand"; 
}
    
void* FabricDFGDeleteAllPVToolsCommand::creator()
{
  return new FabricDFGDeleteAllPVToolsCommand();
}

MStatus FabricDFGDeleteAllPVToolsCommand::doIt(
  const MArgList &args)
{
  FabricCommandManagerCallback *cmdCallback = FabricCommandManagerCallback::GetManagerCallback();
  if(cmdCallback)
  {
    FabricDFGPVToolsNotifierCallBack *toolCallback = cmdCallback->m_dfgPVToolsNotifierCallBack;
    if(toolCallback)
    {
      toolCallback->clear();
      return MS::kSuccess;
    }
  }
  return MS::kFailure;
}

bool FabricDFGDeleteAllPVToolsCommand::isUndoable() const 
{ 
  return false; 
}
