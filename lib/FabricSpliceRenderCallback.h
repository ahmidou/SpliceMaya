#ifndef _SPLICERENDERCALLBACK_H_
#define _SPLICERENDERCALLBACK_H_


#include "Foundation.h"
#include <FabricCore.h>
#include <maya/M3dView.h>

bool isMayaToRTRCallbackEnabled();
void enableMayaToRTRCallback(bool enable);

class FabricSpliceRenderCallback
{
	public: 
	  static FabricCore::RTVal sDrawContext;
	  static FabricCore::RTVal sMayaToRTRCallback;
		static void invalidateMayaToRTRCallback();
		static void setCameraTranformFromMaya(M3dView &view, FabricCore::RTVal &camera);
		static void setCameraProjectionFromMaya(M3dView &view, FabricCore::RTVal &camera);
		static void setCameraParamtersFromMaya(M3dView &view, FabricCore::RTVal &camera);
	  static FabricCore::RTVal& getMayaToRTRCallback(const MString &str, M3dView &view);
	  static void preRenderCallback(const MString &str, void *clientData);
	  static void postRenderCallback(const MString &str, void *clientData);
};

#endif
