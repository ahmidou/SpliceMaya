//
// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
//

#pragma once

#include "Foundation.h"
#include <maya/MString.h>
#include <maya/MString.h>

void initModuleFolder(MString moduleFolder);
MString getModuleFolder();
void mayaLogFunc(const char * message, unsigned int length = 0);
void mayaLogFunc(const MString & message);
void mayaErrorLogEnable(bool enable = true);
void mayaLogErrorFunc(const char * message, unsigned int length = 0);
void mayaLogErrorFunc(const MString & message);
void mayaKLReportFunc(const char * message, unsigned int length);
void mayaCompilerErrorFunc(unsigned int row, unsigned int col, const char * file, const char * level, const char * desc);
void mayaKLStatusFunc(const char * topic, unsigned int topicLength,  const char * message, unsigned int messageLength);
void mayaClearError();
MStatus mayaErrorOccured();
void mayaRefreshFunc();
MString mayaGetLastLoadedScene();
void mayaSetLastLoadedScene(MString scene);
