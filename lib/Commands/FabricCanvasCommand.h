//
// Copyright (c) 2010-2017, Fabric Software Inc. All rights reserved.
//

#pragma once

#include <maya/MArgList.h>
#include <maya/MPxCommand.h>

class FabricCanvasCommand : public MPxCommand
{
  /**
    FabricCanvasCommand represents in Maya a FabricUI::Commands::BaseCommands. 
    FabricCanvasCommand are constructed from the CommandManagerMayaCallback when
    a Fabric command has been created. FabricCanvasCommand doesn't own the Fabric 
    command. It's used to log the command in the script-editor and to synchronize 
    the maya and fabric undo-redo stack.

    FabricCanvasCommand shall not be used to create a Fabric command from Maya 
    script editor. Use 'FabricCanvasExecuteCommand' instead.
  */

  public:
    FabricCanvasCommand();

    virtual ~FabricCanvasCommand();

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
