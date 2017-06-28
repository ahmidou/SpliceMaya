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
    FabricCommand are constructed either from the CommandManagerMayaCallback 
    when a Fabric command has been created or explicitly in Maya.

    Usage:    
    - C++:
      #include <maya/MGlobal.h>
      
      MString cmd = "FabricCommand cmdName";
      cmd += " ";
      cmd += "arg_1_name \"arg_1_value\"";
      cmd += ...
      cmd += " ";
      cmd += "arg_n_name \"arg_n_value\"";

      MGlobal::executeCommandOnIdle(
        cmd,
        true
        );

    - Python:
      import maya.OpenMaya as om

      cmd = "FabricCommand cmdName"
      cmd = cmd + " "
      cmd = cmd + "arg_1_name \"arg_1_value\""
      cmd = cmd + ...
      cmd = cmd + " "
      cmd = cmd + "arg_n_name \"arg_n_value\""

      om.MGlobal_executeCommandOnIdle(cmd)  
  */
  public:
    FabricCommand();

    virtual ~FabricCommand();

    /// \internal
    /// To check if the command is created from 
    /// the fabric manager or explicitely from Maya.
    static MString createFromManagerCallback;

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
    /// \internal
    bool m_isUndoable;
};
