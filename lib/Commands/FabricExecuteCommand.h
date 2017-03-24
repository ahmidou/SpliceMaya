//
// Copyright (c) 2010-2017, Fabric Software Inc. All rights reserved.
//

#pragma once

#include <maya/MArgList.h>
#include <maya/MPxCommand.h>

class FabricExecuteCommand : public MPxCommand
{
  /**
    FabricExecuteCommand is used to create a Fabric command from maya. 
    FabricExecuteCommand are not pushed to the maya stack and are not.
    logged. When the Fabric command is  created, a FabricCommand that
    represents the fabric command in maya is automatically constructed.
 
    Usage:    
    - C++:
      #include <maya/MGlobal.h>
      
      MString cmd = "FabricExecuteCommand cmdName";
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

      cmd = "FabricExecuteCommand cmdName"
      cmd = cmd + " "
      cmd = cmd + "arg_1_name \"arg_1_value\""
      cmd = cmd + ...
      cmd = cmd + " "
      cmd = cmd + "arg_n_name \"arg_n_value\""

      om.MGlobal_executeCommandOnIdle(cmd)
  */

  public:
    FabricExecuteCommand();

    virtual ~FabricExecuteCommand();

    /// Implementation of MPxCommand.
    static void* creator();
    
    /// Implementation of MPxCommand.
    virtual bool isUndoable() const;

    /// Implementation of MPxCommand.
    virtual MStatus doIt(
      const MArgList &args
      );
};
