#ifndef ViewOverrideSimple_h_
#define ViewOverrideSimple_h_

 
#include <FabricCore.h>
#include "Foundation.h"
#include <FabricSplice.h>
#include <maya/MColor.h>
#include <maya/MString.h>
#include <maya/MDrawContext.h>
#include <maya/MViewport2Renderer.h>

 
class ViewOverrideSimple : public MHWRender::MRenderOverride {

  public:
    ViewOverrideSimple(const MString &name, const MString &viewportName);
    virtual ~ViewOverrideSimple();
    virtual MHWRender::DrawAPI supportedDrawAPIs() const;
    virtual MStatus setup( const MString & destination );
    virtual MStatus cleanup();
    virtual bool startOperationIterator();
    virtual MHWRender::MRenderOperation * renderOperation();
    virtual bool nextRenderOperation();
    virtual MString uiName() const { return mUIName; }
        
    void enable();

  protected:
    MString mViewportName;
    MString mUIName;
    MHWRender::MRenderOperation* mOperations[4];
    MString mOperationNames[4];
    int mCurrentOperation;
    bool mEnable;
};


class simpleViewRenderSceneRender : public MHWRender::MSceneRender {
  public:
    simpleViewRenderSceneRender(const MString &name, const MString &viewportName);
    virtual MHWRender::MClearOperation& clearOperation();
};


bool isRTRRenderEnabled();
void enableRTRRender(bool enable);

class RTRRender : public MHWRender::MUserRenderOperation {

  public:
    RTRRender(const MString &name, const MString &viewportName);
    virtual MStatus execute(const MHWRender::MDrawContext &context);
    
    static FabricCore::RTVal mRTRRender;
    static void invalidateHostToRTRCallback();

  private:
    bool initRTRRender();
    MString mViewportName;
};
#endif
