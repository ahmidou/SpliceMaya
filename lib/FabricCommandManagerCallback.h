//
// Copyright (c) 2010-2017, Fabric Software Inc. All rights reserved.
//

#pragma once

#include <QObject>
#include <FabricUI/Commands/BaseCommand.h>
 
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
    
    /// Plugs and initializes the command system.
    /// To call from the plugin.
    void plug();

    /// Unplugs and deletes the command system.
    /// To call from the plugin.
    void unplug();
 
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
    /// \param addedToStack If true, the command has been pushed in the manager stack.
    void onCommandDone(
    	FabricUI::Commands::BaseCommand *cmd,
      bool addedToStack
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
};
