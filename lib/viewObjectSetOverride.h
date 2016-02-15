// Copyright 2015 Autodesk, Inc.  All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license agreement
// provided at the time of installation or download, or which otherwise
// accompanies this software in either electronic or hard copy form.
#include <iostream>
#include <maya/MViewport2Renderer.h>
#include <maya/MSelectionList.h>
#include <maya/MFnSet.h>

using namespace MHWRender;

// Class which filters what to render in a scene draw by
// returning the objects in a named set as the object set filter.
// Has the option of what to do as the clear operation.
// Usage can be to clear on first set draw, and and not clear
// on subsequent draws.
class ObjectSetSceneRender : public MSceneRender
{
	public:
		ObjectSetSceneRender( const MString& name, const MString setName, unsigned int clearMask ) 
		: MSceneRender( name )
		, mSetName( setName ) 
		, mClearMask( clearMask )
		{}

		// Return filtered list of items to draw
		virtual const MSelectionList* objectSetOverride();

		// Return clear operation to perform
		virtual MHWRender::MClearOperation & clearOperation();

	protected:
		MSelectionList mFilterSet;
		MString mSetName;
		unsigned int mClearMask;
};

// Render override which draws 2 sets of objects in multiple "passes" (multiple scene renders)
// by using a filtered draw for each pass.
class ViewObjectSetOverride : public MRenderOverride
{
	public:
		ViewObjectSetOverride( const MString& name );
		~ViewObjectSetOverride();
		virtual MHWRender::DrawAPI supportedDrawAPIs() const;
		virtual bool startOperationIterator();
		virtual MHWRender::MRenderOperation* renderOperation();

		virtual bool nextRenderOperation();
		
		// UI name to appear as renderer 
		virtual MString uiName() const;

	protected:
		ObjectSetSceneRender* mRenderSet1;
		ObjectSetSceneRender* mRenderSet2;
		MPresentTarget* mPresentTarget;
		int mOperation;
		MString mUIName;
};
