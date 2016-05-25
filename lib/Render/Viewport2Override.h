//
// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
//

#ifndef __RTR_OVERRIDE_H__
#define __RTR_OVERRIDE_H__
 
#include "FabricSpliceRenderCallback.h"
#include <maya/MUiMessage.h>
#include <maya/MDrawContext.h>
#include <maya/MViewport2Renderer.h>

using namespace MHWRender;

const MString Viewport2Override_name = "Viewport2Override";

class Viewport2Override : public MRenderOverride {

  public:
    Viewport2Override(const MString &name);
    
    virtual ~Viewport2Override();

    virtual MString uiName() const;
    
    virtual DrawAPI supportedDrawAPIs() const;
        
    virtual MStatus setup(const MString& destination);
    
    virtual MStatus cleanup();
    
    virtual bool startOperationIterator();
    
    virtual MRenderOperation* renderOperation();
    
    virtual bool nextRenderOperation();

   
  private:
    int mCurrentOp;
    MString mOpNames[4];
    MRenderOperation* mOps[4];
};

#endif // __RTR_OVERRIDE_H__
