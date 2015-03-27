#ifndef _SPLICERENDERCALLBACK_H_
#define _SPLICERENDERCALLBACK_H_


#include "Foundation.h"
#include "plugin.h"
#include <FabricCore.h>
#include <maya/M3dView.h>

bool isRTRPassEnabled();
void enableRTRPass(bool enable);

class FabricSpliceRenderCallback
{
public:
  static void draw(const MString &str, void *clientData);
  static FabricCore::RTVal & getDrawContext(M3dView & view);
  static FabricCore::RTVal sDrawContext;
};

#endif
