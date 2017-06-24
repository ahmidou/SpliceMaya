//
// Copyright (c) 2010-2017, Fabric Software Inc. All rights reserved.
//

#pragma once

#include <QObject>
#include <FabricUI/Commands/BaseCommand.h>
 
class CommandManagerMayaCallback : public QObject
{
  /**
    CommandManagerQtCallback is connected to the CommandManagerCallback.
    It adds the commands into the maya undo stack when they are created
    from Canvas. 
  */

  Q_OBJECT
    
  public:
    CommandManagerMayaCallback();
    
    virtual ~CommandManagerMayaCallback();

 	  /// Gets the manager callback singleton.
    /// Thows an error if the manager callback has not been created.
    static CommandManagerMayaCallback* GetCommandManagerMayaCallback();
    
    static void plug();

    static void clear();

    static void unplug();

  private slots:
    /// \internal
    /// Called when a command has been pushed to the manager. 
    void onCommandDone(
    	FabricUI::Commands::BaseCommand *cmd,
      bool addToStack,
      bool replaceLog
    	);

  private:
  	/// CommandManagerMayaCallback singleton, set from Constructor.
    static CommandManagerMayaCallback *s_cmdManagerMayaCallback;
    /// Check if the singleton has been set.
    static bool s_instanceFlag;
};
