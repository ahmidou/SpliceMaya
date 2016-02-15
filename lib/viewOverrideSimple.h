#ifndef ViewOverrideSimple_h_
#define ViewOverrideSimple_h_

 
// Use of this software is subject to the terms of the Autodesk license agreement
// provided at the time of installation or download, or which otherwise
// accompanies this software in either electronic or hard copy form.
#include <FabricCore.h>
#include "Foundation.h"
#include <FabricSplice.h>
#include <maya/MColor.h>
#include <maya/MString.h>
#include <maya/MDrawContext.h>
#include <maya/MViewport2Renderer.h>

// Simple override class derived from MRenderOverride
class ViewOverrideSimple : public MHWRender::MRenderOverride {

  public:
    ViewOverrideSimple(const MString &clientContextID, const MString &name, const MString &viewportName);
    virtual ~ViewOverrideSimple();
    virtual MHWRender::DrawAPI supportedDrawAPIs() const;

    // Basic setup and cleanup
    virtual MStatus setup( const MString & destination );
    virtual MStatus cleanup();

    // Operation iteration methods
    virtual bool startOperationIterator();
    virtual MHWRender::MRenderOperation * renderOperation();
    virtual bool nextRenderOperation();

    // UI name
    virtual MString uiName() const { return mUIName; }
    
  protected:
    // UI name 
    MString mViewportName;
    MString mUIName;
    // Operations and operation names
    MHWRender::MRenderOperation* mOperations[3];
    MString mOperationNames[3];
    // Temporary of operation iteration
    int mCurrentOperation;
};

// Simple scene operation override to allow for clear color
// tracking.
class simpleViewRenderSceneRender : public MHWRender::MSceneRender {
  public:
    simpleViewRenderSceneRender(const MString &name, const MString &viewportName);
    virtual MHWRender::MClearOperation& clearOperation();
    virtual void preSceneRender(const MHWRender::MDrawContext &context);
    virtual void postSceneRender(const MHWRender::MDrawContext &context);

  private:
    FabricCore::RTVal mViewport;
    FabricCore::RTVal mHostToRTRCallback;
};

#endif
