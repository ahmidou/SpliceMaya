#ifndef _FOUNDATION_H_
#define _FOUNDATION_H_

// [andrew 20140609] these defines override Qt enums
#if defined(__linux__)
# include <X11/Xlib.h>
# undef KeyPress
# undef KeyRelease
#endif

#include <utility>
#include <limits>
#include <set>
#include <vector>
#include <map>
#include <list>

#include <stdexcept>
#include <exception>

#include <string>

#include <math.h>
#include <float.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <map>

#ifndef _BOOL
  #define _BOOL
#endif

#include <maya/MGlobal.h>
#include <maya/MObject.h>
#include <maya/MObjectArray.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MFileObject.h>
#include <maya/MPxCommand.h>
#include <maya/MSyntax.h>
#include <maya/MArgList.h>
#include <maya/MArgParser.h>
#include <maya/MTime.h>
#include <maya/MDagPath.h>
#include <maya/MDagPathArray.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MSelectionList.h>
#include <maya/MVector.h>
#include <maya/MQuaternion.h>
#include <maya/MMatrix.h>
#include <maya/MFloatMatrix.h>
#include <maya/MTransformationMatrix.h>
#include <maya/MPxNode.h>
#include <maya/MPxDeformerNode.h>
#include <maya/MPxEmitterNode.h>
#include <maya/MPxLocatorNode.h>
#include <maya/MDataHandle.h>
#include <maya/MDGContext.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnGenericAttribute.h>
#include <maya/MFnMatrixAttribute.h>
#include <maya/MFnStringData.h>
#include <maya/MFnMatrixData.h>
#include <maya/MAngle.h>
#include <maya/MFloatPointArray.h>
#include <maya/MFloatVectorArray.h>
#include <maya/MIntArray.h>
#include <maya/MFnIntArrayData.h>
#include <maya/MFloatArray.h>
#include <maya/MItGeometry.h>
#include <maya/MPxManipContainer.h>
#include <maya/MPxSelectionContext.h>
#include <maya/MPxContext.h>
#include <maya/MPxSelectionContext.h>
#include <maya/MPxContextCommand.h>
#include <maya/MPxToolCommand.h>
#include <maya/MArgDatabase.h>
#include <maya/MObjectHandle.h>
#include <maya/MDagModifier.h>
#include <maya/MFnPluginData.h>
#include <maya/MFnVectorArrayData.h>

#ifndef LONG
	typedef long LONG;
#endif

#ifndef ULONG
	typedef unsigned long ULONG;
#endif

#endif  // _FOUNDATION_H_
