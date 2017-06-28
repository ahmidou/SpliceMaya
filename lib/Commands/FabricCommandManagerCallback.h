//
// Copyright (c) 2010-2017, Fabric Software Inc. All rights reserved.
//

#ifndef __MAYA_FABRIC_COMMAND_MANAGER_CALLBACK__
#define __MAYA_FABRIC_COMMAND_MANAGER_CALLBACK__

#include <QObject>
#include <FabricUI/Commands/BaseCommand.h>
 
namespace FabricMaya {
namespace Commands {

class FabricCommandManagerCallback : public QObject
{
  /**
    CommandManagerQtCallback is connected to the CommandManager.
    It adds the commands into the maya undo stack when they are created
    from Canvas. 
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

    /// \internal
    /// To know if the command is created from maya or
    /// by the manager
    void commandCreatedFromManagerCallback(
      bool createdFromManagerCallback
      );

    /// \internal
    /// To know if the command is created from maya or
    /// by the manager
    bool isCommandCreatedFromManagerCallback();

  private slots:
    /// \internal
    /// Called when a command has been pushed to the manager.
    /// \param cmd The command that has been pushed.
    /// \param addToStack If true, the command has been pushed in the manager stack.
    /// \param replaceLog If true, the log of the last pushed command must be udpated.
    void onCommandDone(
    	FabricUI::Commands::BaseCommand *cmd,
      bool addToStack,
      bool replaceLog
    	);

  private:
  	/// FabricCommandManagerCallback singleton, set from Constructor.
    static FabricCommandManagerCallback *s_cmdManagerMayaCallback;
    /// Check if the singleton has been set.
    static bool s_instanceFlag;
    /// \internal
    /// To know if the command is created from maya or
    /// by the manager
    bool m_createdFromManagerCallback;
};

} // namespace Commands
} // namespace FabricMaya

#endif // __MAYA_FABRIC_COMMAND_MANAGER_CALLBACK__
