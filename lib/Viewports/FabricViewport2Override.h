//
// Copyright (c) 2010-2017 Fabric Software Inc. All rights reserved.
//

#ifndef __FABRIC_MAYA_VIEWPORT2_OVERRIDE__
#define __FABRIC_MAYA_VIEWPORT2_OVERRIDE__
 
#include <maya/MDrawContext.h>
#include <maya/MViewport2Renderer.h>

namespace FabricMaya {
namespace Viewports {

/// Name of the fabric viewport
const MString FabricViewport2Override_name = "Viewport2Override";

class FabricViewport2Override : public MHWRender::MRenderOverride 
{
  /**
    FabricViewport2Override overrides the native Maya Viewport 2.0
    iderto properly draws the fabric scene after maya (post draw pass) 
    since maya clears the depth buffer after drawing its scene.
  */
  public:
    FabricViewport2Override(
      const MString &name
      );
    
    /// Implementation of MRenderOverride.
    virtual ~FabricViewport2Override();

    /// Implementation of MRenderOverride.
    virtual MString uiName() const;
    
    /// Implementation of MRenderOverride.
    virtual MHWRender::DrawAPI supportedDrawAPIs() const;
        
    /// Implementation of MRenderOverride.
    virtual MStatus setup(
      const MString& destination
      );
    
    /// Implementation of MRenderOverride.
    virtual MStatus cleanup();
    
    /// Implementation of MRenderOverride.
    virtual bool startOperationIterator();
    
    /// Implementation of MRenderOverride.
    virtual MHWRender::MRenderOperation* renderOperation();
    
    /// Implementation of MRenderOverride.
    virtual bool nextRenderOperation();

  private:
    int mCurrentOp;
    MString mOpNames[4];
    MHWRender::MRenderOperation* mOps[4];
};

} // namespace Viewports
} // namespace FabricMaya

#endif // __FABRIC_MAYA_VIEWPORT2_OVERRIDE__
