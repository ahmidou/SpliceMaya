//
// Copyright (c) 2010-2017, Fabric Software Inc. All rights reserved.
//

#pragma once

#include <QObject>
#include <FabricCore.h>
#include <FabricUI/Commands/BaseCommand.h>
#include <FabricUI/DFG/Tools/DFGPVToolsNotifier.h>

class FabricCommandManagerCallback : public QObject
{
  /**
    CommandManagerQtCallback is connected to the CommandManager.
    It adds the commands into the maya undo stack when they are 
    created from Canvas. 
  */
  Q_OBJECT
    
  public:
    FabricCommandManagerCallback();
    
    virtual ~FabricCommandManagerCallback();

 	  /// Gets the manager callback singleton.
    /// Thows an error if the manager callback has not been created.
    static FabricCommandManagerCallback* GetManagerCallback();
    
    /// Initializes the command system.
    /// To do when the Fabric client is created the first time.
    void init(
      FabricCore::Client client
      );

    /// Clears and deletes the command system.
    void clear();
 
    /// Resets the commands system (clears manager stacks)
    void reset();

    /// To know if the command is created from maya or by the manager
    void commandCreatedFromManagerCallback(
      bool createdFromManagerCallback
      );

    /// To know if the command is created from maya or by the manager
    bool isCommandCreatedFromManagerCallback();

    /// To know if the command is created from maya or by the manager
    bool isCommandCanUndo();

  private slots:
    /// Called when a command has been pushed to the manager.
    /// \param cmd The command that has been pushed.
    /// \param canUndo If true, the command can undo and is on the top of the manager stack.
    void onCommandDone(
    	FabricUI::Commands::BaseCommand *cmd,
      bool canUndo
    	);

  private:
  	/// FabricCommandManagerCallback singleton, set from Constructor.
    static FabricCommandManagerCallback *s_cmdManagerMayaCallback;
    /// Check if the singleton has been set.
    static bool s_instanceFlag;
    /// To know if the command is created from maya or by the manager
    bool m_commandCreatedFromManagerCallback;
    /// To know if the last command created is undoable
    bool m_commandCanUndo;

    FabricUI::DFG::DFGPVToolsNotifierRegistry *m_toolsDFGPVNotifierRegistry;
};
