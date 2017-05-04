//
// Copyright (c) 2010-2017, Fabric Software Inc. All rights reserved.
//

#pragma once

#include <maya/MArgList.h>
#include <maya/MPxCommand.h>

class FabricCommand : public MPxCommand
{
  /**
    FabricCommand represents in Maya a FabricUI::Commands::BaseCommands. 
    FabricCommand are constructed from the CommandManagerMayaCallback when
    a Fabric command has been created. FabricCommand doesn't own the Fabric 
    command. It's used to log the command in the script-editor and to synch-
    ronize the maya and fabric undo-redo stack.

    FabricCommand shall not be used to create a Fabric command from Maya 
    script editor. Use 'FabricExecuteCommand' instead.
  */

  public:
    FabricCommand();

    virtual ~FabricCommand();

    /// Implementation of MPxCommand.
    static void* creator();
    
    /// Implementation of MPxCommand.
    virtual bool isUndoable() const;

    /// Implementation of MPxCommand.
    virtual MStatus doIt(
      const MArgList &args
      );

    /// Implementation of MPxCommand.
    virtual MStatus undoIt();
  
    /// Implementation of MPxCommand.
    virtual MStatus redoIt();

  private:
    bool m_isUndoable;
};
