//
// Copyright (c) 2010-2017 Fabric Software Inc. All rights reserved.
//

#pragma once

#include <QObject>
#include "Foundation.h"
#include <FabricSplice.h>
#include "FabricSpliceBaseInterface.h"

class FabricManipulationCmd : public MPxToolCommand {

  private:
    FabricCore::RTVal m_rtval_commands;
    
  public:
    FabricManipulationCmd(); 

    virtual ~FabricManipulationCmd(); 
    
    static void* creator();

    MStatus doIt(const MArgList& args);

    MStatus redoIt();

    MStatus undoIt();

    bool isUndoable() const;

    // We set the static commands pointer, and then construct the command. 
    static FabricCore::RTVal s_rtval_commands;
};


class FabricToolCmd : public MPxToolCommand {
  public:
    FabricToolCmd(); 

    virtual ~FabricToolCmd(); 
    
    static void* creator();

    MStatus doIt(const MArgList& args);

    MStatus redoIt();

    MStatus undoIt();

    bool isUndoable() const;
};


class FabricToolContext : public MPxContext {
  public:
    FabricToolContext();
    
    virtual void getClassName(MString & name) const;
    
    virtual void toolOnSetup(MEvent &event);
    
    virtual void toolOffCleanup();
    
    virtual MStatus doPress(MEvent &event);
    
    virtual MStatus doDrag(MEvent &event);
    
    virtual MStatus doRelease(MEvent &event);
    
    virtual MStatus doEnterRegion(MEvent &event);

    bool onEvent(QEvent *event);

  private:
    bool onIDEvent(QEvent *event, M3dView &view);

    bool onRTR2Event(QEvent *event, M3dView &view);
    
    FabricCore::RTVal mEventDispatcher;
};


class FabricToolContextCmd : public MPxContextCommand {
  public: 
    virtual MPxContext* makeObj();

    static void* creator();
};
