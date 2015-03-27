#ifndef _PLUGIN_H_
#define _PLUGIN_H_

#include "Foundation.h"

MString getPluginName();
void loadMenu();
void unloadMenu();
bool isDestroyingScene();
MString getModuleFolder();
void mayaLogFunc(const char * message, unsigned int length = 0);
void mayaLogFunc(const MString & message);
void mayaErrorLogEnable(bool enable = true);
void mayaLogErrorFunc(const char * message, unsigned int length = 0);
void mayaLogErrorFunc(const MString & message);
void mayaClearError();
MStatus mayaErrorOccured();
void mayaRefreshFunc();
MString mayaGetLastLoadedScene();

#endif
