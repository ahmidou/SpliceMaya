//-
// Copyright 2015 Autodesk, Inc.  All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk license agreement
// provided at the time of installation or download, or which otherwise
// accompanies this software in either electronic or hard copy form.
//+
#include "viewObjectSetOverride.h"

// Return filtered list of items to draw
const MSelectionList* ObjectSetSceneRender::objectSetOverride()
{
	MSelectionList list; 
	list.add( mSetName );
	
	MObject obj; 
	list.getDependNode( 0, obj );
	
	MFnSet set( obj ); 
	set.getMembers( mFilterSet, true );
	
	return NULL;//&mFilterSet;
}

// Return clear operation to perform
MHWRender::MClearOperation & ObjectSetSceneRender::clearOperation()
{
	mClearOperation.setMask( mClearMask );
	return mClearOperation;
}

ViewObjectSetOverride::ViewObjectSetOverride( const MString& name ) 
: MHWRender::MRenderOverride( name ) 
, mUIName("Multi-pass filtered object-set renderer")
, mOperation(0)
{
	const MString render1Name("Render Set 1");
	const MString render2Name("Render Set 2");
	const MString set1Name("set1");
	const MString set2Name("set2");
	const MString presentName("Present Target");

	// Clear + render set 1
	mRenderSet1 = new ObjectSetSceneRender( render1Name, set1Name,  (unsigned int)MHWRender::MClearOperation::kClearAll );
	// Don't clear and render set 2
	mRenderSet2 = new ObjectSetSceneRender( render2Name, set2Name,  (unsigned int)MHWRender::MClearOperation::kClearNone );
	// Present results
	mPresentTarget = new MPresentTarget( presentName ); 
}

ViewObjectSetOverride::~ViewObjectSetOverride()
{
	delete mRenderSet1; mRenderSet1 = NULL;
	delete mRenderSet2; mRenderSet2 = NULL;
	delete mPresentTarget; mPresentTarget = NULL;
}

MHWRender::DrawAPI ViewObjectSetOverride::supportedDrawAPIs() const
{
	// this plugin supports both GL and DX
	return (MHWRender::kOpenGL); // | MHWRender::kOpenGLCoreProfile | MHWRender::kDirectX11);
}

bool ViewObjectSetOverride::startOperationIterator()
{
	mOperation = 0; 
	return true;
}

MHWRender::MRenderOperation* ViewObjectSetOverride::renderOperation()
{
	switch( mOperation )
	{
		case 0 : return mRenderSet1;
		case 1 : return mRenderSet2;
		case 2 : return mPresentTarget;
	}
	return NULL;
}

bool ViewObjectSetOverride::nextRenderOperation()
{
	mOperation++; 
	return mOperation < 3 ? true : false;
}

// UI name to appear as renderer 
MString ViewObjectSetOverride::uiName() const
{
	return mUIName;
}
 