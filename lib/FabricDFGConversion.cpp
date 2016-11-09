//
// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
//

#include "FabricDFGConversion.h"
#include "FabricSpliceMayaData.h"
#include "FabricSpliceHelpers.h"
#include "FabricDFGProfiling.h"

#include <maya/MGlobal.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnCompoundAttribute.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MFloatVector.h>
#include <maya/MFnIntArrayData.h>
#include <maya/MFnDoubleArrayData.h>
#include <maya/MFnVectorArrayData.h>
#include <maya/MFnPointArrayData.h>
#include <maya/MIntArray.h>
#include <maya/MDoubleArray.h>
#include <maya/MVectorArray.h>
#include <maya/MPointArray.h>
#include <maya/MFloatVector.h>
#include <maya/MMatrix.h>
#include <maya/MFnMatrixAttribute.h>
#include <maya/MAngle.h>
#include <maya/MDistance.h>
#include <maya/MArrayDataBuilder.h>
#include <maya/MFnPluginData.h>
#include <maya/MFnMesh.h>
#include <maya/MFnMeshData.h>
#include <maya/MFnNurbsCurve.h>
#include <maya/MFnNurbsCurveData.h>
#include <maya/MFloatVectorArray.h>
#include <maya/MFnAnimCurve.h>

#define CORE_CATCH_BEGIN try {
#define CORE_CATCH_END } \
  catch (FabricCore::Exception e) { \
    mayaLogErrorFunc(e.getDesc_cstr()); \
  }

typedef std::map<std::string, DFGPlugToArgFunc> DFGPlugToArgFuncMap;
typedef std::map<std::string, DFGArgToPlugFunc> DFGArgToPlugFuncMap;
typedef DFGPlugToArgFuncMap::iterator DFGPlugToArgFuncIt;
typedef DFGArgToPlugFuncMap::iterator DFGArgToPlugFuncIt;

struct KLEuler{
  float x;
  float y;
  float z;
  int32_t order;
};

typedef float floatVec[3];

double dfgGetFloat64FromRTVal(FabricCore::RTVal rtVal)
{
  FabricCore::RTVal::SimpleData simpleData;
  if(!rtVal.maybeGetSimpleData(&simpleData))
    return DBL_MAX;
  if(simpleData.type == FEC_RTVAL_SIMPLE_TYPE_FLOAT32)  return simpleData.value.float32;
  if(simpleData.type == FEC_RTVAL_SIMPLE_TYPE_FLOAT64)  return simpleData.value.float64;
  
  if(simpleData.type == FEC_RTVAL_SIMPLE_TYPE_SINT32)   return simpleData.value.sint32;
  if(simpleData.type == FEC_RTVAL_SIMPLE_TYPE_UINT32)   return simpleData.value.uint32;
  
  if(simpleData.type == FEC_RTVAL_SIMPLE_TYPE_UINT8)    return simpleData.value.uint8;
  if(simpleData.type == FEC_RTVAL_SIMPLE_TYPE_UINT16)   return simpleData.value.uint16;
  if(simpleData.type == FEC_RTVAL_SIMPLE_TYPE_UINT64)   return double(simpleData.value.uint64);

  if(simpleData.type == FEC_RTVAL_SIMPLE_TYPE_SINT8)    return simpleData.value.sint8;
  if(simpleData.type == FEC_RTVAL_SIMPLE_TYPE_SINT16)   return simpleData.value.sint16;
  if(simpleData.type == FEC_RTVAL_SIMPLE_TYPE_SINT64)   return double(simpleData.value.sint64);

  return DBL_MAX;
}

inline void Mat44ToMMatrix_data(float const *data, MMatrix &matrix)
{
  double vals[4][4] ={
    { data[0], data[4], data[8],  data[12] },
    { data[1], data[5], data[9],  data[13] },
    { data[2], data[6], data[10], data[14] },
    { data[3], data[7], data[11], data[15] }
  };
  matrix = MMatrix(vals);
}

inline void Mat44ToMMatrix(FabricCore::RTVal &rtVal, MMatrix &matrix)
{
  FabricCore::RTVal dataRtVal = rtVal.callMethod("Data", "data", 0, 0);
  float * data = (float*)dataRtVal.getData();
  Mat44ToMMatrix_data(data, matrix);
}

inline void MMatrixToMat44_data(MMatrix const &matrix, float *data)
{
  data[0]  = (float)matrix[0][0];
  data[1]  = (float)matrix[1][0];
  data[2]  = (float)matrix[2][0];
  data[3]  = (float)matrix[3][0];
  data[4]  = (float)matrix[0][1];
  data[5]  = (float)matrix[1][1];
  data[6]  = (float)matrix[2][1];
  data[7]  = (float)matrix[3][1];
  data[8]  = (float)matrix[0][2];
  data[9]  = (float)matrix[1][2];
  data[10] = (float)matrix[2][2];
  data[11] = (float)matrix[3][2];
  data[12] = (float)matrix[0][3];
  data[13] = (float)matrix[1][3];
  data[14] = (float)matrix[2][3];
  data[15] = (float)matrix[3][3];
}

inline void MMatrixToMat44(MMatrix const &matrix, FabricCore::RTVal &rtVal)
{
  FabricCore::RTVal dataRtVal = rtVal.callMethod("Data", "data", 0, 0);
  float * data = (float*)dataRtVal.getData();
  MMatrixToMat44_data(matrix, data);
}

void dfgPlugToPort_compound_convertMat44(const MMatrix & matrix, FabricCore::RTVal & rtVal)
{
  CORE_CATCH_BEGIN;
  rtVal = FabricSplice::constructRTVal("Mat44", 0, 0);
  MMatrixToMat44(matrix, rtVal);
  CORE_CATCH_END;
}

void dfgPlugToPort_compound_convertCompound(MFnCompoundAttribute & compound, MDataHandle & handle, FabricCore::RTVal & rtVal)
{
  std::vector<FabricCore::RTVal> args(5);

  CORE_CATCH_BEGIN;

  // treat special cases
  if(compound.numChildren() == 3)
  {
    MString compoundName = compound.name();
    FabricCore::RTVal compoundNameRTVal = FabricSplice::constructStringRTVal(compoundName.asChar());

    MFnAttribute x(compound.child(0));
    MFnAttribute y(compound.child(1));
    MFnAttribute z(compound.child(2));

    if(x.name() == compoundName+"X" && y.name() == compoundName+"Y" && z.name() == compoundName+"Z")
    {
      MStatus attrStatus;
      MFnNumericAttribute nx(x.object(), &attrStatus);
      if(attrStatus == MS::kSuccess)
      {
        if(!compound.isArray())
        {
          MDataHandle xHandle(handle.child(x.object()));
          MDataHandle yHandle(handle.child(y.object()));
          MDataHandle zHandle(handle.child(z.object()));
          args[0] = FabricSplice::constructFloat32RTVal(xHandle.asDouble());
          args[1] = FabricSplice::constructFloat32RTVal(yHandle.asDouble());
          args[2] = FabricSplice::constructFloat32RTVal(zHandle.asDouble());
          FabricCore::RTVal value = FabricSplice::constructRTVal("Vec3", 3, &args[0]);
          args[0] = compoundNameRTVal;
          args[1] = value;
          rtVal = FabricSplice::constructObjectRTVal("Vec3Param", 2, &args[0]);
        }
        else
        {
          MArrayDataHandle arrayHandle(handle);

          rtVal = FabricSplice::constructObjectRTVal("Vec3ArrayParam", 1, &compoundNameRTVal);
          args[0] = FabricSplice::constructUInt32RTVal(arrayHandle.elementCount());
          rtVal.callMethod("", "resize", 1, &args[0]);

          for(unsigned int j=0;j<arrayHandle.elementCount();j++)
          {
            MDataHandle elementHandle = arrayHandle.inputValue();
            MDataHandle xHandle(elementHandle.child(x.object()));
            MDataHandle yHandle(elementHandle.child(y.object()));
            MDataHandle zHandle(elementHandle.child(z.object()));
            args[0] = FabricSplice::constructFloat32RTVal(xHandle.asDouble());
            args[1] = FabricSplice::constructFloat32RTVal(yHandle.asDouble());
            args[2] = FabricSplice::constructFloat32RTVal(zHandle.asDouble());
            FabricCore::RTVal value = FabricSplice::constructRTVal("Vec3", 3, &args[0]);

            args[0] = FabricSplice::constructUInt32RTVal(j);
            args[1] = value;
            rtVal.callMethod("", "setValue", 2, &args[0]);
            arrayHandle.next();
          }
        }

        return;
      }
      else
      {
        MFnUnitAttribute ux(x.object(), &attrStatus);
        if(attrStatus == MS::kSuccess)
        {
          if(!compound.isArray())
          {
            MDataHandle xHandle(handle.child(x.object()));
            MDataHandle yHandle(handle.child(y.object()));
            MDataHandle zHandle(handle.child(z.object()));
            args[0] = FabricSplice::constructFloat32RTVal(xHandle.asAngle().as(MAngle::kRadians));
            args[1] = FabricSplice::constructFloat32RTVal(yHandle.asAngle().as(MAngle::kRadians));
            args[2] = FabricSplice::constructFloat32RTVal(zHandle.asAngle().as(MAngle::kRadians));
            FabricCore::RTVal value = FabricSplice::constructRTVal("Euler", 3, &args[0]);
            args[0] = compoundNameRTVal;
            args[1] = value;
            rtVal = FabricSplice::constructObjectRTVal("EulerParam", 2, &args[0]);
          }
          else
          {
            MArrayDataHandle arrayHandle(handle);

            rtVal = FabricSplice::constructObjectRTVal("EulerArrayParam", 1, &compoundNameRTVal);
            args[0] = FabricSplice::constructUInt32RTVal(arrayHandle.elementCount());
            rtVal.callMethod("", "resize", 1, &args[0]);

            for(unsigned int j=0;j<arrayHandle.elementCount();j++)
            {
              MDataHandle elementHandle = arrayHandle.inputValue();
              MDataHandle xHandle(elementHandle.child(x.object()));
              MDataHandle yHandle(elementHandle.child(y.object()));
              MDataHandle zHandle(elementHandle.child(z.object()));
              args[0] = FabricSplice::constructFloat32RTVal(xHandle.asAngle().as(MAngle::kRadians));
              args[1] = FabricSplice::constructFloat32RTVal(yHandle.asAngle().as(MAngle::kRadians));
              args[2] = FabricSplice::constructFloat32RTVal(zHandle.asAngle().as(MAngle::kRadians));
              FabricCore::RTVal value = FabricSplice::constructRTVal("Euler", 3, &args[0]);

              args[0] = FabricSplice::constructUInt32RTVal(j);
              args[1] = value;
              rtVal.callMethod("", "setValue", 2, &args[0]);
              arrayHandle.next();
            }
          }

          return;
        }
      }
    }
  }

  for(unsigned int i=0;i<compound.numChildren();i++)
  {
    MFnAttribute child(compound.child(i));
    MString childName = child.name();
    FabricCore::RTVal childNameRTVal = FabricSplice::constructStringRTVal(childName.asChar());
    FabricCore::RTVal childRTVal;
    MStatus attrStatus;

    MFnNumericAttribute nAttr(child.object(), &attrStatus);
    if(attrStatus == MS::kSuccess)
    {
      if(nAttr.unitType() == MFnNumericData::kBoolean)
      {
        if(!nAttr.isArray())
        {
          MDataHandle childHandle(handle.child(child.object()));
          args[0] = childNameRTVal;
          args[1] = FabricSplice::constructBooleanRTVal(childHandle.asBool());
          childRTVal = FabricSplice::constructObjectRTVal("BooleanParam", 2, &args[0]);
        }
        else
        {
          MArrayDataHandle childHandle(handle.child(child.object()));
          childRTVal = FabricSplice::constructObjectRTVal("BooleanArrayParam", 1, &childNameRTVal);
          args[0] = FabricSplice::constructUInt32RTVal(childHandle.elementCount());
          childRTVal.callMethod("", "resize", 1, &args[0]);

          for(unsigned int j=0;j<childHandle.elementCount();j++)
          {
            args[0] = FabricSplice::constructUInt32RTVal(j);
            args[1] = FabricSplice::constructBooleanRTVal(childHandle.inputValue().asBool());
            childRTVal.callMethod("", "setValue", 2, &args[0]);
            childHandle.next();
          }
        }
      }
      else if(nAttr.unitType() == MFnNumericData::kInt)
      {
        if(!nAttr.isArray())
        {
          MDataHandle childHandle(handle.child(child.object()));
          args[0] = childNameRTVal;
          args[1] = FabricSplice::constructSInt32RTVal(childHandle.asInt());
          childRTVal = FabricSplice::constructObjectRTVal("SInt32Param", 2, &args[0]);
        }
        else
        {
          MArrayDataHandle childHandle(handle.child(child.object()));
          childRTVal = FabricSplice::constructObjectRTVal("SInt32ArrayParam", 1, &childNameRTVal);
          args[0] = FabricSplice::constructUInt32RTVal(childHandle.elementCount());
          childRTVal.callMethod("", "resize", 1, &args[0]);

          for(unsigned int j=0;j<childHandle.elementCount();j++)
          {
            args[0] = FabricSplice::constructUInt32RTVal(j);
            args[1] = FabricSplice::constructSInt32RTVal(childHandle.inputValue().asInt());
            childRTVal.callMethod("", "setValue", 2, &args[0]);
            childHandle.next();
          }
        }
      }
      else if(nAttr.unitType() == MFnNumericData::kFloat)
      {
        if(!nAttr.isArray())
        {
          MDataHandle childHandle(handle.child(child.object()));
          args[0] = childNameRTVal;
          args[1] = FabricSplice::constructFloat64RTVal(childHandle.asFloat());
          childRTVal = FabricSplice::constructObjectRTVal("Float64Param", 2, &args[0]);
        }
        else
        {
          MArrayDataHandle childHandle(handle.child(child.object()));
          childRTVal = FabricSplice::constructObjectRTVal("Float64ArrayParam", 1, &childNameRTVal);
          args[0] = FabricSplice::constructUInt32RTVal(childHandle.elementCount());
          childRTVal.callMethod("", "resize", 1, &args[0]);

          for(unsigned int j=0;j<childHandle.elementCount();j++)
          {
            args[0] = FabricSplice::constructUInt32RTVal(j);
            args[1] = FabricSplice::constructFloat64RTVal(childHandle.inputValue().asFloat());
            childRTVal.callMethod("", "setValue", 2, &args[0]);
            childHandle.next();
          }
        }
      }
      else if(nAttr.unitType() == MFnNumericData::kDouble)
      {
        if(!nAttr.isArray())
        {
          MDataHandle childHandle(handle.child(child.object()));
          args[0] = childNameRTVal;
          args[1] = FabricSplice::constructFloat64RTVal(childHandle.asDouble());
          childRTVal = FabricSplice::constructObjectRTVal("Float64Param", 2, &args[0]);
        }
        else
        {
          MArrayDataHandle childHandle(handle.child(child.object()));
          childRTVal = FabricSplice::constructObjectRTVal("Float64ArrayParam", 1, &childNameRTVal);
          args[0] = FabricSplice::constructUInt32RTVal(childHandle.elementCount());
          childRTVal.callMethod("", "resize", 1, &args[0]);

          for(unsigned int j=0;j<childHandle.elementCount();j++)
          {
            args[0] = FabricSplice::constructUInt32RTVal(j);
            args[1] = FabricSplice::constructFloat64RTVal(childHandle.inputValue().asDouble());
            childRTVal.callMethod("", "setValue", 2, &args[0]);
            childHandle.next();
          }
        }
      }
      else if(nAttr.unitType() == MFnNumericData::k3Double) // vec3
      {
        if(!nAttr.isArray())
        {
          MDataHandle childHandle(handle.child(child.object()));
          args[0] = childNameRTVal;
          args[1] = FabricSplice::constructRTVal("Vec3", 0, 0);
          MFloatVector v = childHandle.asFloatVector();
          args[1].setMember("x", FabricSplice::constructFloat64RTVal(v.x));
          args[1].setMember("y", FabricSplice::constructFloat64RTVal(v.y));
          args[1].setMember("z", FabricSplice::constructFloat64RTVal(v.z));
          childRTVal = FabricSplice::constructObjectRTVal("Vec3Param", 2, &args[0]);
        }
        else
        {
          MArrayDataHandle childHandle(handle.child(child.object()));
          childRTVal = FabricSplice::constructObjectRTVal("Vec3ArrayParam", 1, &childNameRTVal);
          args[0] = FabricSplice::constructUInt32RTVal(childHandle.elementCount());
          childRTVal.callMethod("", "resize", 1, &args[0]);

          for(unsigned int j=0;j<childHandle.elementCount();j++)
          {
            args[0] = FabricSplice::constructUInt32RTVal(j);
            args[1] = FabricSplice::constructRTVal("Vec3", 0, 0);
            MFloatVector v = childHandle.inputValue().asFloatVector();
            args[1].setMember("x", FabricSplice::constructFloat64RTVal(v.x));
            args[1].setMember("y", FabricSplice::constructFloat64RTVal(v.y));
            args[1].setMember("z", FabricSplice::constructFloat64RTVal(v.z));
            childRTVal.callMethod("", "setValue", 2, &args[0]);
            childHandle.next();
          }
        }
      }
      else if(nAttr.unitType() == MFnNumericData::k3Float) // color
      {
        if(!nAttr.isArray())
        {
          MDataHandle childHandle(handle.child(child.object()));
          args[0] = childNameRTVal;
          args[1] = FabricSplice::constructRTVal("Color", 0, 0);
          MFloatVector v = childHandle.asFloatVector();
          args[1].setMember("r", FabricSplice::constructFloat64RTVal(v.x));
          args[1].setMember("g", FabricSplice::constructFloat64RTVal(v.y));
          args[1].setMember("b", FabricSplice::constructFloat64RTVal(v.z));
          args[1].setMember("a", FabricSplice::constructFloat64RTVal(1.0));
          childRTVal = FabricSplice::constructObjectRTVal("ColorParam", 2, &args[0]);
        }
        else
        {
          MArrayDataHandle childHandle(handle.child(child.object()));
          childRTVal = FabricSplice::constructObjectRTVal("ColorArrayParam", 1, &childNameRTVal);
          args[0] = FabricSplice::constructUInt32RTVal(childHandle.elementCount());
          childRTVal.callMethod("", "resize", 1, &args[0]);

          for(unsigned int j=0;j<childHandle.elementCount();j++)
          {
            args[0] = FabricSplice::constructUInt32RTVal(j);
            args[1] = FabricSplice::constructRTVal("Color", 0, 0);
            MFloatVector v = childHandle.inputValue().asFloatVector();
            args[1].setMember("r", FabricSplice::constructFloat64RTVal(v.x));
            args[1].setMember("g", FabricSplice::constructFloat64RTVal(v.y));
            args[1].setMember("b", FabricSplice::constructFloat64RTVal(v.z));
            args[1].setMember("a", FabricSplice::constructFloat64RTVal(1.0));
            childRTVal.callMethod("", "setValue", 2, &args[0]);
            childHandle.next();
          }
        }
      }
      else
      {
        mayaLogErrorFunc("Unsupported numeric attribute '"+childName+"'.");
        return;
      }
    }
    else
    {
      MFnTypedAttribute tAttr(child.object(), &attrStatus);
      if(attrStatus == MS::kSuccess)
      {
        if(tAttr.attrType() == MFnData::kString)
        {
          if(!tAttr.isArray())
          {
            MDataHandle childHandle(handle.child(child.object()));
            args[0] = childNameRTVal;
            args[1] = FabricSplice::constructStringRTVal(childHandle.asString().asChar());
            childRTVal = FabricSplice::constructObjectRTVal("StringParam", 2, &args[0]);
          }
          else
          {
            MArrayDataHandle childHandle(handle.child(child.object()));
            childRTVal = FabricSplice::constructObjectRTVal("StringArrayParam", 1, &childNameRTVal);
            args[0] = FabricSplice::constructUInt32RTVal(childHandle.elementCount());
            childRTVal.callMethod("", "resize", 1, &args[0]);

            for(unsigned int j=0;j<childHandle.elementCount();j++)
            {
              args[0] = FabricSplice::constructUInt32RTVal(j);
              args[1] = FabricSplice::constructStringRTVal(childHandle.inputValue().asString().asChar());
              childRTVal.callMethod("", "setValue", 2, &args[0]);
              childHandle.next();
            }
          }
        }
        else if(tAttr.attrType() == MFnData::kIntArray)
        {
          if(!tAttr.isArray())
          {
            args[0] = childNameRTVal;
            childRTVal = FabricSplice::constructObjectRTVal("SInt32ArrayParam", 1, &args[0]);

            MIntArray arrayValues = MFnIntArrayData(handle.data()).array();
            unsigned int numArrayValues = arrayValues.length();
            args[0] = FabricSplice::constructUInt32RTVal(numArrayValues);
            childRTVal.callMethod("", "resize", 1, &args[0]);

            FabricCore::RTVal valuesRTVal = childRTVal.maybeGetMember("values");
            FabricCore::RTVal dataRtVal = valuesRTVal.callMethod("Data", "data", 0, 0);
            void * data = dataRtVal.getData();
            memcpy(data, &arrayValues[0], sizeof(int32_t) * numArrayValues);
          }
          else
          {
            mayaLogErrorFunc("Arrays of MFnData::kIntArray are not supported for '"+childName+"'.");
          }
        }
        else if(tAttr.attrType() == MFnData::kDoubleArray)
        {
          if(!tAttr.isArray())
          {
            args[0] = childNameRTVal;
            childRTVal = FabricSplice::constructObjectRTVal("Float64ArrayParam", 1, &args[0]);

            MDoubleArray arrayValues = MFnDoubleArrayData(handle.data()).array();
            unsigned int numArrayValues = arrayValues.length();
            args[0] = FabricSplice::constructUInt32RTVal(numArrayValues);
            childRTVal.callMethod("", "resize", 1, &args[0]);

            FabricCore::RTVal valuesRTVal = childRTVal.maybeGetMember("values");
            FabricCore::RTVal dataRtVal = valuesRTVal.callMethod("Data", "data", 0, 0);
            void * data = dataRtVal.getData();
            memcpy(data, &arrayValues[0], sizeof(double) * numArrayValues);
          }
          else
          {
            mayaLogErrorFunc("Arrays of MFnData::kDoubleArray are not supported for '"+childName+"'.");
          }
        }
        else if(tAttr.attrType() == MFnData::kVectorArray)
        {
          if(!tAttr.isArray())
          {
            args[0] = childNameRTVal;
            childRTVal = FabricSplice::constructObjectRTVal("Vec3ArrayParam", 1, &args[0]);

            MVectorArray arrayValues = MFnVectorArrayData(handle.data()).array();
            unsigned int numArrayValues = arrayValues.length();
            args[0] = FabricSplice::constructUInt32RTVal(numArrayValues);
            childRTVal.callMethod("", "resize", 1, &args[0]);

            FabricCore::RTVal valuesRTVal = childRTVal.maybeGetMember("values");
            FabricCore::RTVal dataRtVal = valuesRTVal.callMethod("Data", "data", 0, 0);
            float * data = (float*)dataRtVal.getData();
            arrayValues.get((floatVec*)data);
          }
          else
          {
            mayaLogErrorFunc("Arrays of MFnData::kVectorArray are not supported for '"+childName+"'.");
          }
        }
        else
        {
          mayaLogErrorFunc("Unsupported typed attribute '"+childName+"'.");
          return;
        }
      }
      else
      {
        MFnMatrixAttribute mAttr(child.object(), &attrStatus);
        if(attrStatus == MS::kSuccess)
        {
          if(!mAttr.isArray())
          {
            MDataHandle childHandle(handle.child(child.object()));

            childRTVal = FabricSplice::constructObjectRTVal("Mat44Param", 1, &childNameRTVal);
            FabricCore::RTVal matrixRTVal;
            dfgPlugToPort_compound_convertMat44(childHandle.asMatrix(), matrixRTVal);
            childRTVal.callMethod("", "setValue", 1, &matrixRTVal);
          }
          else
          {
            MArrayDataHandle childHandle(handle.child(child.object()));
            childRTVal = FabricSplice::constructObjectRTVal("Mat44ArrayParam", 1, &childNameRTVal);
            args[0] = FabricSplice::constructUInt32RTVal(childHandle.elementCount());
            childRTVal.callMethod("", "resize", 1, &args[0]);

            for(unsigned int j=0;j<childHandle.elementCount();j++)
            {
              args[0] = FabricSplice::constructUInt32RTVal(j);
              dfgPlugToPort_compound_convertMat44(childHandle.inputValue().asMatrix(), args[1]);
              childRTVal.callMethod("", "setValue", 2, &args[0]);
              childHandle.next();
            }
          }
        }
        else
        {
          MFnCompoundAttribute cAttr(child.object(), &attrStatus);
          if(attrStatus == MS::kSuccess)
          {
            MFnCompoundAttribute cAttr(compound.child(i));
            if(!cAttr.isArray())
            {
              MDataHandle childHandle(handle.child(child.object()));
              childRTVal = FabricSplice::constructObjectRTVal("CompoundParam", 1, &childNameRTVal);
              dfgPlugToPort_compound_convertCompound(cAttr, childHandle, childRTVal);
            }
          }
        }
      }
    }

    if(childRTVal.isValid())
      rtVal.callMethod("", "addParam", 1, &childRTVal);
  }

  CORE_CATCH_END;
}

void dfgPlugToPort_compoundArray(
  unsigned argIndex,
  char const *argName,
  char const *argTypeName,
  FEC_DFGPortType argOutsidePortType,
  uint64_t argRawDataSize,
  FEC_DFGBindingVisitArgs_GetCB getCB,
  FEC_DFGBindingVisitArgs_GetRawCB getRawCB,
  FEC_DFGBindingVisitArgs_SetCB setCB,
  FEC_DFGBindingVisitArgs_SetRawCB setRawCB,
  void *getSetUD,
  MPlug &plug, 
  MDataBlock &data)
{
  FabricMayaProfilingEvent bracket("dfgPlugToPort_compoundArray");

  CORE_CATCH_BEGIN;

  if(plug.isArray()){
    FabricCore::RTVal compoundVals = FabricSplice::constructObjectRTVal("CompoundArrayParam");
    FabricCore::RTVal numElements = FabricSplice::constructUInt32RTVal(plug.numElements());
    compoundVals.callMethod("", "resize", 1, &numElements);

    for(unsigned int j=0;j<plug.numElements();j++) {

      MPlug element = plug.elementByPhysicalIndex(j);
      FTL::AutoProfilingPauseEvent pauseBracket(bracket);
      MDataHandle handle = data.inputValue(element);
      pauseBracket.resume();
      MFnCompoundAttribute compound(element.attribute());

      FabricCore::RTVal compoundVal = FabricSplice::constructObjectRTVal("CompoundParam");
      dfgPlugToPort_compound_convertCompound(compound, handle, compoundVal);
      compoundVals.callMethod("", "addParam", 1, &compoundVal);
    }
    setCB(getSetUD, compoundVals.getFECRTValRef());
  }
  else{
  }

  CORE_CATCH_END;
}

void dfgPlugToPort_compound(
  unsigned argIndex,
  char const *argName,
  char const *argTypeName,
  FEC_DFGPortType argOutsidePortType,
  uint64_t argRawDataSize,
  FEC_DFGBindingVisitArgs_GetCB getCB,
  FEC_DFGBindingVisitArgs_GetRawCB getRawCB,
  FEC_DFGBindingVisitArgs_SetCB setCB,
  FEC_DFGBindingVisitArgs_SetRawCB setRawCB,
  void *getSetUD,
  MPlug &plug, 
  MDataBlock &data)
{
  FabricMayaProfilingEvent bracket("dfgPlugToPort_compound");

  if(plug.isArray()){
    FabricCore::RTVal compoundVals = FabricSplice::constructObjectRTVal("CompoundParam[]");
    compoundVals.setArraySize(plug.numElements());

    for(unsigned int j=0;j<plug.numElements();j++) {

      MPlug element = plug.elementByPhysicalIndex(j);
      FTL::AutoProfilingPauseEvent pauseBracket(bracket);
      MDataHandle handle = data.inputValue(element);
      pauseBracket.resume();
      MFnCompoundAttribute compound(element.attribute());

      FabricCore::RTVal compoundVal;
      dfgPlugToPort_compound_convertCompound(compound, handle, compoundVal);
      compoundVals.setArrayElement(j, compoundVal);
    }
    setCB(getSetUD, compoundVals.getFECRTValRef());
  }
  else{
    FTL::AutoProfilingPauseEvent pauseBracket(bracket);
    MDataHandle handle = data.inputValue(plug);
    pauseBracket.resume();
    FabricCore::RTVal rtVal = FabricSplice::constructObjectRTVal("CompoundParam");
    MFnCompoundAttribute compound(plug.attribute());
    dfgPlugToPort_compound_convertCompound(compound, handle, rtVal);
    setCB(getSetUD, rtVal.getFECRTValRef());
  }
}

void dfgPlugToPort_bool(
  unsigned argIndex,
  char const *argName,
  char const *argTypeName,
  FEC_DFGPortType argOutsidePortType,
  uint64_t argRawDataSize,
  FEC_DFGBindingVisitArgs_GetCB getCB,
  FEC_DFGBindingVisitArgs_GetRawCB getRawCB,
  FEC_DFGBindingVisitArgs_SetCB setCB,
  FEC_DFGBindingVisitArgs_SetRawCB setRawCB,
  void *getSetUD,
  MPlug &plug, 
  MDataBlock &data)
{
  FabricMayaProfilingEvent bracket("dfgPlugToPort_bool");

  uint64_t elementDataSize = sizeof(bool);
  uint64_t currentNumElements = argRawDataSize / elementDataSize;

  if(plug.isArray()){
    
    FTL::AutoProfilingPauseEvent pauseBracket(bracket);
    MArrayDataHandle arrayHandle = data.inputArrayValue(plug);
    pauseBracket.resume();

    unsigned int numElements = arrayHandle.elementCount();
    std::vector<bool> values(numElements);

    for(unsigned int i = 0; i < numElements; ++i){
      arrayHandle.jumpToArrayElement(i);
      MDataHandle handle = arrayHandle.inputValue();
      values[i] = handle.asBool();
    }

    setRawCB(getSetUD, &values[0], elementDataSize * numElements);
  }
  else{
    FTL::AutoProfilingPauseEvent pauseBracket(bracket);
    MDataHandle handle = data.inputValue(plug);
    pauseBracket.resume();
    setCB(getSetUD, FabricSplice::constructBooleanRTVal(handle.asBool()).getFECRTValRef());
  }
}

void dfgPlugToPort_integer(
  unsigned argIndex,
  char const *argName,
  char const *argTypeName,
  FEC_DFGPortType argOutsidePortType,
  uint64_t argRawDataSize,
  FEC_DFGBindingVisitArgs_GetCB getCB,
  FEC_DFGBindingVisitArgs_GetRawCB getRawCB,
  FEC_DFGBindingVisitArgs_SetCB setCB,
  FEC_DFGBindingVisitArgs_SetRawCB setRawCB,
  void *getSetUD,
  MPlug &plug, 
  MDataBlock &data)
{
  FabricMayaProfilingEvent bracket("dfgPlugToPort_integer");

  uint64_t elementDataSize = sizeof(int32_t);
  uint64_t currentNumElements = argRawDataSize / elementDataSize;

  if(plug.isArray()){
    FTL::AutoProfilingPauseEvent pauseBracket(bracket);
    MArrayDataHandle arrayHandle = data.inputArrayValue(plug);
    pauseBracket.resume();

    unsigned int numElements = arrayHandle.elementCount();
    std::vector<int32_t> buffer(numElements);
    int32_t * values = &buffer[0];

    for(unsigned int i = 0; i < numElements; ++i){
      arrayHandle.jumpToArrayElement(i);
      MDataHandle handle = arrayHandle.inputValue();
      values[i] = (int32_t)handle.asLong();
    }

    setRawCB(getSetUD, values, elementDataSize * numElements);
  }else{
    FTL::AutoProfilingPauseEvent pauseBracket(bracket);
    MDataHandle handle = data.inputValue(plug);
    pauseBracket.resume();
    // bool isNativeArray = FTL::CStrRef(binding.getExec().getExecPortMetadata(argName, "nativeArray")) == "true";
    if(handle.type() == MFnData::kIntArray) { // || isNativeArray) {
      MIntArray arrayValues = MFnIntArrayData(handle.data()).array();

      unsigned int numElements = arrayValues.length();
      std::vector<int32_t> buffer(numElements);
      int32_t * values = &buffer[0];

      for(unsigned int i = 0; i < numElements; ++i) {
        values[i] = (int32_t)arrayValues[i];
      }

      setRawCB(getSetUD, values, elementDataSize * numElements);
    }else{
      setCB(getSetUD, FabricSplice::constructSInt32RTVal(handle.asLong()).getFECRTValRef());
    }
  }
}

void dfgPlugToPort_scalar(
  unsigned argIndex,
  char const *argName,
  char const *argTypeName,
  FEC_DFGPortType argOutsidePortType,
  uint64_t argRawDataSize,
  FEC_DFGBindingVisitArgs_GetCB getCB,
  FEC_DFGBindingVisitArgs_GetRawCB getRawCB,
  FEC_DFGBindingVisitArgs_SetCB setCB,
  FEC_DFGBindingVisitArgs_SetRawCB setRawCB,
  void *getSetUD,
  MPlug &plug, 
  MDataBlock &data)
{
  FabricMayaProfilingEvent bracket("dfgPlugToPort_scalar");

  uint64_t elementDataSize = sizeof(float);
  uint64_t currentNumElements = argRawDataSize / elementDataSize;

  // FTL::CStrRef scalarUnit = binding.getExec().getExecPortMetadata(argName, "scalarUnit");
  if(plug.isArray()){
    FTL::AutoProfilingPauseEvent pauseBracket(bracket);
    MArrayDataHandle arrayHandle = data.inputArrayValue(plug);
    pauseBracket.resume();

    unsigned int numElements = arrayHandle.elementCount();

    std::vector<float> buffer(numElements);
    float * values = &buffer[0];

    for(unsigned int i = 0; i < numElements; ++i){
      arrayHandle.jumpToArrayElement(i);
      MDataHandle handle = arrayHandle.inputValue();

      // todo: renable units
      /*
      if(scalarUnit == "time")
        values[i] = handle.asTime().as(MTime::kSeconds);
      else if(scalarUnit == "angle")
        values[i] = handle.asAngle().as(MAngle::kRadians);
      else if(scalarUnit == "distance")
        values[i] = handle.asDistance().as(MDistance::kMillimeters);
      else */
      {
        if(handle.numericType() == MFnNumericData::kFloat){
          values[i] = handle.asFloat();
        }else{
          values[i] = (float)handle.asDouble();
        }
      }
    }
    setRawCB(getSetUD, values, elementDataSize * numElements);

  }else{
    FTL::AutoProfilingPauseEvent pauseBracket(bracket);
    MDataHandle handle = data.inputValue(plug);
    pauseBracket.resume();
    if(currentNumElements > 1){
      MDoubleArray arrayValues = MFnDoubleArrayData(handle.data()).array();
      unsigned int numElements = arrayValues.length();
  
      std::vector<float> buffer(numElements);
      float * values = &buffer[0];

      for(unsigned int i = 0; i < numElements; ++i){
        values[i] = arrayValues[i];
      }

      setRawCB(getSetUD, values, elementDataSize * numElements);
    }
    else
    {
      // get the value of the source Maya plug.
      double plugValue = 0;
      /*if      (scalarUnit == "time")      plugValue = handle.asTime().as(MTime::kSeconds);
      else if (scalarUnit == "angle")     plugValue = handle.asAngle().as(MAngle::kRadians);
      else if (scalarUnit == "distance")  plugValue = handle.asDistance().as(MDistance::kMillimeters);
      else */
      {
        if(handle.numericType() == MFnNumericData::kFloat)  plugValue = handle.asFloat();
        else                                                plugValue = handle.asDouble();
      }

      // get the resolved data type of the destination exec port and set its value.
      FTL::CStrRef resolvedType = argTypeName;
      if      (resolvedType == FTL_STR("Scalar"))  setCB(getSetUD, FabricSplice::constructFloat32RTVal((float)plugValue).getFECRTValRef());
      else if (resolvedType == FTL_STR("Float32")) setCB(getSetUD, FabricSplice::constructFloat32RTVal((float)plugValue).getFECRTValRef());
      else if (resolvedType == FTL_STR("Float64")) setCB(getSetUD, FabricSplice::constructFloat64RTVal(       plugValue).getFECRTValRef());
    }
  }
}

void dfgPlugToPort_string(
  unsigned argIndex,
  char const *argName,
  char const *argTypeName,
  FEC_DFGPortType argOutsidePortType,
  uint64_t argRawDataSize,
  FEC_DFGBindingVisitArgs_GetCB getCB,
  FEC_DFGBindingVisitArgs_GetRawCB getRawCB,
  FEC_DFGBindingVisitArgs_SetCB setCB,
  FEC_DFGBindingVisitArgs_SetRawCB setRawCB,
  void *getSetUD,
  MPlug &plug, 
  MDataBlock &data)
{
  FabricMayaProfilingEvent bracket("dfgPlugToPort_string");

  CORE_CATCH_BEGIN;

  if(plug.isArray()){
    FTL::AutoProfilingPauseEvent pauseBracket(bracket);
    MArrayDataHandle arrayHandle = data.inputArrayValue(plug);
    pauseBracket.resume();
    unsigned int elements = arrayHandle.elementCount();
    FabricCore::RTVal stringArrayVal = FabricSplice::constructVariableArrayRTVal("String");
    FabricCore::RTVal elementsVal = FabricSplice::constructUInt32RTVal(elements);
    stringArrayVal.callMethod("", "resize", 1, &elementsVal);
    for(unsigned int i = 0; i < elements; ++i){
      arrayHandle.jumpToArrayElement(i);
      MDataHandle handle = arrayHandle.inputValue();
      stringArrayVal.setArrayElement(i, FabricSplice::constructStringRTVal(handle.asString().asChar()));
    }

    setCB(getSetUD, stringArrayVal.getFECRTValRef());
  }
  else{
    FTL::AutoProfilingPauseEvent pauseBracket(bracket);
    MStatus mStatus;
    MDataHandle handle = data.inputValue( plug, &mStatus );
    pauseBracket.resume();
    if ( mStatus != MS::kSuccess
      || handle.type() != MFnData::kString )
    {
      std::string error;
      error += "Unexpected failure transferring String value of port '";
      error += argName;
      error += "'";
      mayaLogErrorFunc( error.c_str() );
      return;
    }
    MString valueMString = handle.asString();
    char const *valueCStr = valueMString.asChar();
    setCB(getSetUD, FabricSplice::constructStringRTVal( valueCStr ).getFECRTValRef());
  }

  CORE_CATCH_END;
}

void dfgPlugToPort_color(
  unsigned argIndex,
  char const *argName,
  char const *argTypeName,
  FEC_DFGPortType argOutsidePortType,
  uint64_t argRawDataSize,
  FEC_DFGBindingVisitArgs_GetCB getCB,
  FEC_DFGBindingVisitArgs_GetRawCB getRawCB,
  FEC_DFGBindingVisitArgs_SetCB setCB,
  FEC_DFGBindingVisitArgs_SetRawCB setRawCB,
  void *getSetUD,
  MPlug &plug, 
  MDataBlock &data)
{
  FabricMayaProfilingEvent bracket("dfgPlugToPort_color");

  uint64_t elementDataSize = sizeof(float) * 4;

  CORE_CATCH_BEGIN;

  if(plug.isArray()){
    FTL::AutoProfilingPauseEvent pauseBracket(bracket);
    MArrayDataHandle arrayHandle = data.inputArrayValue(plug);
    pauseBracket.resume();
    unsigned int numElements = arrayHandle.elementCount();

    std::vector<float> buffer(numElements * 4);
    float * values = &buffer[0];

    unsigned int offset = 0;
    for(unsigned int i = 0; i < numElements; ++i){
      arrayHandle.jumpToArrayElement(i);
      MDataHandle handle = arrayHandle.inputValue();

      if(handle.numericType() == MFnNumericData::k3Float || handle.numericType() == MFnNumericData::kFloat){
        MFloatVector v = handle.asFloatVector();
        values[offset++] = v.x;
        values[offset++] = v.y;
        values[offset++] = v.z;
      } else{
        MVector v = handle.asVector();
        values[offset++] = (float)v.x;
        values[offset++] = (float)v.y;
        values[offset++] = (float)v.z;
      }
      values[offset++] = 1.0f;
    }

    setRawCB(getSetUD, values, elementDataSize * numElements);
  }
  else {
    FTL::AutoProfilingPauseEvent pauseBracket(bracket);
    MDataHandle handle = data.inputValue(plug);
    pauseBracket.resume();

    float values[4];
    if(handle.numericType() == MFnNumericData::k3Float || handle.numericType() == MFnNumericData::kFloat){
      MFloatVector v = handle.asFloatVector();
      values[0] = (float)v.x;
      values[1] = (float)v.y;
      values[2] = (float)v.z;
    } else{
      MVector v = handle.asVector();
      values[0] = (float)v.x;
      values[1] = (float)v.y;
      values[2] = (float)v.z;
    }
    values[3] = 1.0f;

    setRawCB(getSetUD, values, elementDataSize);
  }

  CORE_CATCH_END;
}

void dfgPlugToPort_vec3(
  unsigned argIndex,
  char const *argName,
  char const *argTypeName,
  FEC_DFGPortType argOutsidePortType,
  uint64_t argRawDataSize,
  FEC_DFGBindingVisitArgs_GetCB getCB,
  FEC_DFGBindingVisitArgs_GetRawCB getRawCB,
  FEC_DFGBindingVisitArgs_SetCB setCB,
  FEC_DFGBindingVisitArgs_SetRawCB setRawCB,
  void *getSetUD,
  MPlug &plug, 
  MDataBlock &data)
{
  FabricMayaProfilingEvent bracket("dfgPlugToPort_vec3");

  uint64_t elementDataSize = sizeof(float) * 3;

  if(plug.isArray()){
    FTL::AutoProfilingPauseEvent pauseBracket(bracket);
    MArrayDataHandle arrayHandle = data.inputArrayValue(plug);
    pauseBracket.resume();

    unsigned int numElements = arrayHandle.elementCount();

    std::vector<float> buffer(numElements * 3);
    float * values = &buffer[0];

    unsigned int offset = 0;
    for(unsigned int i = 0; i < numElements; ++i){
      arrayHandle.jumpToArrayElement(i);
      MDataHandle handle = arrayHandle.inputValue();
      if(handle.numericType() == MFnNumericData::k3Float || handle.numericType() == MFnNumericData::kFloat){
        const float3& mayaVec = handle.asFloat3();
        values[offset++] = (float)mayaVec[0];
        values[offset++] = (float)mayaVec[1];
        values[offset++] = (float)mayaVec[2];
      } else {
        const double3& mayaVec = handle.asDouble3();
        values[offset++] = (float)mayaVec[0];
        values[offset++] = (float)mayaVec[1];
        values[offset++] = (float)mayaVec[2];
      }
    }

    setRawCB(getSetUD, values, elementDataSize * numElements);
  }else{
    FTL::AutoProfilingPauseEvent pauseBracket(bracket);
    MDataHandle handle = data.inputValue(plug);
    pauseBracket.resume();
    // todo: reenable the native array support
    // bool isNativeArray = FTL::CStrRef(binding.getExec().getExecPortMetadata(argName, "nativeArray")) == "true";
    if(handle.type() == MFnData::kVectorArray) { // || isNativeArray){
      MVectorArray arrayValues = MFnVectorArrayData(handle.data()).array();
      unsigned int numElements = arrayValues.length();

      std::vector<float> buffer(numElements * 3);
      float * values = &buffer[0];

      size_t offset = 0;
      for(unsigned int i = 0; i < numElements; ++i){
        values[offset++] = (float)arrayValues[i].x;
        values[offset++] = (float)arrayValues[i].y;
        values[offset++] = (float)arrayValues[i].z;
      }

      setRawCB(getSetUD, values, elementDataSize * numElements);
    }else if(handle.type() == MFnData::kPointArray){
      MPointArray arrayValues = MFnPointArrayData(handle.data()).array();
      unsigned int numElements = arrayValues.length();

      std::vector<float> buffer(numElements * 3);
      float * values = &buffer[0];

      size_t offset = 0;
      for(unsigned int i = 0; i < numElements; ++i){
        values[offset++] = (float)arrayValues[i].x;
        values[offset++] = (float)arrayValues[i].y;
        values[offset++] = (float)arrayValues[i].z;
      }

      setRawCB(getSetUD, values, elementDataSize * numElements);
    }else{
      float values[3];
      if(handle.numericType() == MFnNumericData::k3Float || handle.numericType() == MFnNumericData::kFloat){
        const float3& mayaVec = handle.asFloat3();
        values[0] = mayaVec[0];
        values[1] = mayaVec[1];
        values[2] = mayaVec[2];
      } else{
        const double3& mayaVec = handle.asDouble3();
        values[0] = (float)mayaVec[0];
        values[1] = (float)mayaVec[1];
        values[2] = (float)mayaVec[2];
      }
      setRawCB(getSetUD, values, elementDataSize);
    }
  }
}

void dfgPlugToPort_euler(
  unsigned argIndex,
  char const *argName,
  char const *argTypeName,
  FEC_DFGPortType argOutsidePortType,
  uint64_t argRawDataSize,
  FEC_DFGBindingVisitArgs_GetCB getCB,
  FEC_DFGBindingVisitArgs_GetRawCB getRawCB,
  FEC_DFGBindingVisitArgs_SetCB setCB,
  FEC_DFGBindingVisitArgs_SetRawCB setRawCB,
  void *getSetUD,
  MPlug &plug, 
  MDataBlock &data)
{
  FabricMayaProfilingEvent bracket("dfgPlugToPort_euler");

  CORE_CATCH_BEGIN;

  if(plug.isArray()){
    FTL::AutoProfilingPauseEvent pauseBracket(bracket);
    MArrayDataHandle arrayHandle = data.inputArrayValue(plug);
    pauseBracket.resume();

    unsigned int elements = arrayHandle.elementCount();
    FabricCore::RTVal arrayVal = FabricSplice::constructVariableArrayRTVal("Euler");
    FabricCore::RTVal euler = FabricSplice::constructRTVal("Euler");
    FabricCore::RTVal arraySizeVal = FabricSplice::constructUInt32RTVal(elements);
    arrayVal.callMethod("", "resize", 1, &arraySizeVal);
    for(unsigned int i = 0; i < elements; ++i){
      arrayHandle.jumpToArrayElement(i);
      MDataHandle handle = arrayHandle.inputValue();

      if(handle.numericType() == MFnNumericData::k3Float || handle.numericType() == MFnNumericData::kFloat){
        const float3& mayaVec = handle.asFloat3();
        euler.setMember("x", FabricSplice::constructFloat64RTVal(mayaVec[0]));
        euler.setMember("y", FabricSplice::constructFloat64RTVal(mayaVec[1]));
        euler.setMember("z", FabricSplice::constructFloat64RTVal(mayaVec[2]));
      } else{
        const double3& mayaVec = handle.asDouble3();
        euler.setMember("x", FabricSplice::constructFloat64RTVal(mayaVec[0]));
        euler.setMember("y", FabricSplice::constructFloat64RTVal(mayaVec[1]));
        euler.setMember("z", FabricSplice::constructFloat64RTVal(mayaVec[2]));
      }
      arrayVal.setArrayElement(i, euler);
    }

    setCB(getSetUD, arrayVal.getFECRTValRef());
  }
  else {
    FabricCore::RTVal rtVal = getCB(getSetUD);
    if(rtVal.isArray())
      return;
    FTL::AutoProfilingPauseEvent pauseBracket(bracket);
    MDataHandle handle = data.inputValue(plug);
    pauseBracket.resume();

    FabricCore::RTVal euler = FabricSplice::constructRTVal("Euler");
    if(handle.numericType() == MFnNumericData::k3Float || handle.numericType() == MFnNumericData::kFloat){
      const float3& mayaVec = handle.asFloat3();
      euler.setMember("x", FabricSplice::constructFloat64RTVal(mayaVec[0]));
      euler.setMember("y", FabricSplice::constructFloat64RTVal(mayaVec[1]));
      euler.setMember("z", FabricSplice::constructFloat64RTVal(mayaVec[2]));
    } else{
      const double3& mayaVec = handle.asDouble3();
      euler.setMember("x", FabricSplice::constructFloat64RTVal(mayaVec[0]));
      euler.setMember("y", FabricSplice::constructFloat64RTVal(mayaVec[1]));
      euler.setMember("z", FabricSplice::constructFloat64RTVal(mayaVec[2]));
    }

    setCB(getSetUD, euler.getFECRTValRef());
  }

  CORE_CATCH_END;
}

void dfgPlugToPort_mat44(
  unsigned argIndex,
  char const *argName,
  char const *argTypeName,
  FEC_DFGPortType argOutsidePortType,
  uint64_t argRawDataSize,
  FEC_DFGBindingVisitArgs_GetCB getCB,
  FEC_DFGBindingVisitArgs_GetRawCB getRawCB,
  FEC_DFGBindingVisitArgs_SetCB setCB,
  FEC_DFGBindingVisitArgs_SetRawCB setRawCB,
  void *getSetUD,
  MPlug &plug, 
  MDataBlock &data)
{
  FabricMayaProfilingEvent bracket("dfgPlugToPort_mat44");

  uint64_t elementDataSize = sizeof(float) * 16;
  uint64_t offset = 0;

  if(plug.isArray()){
    FTL::AutoProfilingPauseEvent pauseBracket(bracket);
    MArrayDataHandle arrayHandle = data.inputArrayValue(plug);
    pauseBracket.resume();

    unsigned int numElements = arrayHandle.elementCount();
    std::vector<float> buffer(numElements * 16);
    float * values = &buffer[0];

    {
      FabricMayaProfilingEvent bracket("converting matrix array");
      for(unsigned int i = 0; i < numElements; ++i){
        arrayHandle.jumpToArrayElement(i);
        MDataHandle handle = arrayHandle.inputValue();
        const MMatrix& mayaMat = handle.asMatrix();
        MMatrixToMat44_data(mayaMat, &values[offset]);
        offset += 16;
      }
    }

    {
      FabricMayaProfilingEvent bracket("setRawCB");
      setRawCB(getSetUD, values, elementDataSize * numElements);
    }
  }
  else{
    FTL::AutoProfilingPauseEvent pauseBracket(bracket);
    MDataHandle handle = data.inputValue(plug);
    pauseBracket.resume();

    float values[16];

    {
      FabricMayaProfilingEvent bracket("converting matrix");
      const MMatrix& mayaMat = handle.asMatrix();
      MMatrixToMat44_data(mayaMat, values);
    }
    {
      FabricMayaProfilingEvent bracket("setRawCB");
      setRawCB(getSetUD, values, elementDataSize);
    }
  }
}

void dfgPlugToPort_PolygonMesh(
  unsigned argIndex,
  char const *argName,
  char const *argTypeName,
  FEC_DFGPortType argOutsidePortType,
  uint64_t argRawDataSize,
  FEC_DFGBindingVisitArgs_GetCB getCB,
  FEC_DFGBindingVisitArgs_GetRawCB getRawCB,
  FEC_DFGBindingVisitArgs_SetCB setCB,
  FEC_DFGBindingVisitArgs_SetRawCB setRawCB,
  void *getSetUD,
  MPlug &plug, 
  MDataBlock &data)
{
  FabricMayaProfilingEvent bracket("dfgPlugToPort_PolygonMesh");

  std::vector<MDataHandle> handles;
  std::vector<FabricCore::RTVal> rtVals;
  FabricCore::RTVal portRTVal;

  try
  {
    if(plug.isArray())
    {
      portRTVal = getCB(getSetUD);

      FTL::AutoProfilingPauseEvent pauseBracket(bracket);
      MArrayDataHandle arrayHandle = data.inputArrayValue(plug);
      pauseBracket.resume();

      unsigned int elements = arrayHandle.elementCount();
      for(unsigned int i = 0; i < elements; ++i){
        arrayHandle.jumpToArrayElement(i);
        handles.push_back(arrayHandle.inputValue());

        FabricCore::RTVal polygonMesh;
        if(portRTVal.isArray())
        {
          if(portRTVal.getArraySize() <= i)
          {
            polygonMesh = FabricSplice::constructObjectRTVal("PolygonMesh");
            portRTVal.callMethod("", "push", 1, &polygonMesh);
          }
          else
          {
            polygonMesh = portRTVal.getArrayElement(i);
            if(!polygonMesh.isValid() || polygonMesh.isNullObject())
            {
              polygonMesh = FabricSplice::constructObjectRTVal("PolygonMesh");
              portRTVal.setArrayElement(i, polygonMesh);
            }
          }
          rtVals.push_back(polygonMesh);
        }
        else
        {
          if(!portRTVal.isValid() || portRTVal.isNullObject())
            portRTVal = FabricSplice::constructObjectRTVal("PolygonMesh");
          rtVals.push_back(portRTVal);
        }
      }
    }
    else
    {
      FTL::AutoProfilingPauseEvent pauseBracket(bracket);
      handles.push_back(data.inputValue(plug));
      pauseBracket.resume();

      if(argOutsidePortType == FabricCore::DFGPortType_IO)
        portRTVal = getCB(getSetUD);
      if(!portRTVal.isValid() || portRTVal.isNullObject())
        portRTVal = FabricSplice::constructObjectRTVal("PolygonMesh");
      rtVals.push_back(portRTVal);
    }

    for(size_t handleIndex=0;handleIndex<handles.size();handleIndex++) 
    {
      MObject meshObj = handles[handleIndex].asMesh();
      MFnMesh mesh(meshObj);
      FabricCore::RTVal polygonMesh = rtVals[handleIndex];

      // determine if we need a topology update
      bool requireTopoUpdate = false;
      if(!requireTopoUpdate)
      {
        uint64_t nbPolygons = polygonMesh.callMethod("UInt64", "polygonCount", 0, 0).getUInt64();
        requireTopoUpdate = nbPolygons != (uint64_t)mesh.numPolygons();
      }
      if(!requireTopoUpdate)
      {
        uint64_t nbSamples = polygonMesh.callMethod("UInt64", "polygonPointsCount", 0, 0).getUInt64();
        requireTopoUpdate = nbSamples != (uint64_t)mesh.numFaceVertices();
      }

      MPointArray mayaPoints;
      MIntArray mayaCounts, mayaIndices;

      mesh.getPoints(mayaPoints);

      if(requireTopoUpdate)
      {
        // clear the mesh
        polygonMesh.callMethod("", "clear", 0, NULL);
      }

      if(mayaPoints.length() > 0)
      {
        std::vector<FabricCore::RTVal> args(2);
        args[0] = FabricSplice::constructExternalArrayRTVal("Float64", mayaPoints.length() * 4, &mayaPoints[0]);
        args[1] = FabricSplice::constructUInt32RTVal(4); // components
        polygonMesh.callMethod("", "setPointsFromExternalArray_d", 2, &args[0]);
        mayaPoints.clear();
      }

      if(requireTopoUpdate)
      {
        mesh.getVertices(mayaCounts, mayaIndices);
        std::vector<FabricCore::RTVal> args(2);
        args[0] = FabricSplice::constructExternalArrayRTVal("UInt32", mayaCounts.length(), &mayaCounts[0]);
        args[1] = FabricSplice::constructExternalArrayRTVal("UInt32", mayaIndices.length(), &mayaIndices[0]);
        polygonMesh.callMethod("", "setTopologyFromCountsIndicesExternalArrays", 2, &args[0]);
      }

      MFloatVectorArray mayaNormals;
      MIntArray mayaNormalsCounts, mayaNormalsIds;
      mesh.getNormals(mayaNormals);
      mesh.getNormalIds(mayaNormalsCounts, mayaNormalsIds);

      if(mayaNormals.length() > 0 && mayaNormalsCounts.length() > 0 && mayaNormalsIds.length() > 0)
      {
        MFloatVectorArray values;
        values.setLength(mayaNormalsIds.length());

        unsigned int offset = 0;
        for(unsigned int i=0;i<mayaNormalsIds.length();i++)
          values[offset++] = mayaNormals[mayaNormalsIds[i]];

        std::vector<FabricCore::RTVal> args(1);
        args[0] = FabricSplice::constructExternalArrayRTVal("Float32", values.length() * 3, &values[0]);
        polygonMesh.callMethod("", "setNormalsFromExternalArray", 1, &args[0]);
        values.clear();
      }

      if(mesh.numUVSets() > 0)
      {
        MFloatArray u, v, values;
        mesh.getUVs(u, v);
        unsigned int offset = 0;

        MIntArray counts, indices;
        mesh.getAssignedUVs(counts, indices);
        counts.clear();
        values.setLength(indices.length() * 2);
        if(values.length() > 0)
        {
          for(unsigned int i=0;i<indices.length(); i++)
          {
            values[offset++] = u[indices[i]];
            values[offset++] = v[indices[i]];
          }
          u.clear();
          v.clear();

          std::vector<FabricCore::RTVal> args(2);
          args[0] = FabricSplice::constructExternalArrayRTVal("Float32", values.length(), &values[0]);
          args[1] = FabricSplice::constructUInt32RTVal(2); // components
          polygonMesh.callMethod("", "setUVsFromExternalArray", 2, &args[0]);
          values.clear();
        }
      }

      if(mesh.numColorSets() > 0)
      {
        MColorArray faceValues;

        MStringArray colorSetNames;
        mesh.getColorSetNames(colorSetNames);
        MString colorSetName = colorSetNames[0];

        mesh.getFaceVertexColors(faceValues, &colorSetName);
        if(faceValues.length() > 0)
        {
          std::vector<FabricCore::RTVal> args(2);
          args[0] = FabricSplice::constructExternalArrayRTVal("Float32", faceValues.length() * 4, &faceValues[0]);
          args[1] = FabricSplice::constructUInt32RTVal(4); // components
          polygonMesh.callMethod("", "setVertexColorsFromExternalArray", 2, &args[0]);
          faceValues.clear();
        }
      }

      mayaCounts.clear();
      mayaIndices.clear();
    }

    setCB(getSetUD, portRTVal.getFECRTValRef());
  }
  catch(FabricCore::Exception e)
  {
    mayaLogErrorFunc(e.getDesc_cstr());
    return;
  }
  catch(FabricSplice::Exception e)
  {
    mayaLogErrorFunc(e.what());
    return;
  }
}

void dfgPlugToPort_Lines(
  unsigned argIndex,
  char const *argName,
  char const *argTypeName,
  FEC_DFGPortType argOutsidePortType,
  uint64_t argRawDataSize,
  FEC_DFGBindingVisitArgs_GetCB getCB,
  FEC_DFGBindingVisitArgs_GetRawCB getRawCB,
  FEC_DFGBindingVisitArgs_SetCB setCB,
  FEC_DFGBindingVisitArgs_SetRawCB setRawCB,
  void *getSetUD,
  MPlug &plug, 
  MDataBlock &data)
{
  FabricMayaProfilingEvent bracket("dfgPlugToPort_Lines");

  std::vector<MDataHandle> handles;
  std::vector<FabricCore::RTVal> rtVals;
  FabricCore::RTVal portRTVal;

  try
  {
    if(plug.isArray())
    {
      portRTVal = getCB(getSetUD);

      FTL::AutoProfilingPauseEvent pauseBracket(bracket);
      MArrayDataHandle arrayHandle = data.inputArrayValue(plug);
      pauseBracket.resume();

      unsigned int elements = arrayHandle.elementCount();
      for(unsigned int i = 0; i < elements; ++i){
        arrayHandle.jumpToArrayElement(i);
        handles.push_back(arrayHandle.inputValue());

        FabricCore::RTVal rtVal;
        if(portRTVal.getArraySize() <= i)
        {
          rtVal = FabricSplice::constructObjectRTVal("Lines");
          portRTVal.callMethod("", "push", 1, &rtVal);
        }
        else
        {
          rtVal = portRTVal.getArrayElement(i);
          if(!rtVal.isValid() || rtVal.isNullObject())
          {
            rtVal = FabricSplice::constructObjectRTVal("Lines");
            portRTVal.setArrayElement(i, rtVal);
          }
        }
        rtVals.push_back(rtVal);
      }
    }
    else
    {
      FTL::AutoProfilingPauseEvent pauseBracket(bracket);
      handles.push_back(data.inputValue(plug));
      pauseBracket.resume();

      if(argOutsidePortType == FabricCore::DFGPortType_IO)
        portRTVal = getCB(getSetUD);
      if(!portRTVal.isValid() || portRTVal.isNullObject())
        portRTVal = FabricSplice::constructObjectRTVal("Lines");
      rtVals.push_back(portRTVal);
    }

    for(size_t handleIndex=0;handleIndex<handles.size();handleIndex++) 
    {
      MObject curveObj = handles[handleIndex].asNurbsCurve();
      MFnNurbsCurve curve(curveObj);
      FabricCore::RTVal rtVal = rtVals[handleIndex];

      MPointArray mayaPoints;
      curve.getCVs(mayaPoints);
      std::vector<double> mayaDoubles(mayaPoints.length() * 3);

      size_t nbSegments = (mayaPoints.length() - 1);
      if(curve.form() == MFnNurbsCurve::kClosed)
        nbSegments++;

      std::vector<uint32_t> mayaIndices(nbSegments * 2);

      size_t voffset = 0;
      size_t coffset = 0;
      for(unsigned int i=0;i<mayaPoints.length();i++)
      {
        mayaDoubles[voffset++] = mayaPoints[i].x;
        mayaDoubles[voffset++] = mayaPoints[i].y;
        mayaDoubles[voffset++] = mayaPoints[i].z;
        if(i < mayaPoints.length() - 1)
        {
          mayaIndices[coffset++] = i;
          mayaIndices[coffset++] = i + 1;
        }
        else if(curve.form() == MFnNurbsCurve::kClosed)
        {
          mayaIndices[coffset++] = i;
          mayaIndices[coffset++] = 0;
        }
      }

      FabricCore::RTVal mayaDoublesVal = FabricSplice::constructExternalArrayRTVal("Float64", mayaDoubles.size(), &mayaDoubles[0]);
      rtVal.callMethod("", "_setPositionsFromExternalArray_d", 1, &mayaDoublesVal);

      FabricCore::RTVal mayaIndicesVal = FabricSplice::constructExternalArrayRTVal("UInt32", mayaIndices.size(), &mayaIndices[0]);
      rtVal.callMethod("", "_setTopologyFromExternalArray", 1, &mayaIndicesVal);
    }

    setCB(getSetUD, portRTVal.getFECRTValRef());
  }
  catch(FabricCore::Exception e)
  {
    mayaLogErrorFunc(e.getDesc_cstr());
    return;
  }
  catch(FabricSplice::Exception e)
  {
    mayaLogErrorFunc(e.what());
    return;
  }
}

void dfgPlugToPort_KeyframeTrack_helper(MFnAnimCurve & curve, FabricCore::RTVal & trackVal) {

  FabricMayaProfilingEvent bracket("dfgPlugToPort_KeyframeTrack_helper");

  CORE_CATCH_BEGIN;

  // find the usage of this plug
  // with this we might be able to determine color
  MString curveName = curve.name();
  double red, green, blue;
  red = green = blue = 0.0;
  if(curveName.indexW("_translateX") > -1 || curveName.indexW("_rotateX") > -1 || curveName.indexW("_scaleX") > -1)
    red = 1.0;
  else if(curveName.indexW("_translateY") > -1 || curveName.indexW("_rotateY") > -1 || curveName.indexW("_scaleY") > -1)
    green = 1.0;
  else if(curveName.indexW("_translateZ") > -1 || curveName.indexW("_rotateZ") > -1 || curveName.indexW("_scaleZ") > -1)
    blue = 1.0;

  trackVal = FabricSplice::constructObjectRTVal("KeyframeTrack");
  FabricCore::RTVal keysVal = trackVal.maybeGetMember("keys");
  FabricCore::RTVal colorVal = FabricSplice::constructRTVal("Color");
  FabricCore::RTVal numKeysVal = FabricSplice::constructUInt32RTVal(curve.numKeys());
  keysVal.callMethod("", "resize", 1, &numKeysVal);

  trackVal.setMember("name", FabricSplice::constructStringRTVal(curveName.asChar()));
  colorVal.setMember("r", FabricSplice::constructFloat64RTVal(red));
  colorVal.setMember("g", FabricSplice::constructFloat64RTVal(green));
  colorVal.setMember("b", FabricSplice::constructFloat64RTVal(blue));
  colorVal.setMember("a", FabricSplice::constructFloat64RTVal(1.0));
  trackVal.setMember("color", colorVal);
  trackVal.setMember("defaultInterpolation", FabricSplice::constructSInt32RTVal(2));
  trackVal.setMember("defaultValue", FabricSplice::constructFloat64RTVal(0.0));

  bool weighted = curve.isWeighted();

  for(unsigned int i=0;i<curve.numKeys();i++)
  {
    FabricCore::RTVal keyVal = FabricSplice::constructRTVal("Keyframe");
    FabricCore::RTVal inTangentVal = FabricSplice::constructRTVal("Vec2");
    FabricCore::RTVal outTangentVal = FabricSplice::constructRTVal("Vec2");

    // Integer interpolation;
    double keyTime = curve.time(i).as(MTime::kSeconds);
    double keyValue = curve.value(i);
    keyVal.setMember("time", FabricSplice::constructFloat64RTVal(keyTime));
    keyVal.setMember("value", FabricSplice::constructFloat64RTVal(keyValue));

    if(i > 0)
    {
      double prevKeyTime = curve.time(i-1).as(MTime::kSeconds);
      double timeDelta = keyTime - prevKeyTime;
      
      float x,y;
      curve.getTangent(i, x, y, true);
      
      float weight = 1.0f/3.0f;
      float gradient = 0.0;
      
      // Weighted out tangents are defined as 3*(P4 - P3),
      // So multiplly by 1/3 to get P3, and then divide by timeDelta
      // to get the ratio stored by the Fabric Engine keyframes.
      // Also note that the default value of 1/3 for the handle weight 
      // will create equally spaced handles, effectively the same as
      // Maya's non-weighted curves.
      if(weighted && fabs(timeDelta) > 0.0001)
        weight = (x*-1.0/3.0)/timeDelta;
      if(fabs(x) > 0.0001)
        gradient = y/x;
        //gradient = ((y*1.0/3.0)/valueDelta)/((x*1.0/3.0)/timeDelta);

      inTangentVal.setMember("x", FabricSplice::constructFloat64RTVal(weight));
      inTangentVal.setMember("y", FabricSplice::constructFloat64RTVal(gradient));
    }

    if(i < curve.numKeys()-1)
    {
      double nextKeyTime = curve.time(i+1).as(MTime::kSeconds);
      double timeDelta = nextKeyTime - keyTime;
      
      float x,y;
      curve.getTangent(i, x, y, false);
      
      float weight = 1.0f/3.0f;
      float gradient = 0.0;
      
      // Weighted out tangents are defined as 3*(P2 - P1),
      // So multiplly by 1/3 to get P2, and then divide by timeDelta
      // to get the ratio stored by the Fabric Engine keyframes.
      // Also note that the default value of 1/3 for the handle weight 
      // will create equally spaced handles, effectively the same as
      // Maya's non-weighted curves.
      if(weighted && fabs(timeDelta) > 0.0001)
        weight = (x*1.0/3.0)/timeDelta;
      if(fabs(x) > 0.0001)
        gradient = y/x;
        //gradient = ((y*1.0/3.0)/valueDelta)/((x*1.0/3.0)/timeDelta);
      
      outTangentVal.setMember("x", FabricSplice::constructFloat64RTVal(weight));
      outTangentVal.setMember("y", FabricSplice::constructFloat64RTVal(gradient));
    }

    keyVal.setMember("inTangent", inTangentVal);
    keyVal.setMember("outTangent", outTangentVal);
    int interpolation = 2;
    if(curve.outTangentType(i) == MFnAnimCurve::kTangentFlat)
      interpolation = 0;
    else if(curve.outTangentType(i) == MFnAnimCurve::kTangentLinear)
      interpolation = 1;
    keyVal.setMember("interpolation", FabricSplice::constructSInt32RTVal(interpolation));
    keysVal.setArrayElement(i, keyVal);
  }

  trackVal.setMember("keys", keysVal);

  CORE_CATCH_END;
}

void dfgPlugToPort_KeyframeTrack(
  unsigned argIndex,
  char const *argName,
  char const *argTypeName,
  FEC_DFGPortType argOutsidePortType,
  uint64_t argRawDataSize,
  FEC_DFGBindingVisitArgs_GetCB getCB,
  FEC_DFGBindingVisitArgs_GetRawCB getRawCB,
  FEC_DFGBindingVisitArgs_SetCB setCB,
  FEC_DFGBindingVisitArgs_SetRawCB setRawCB,
  void *getSetUD,
  MPlug &plug, 
  MDataBlock &data)
{
  if(!plug.isArray()){
    
    MPlugArray plugs;
    plug.connectedTo(plugs,true,false);
    MFnAnimCurve curve;
    for(unsigned int i=0;i<plugs.length();i++)
    {
      MFnDependencyNode fcurveNode(plugs[i].node());
      MString nodeTypeStr = fcurveNode.typeName();
      if(nodeTypeStr.substring(0,8) == "animCurve")
      {
        curve.setObject(plugs[i].node());
        break;
      }
    }
    if(curve.object().isNull())
      return;

    FabricCore::RTVal trackVal;
    dfgPlugToPort_KeyframeTrack_helper(curve, trackVal);
    setCB(getSetUD, trackVal.getFECRTValRef());
  } else {

    FabricCore::RTVal trackVals = FabricSplice::constructRTVal("KeyframeTrack[]");
    trackVals.setArraySize(plug.numElements());

    for(unsigned int j=0;j<plug.numElements();j++) {

      MPlug element = plug.elementByPhysicalIndex(j);

      MPlugArray plugs;
      element.connectedTo(plugs,true,false);
      MFnAnimCurve curve;
      for(unsigned int i=0;i<plugs.length();i++)
      {
        MFnDependencyNode fcurveNode(plugs[i].node());
        MString nodeTypeStr = fcurveNode.typeName();
        if(nodeTypeStr.substring(0,8) == "animCurve")
        {
          curve.setObject(plugs[i].node());
          break;
        }
      }
      if(curve.object().isNull())
        continue;

      FabricCore::RTVal trackVal;
      dfgPlugToPort_KeyframeTrack_helper(curve, trackVal);

      trackVals.setArrayElement(j, trackVal);
    }

    setCB(getSetUD, trackVals.getFECRTValRef());
  }
}

void dfgPlugToPort_spliceMayaData(
  unsigned argIndex,
  char const *argName,
  char const *argTypeName,
  FEC_DFGPortType argOutsidePortType,
  uint64_t argRawDataSize,
  FEC_DFGBindingVisitArgs_GetCB getCB,
  FEC_DFGBindingVisitArgs_GetRawCB getRawCB,
  FEC_DFGBindingVisitArgs_SetCB setCB,
  FEC_DFGBindingVisitArgs_SetRawCB setRawCB,
  void *getSetUD,
  MPlug &plug, 
  MDataBlock &data)
{
  FabricMayaProfilingEvent bracket("dfgPlugToPort_spliceMayaData");

  try{

    /*
    // todo: renable the disableSpliceMayaDataConversion feature
    const char *option = binding.getExec().getExecPortMetadata(argName, "disableSpliceMayaDataConversion");
    if(option)
    {
      if(FTL::CStrRef(option) == "true")
      {
        // this is an unconnected opaque port, exit early
        return;
      }
    }
    */

    if(!plug.isArray()){
      FTL::AutoProfilingPauseEvent pauseBracket(bracket);
      MDataHandle handle = data.inputValue(plug);
      pauseBracket.resume();
      MObject spliceMayaDataObj = handle.data();
      MFnPluginData mfn(spliceMayaDataObj);
      FabricSpliceMayaData *spliceMayaData = (FabricSpliceMayaData*)mfn.data();
      if(!spliceMayaData)
        return;

      setCB(getSetUD, spliceMayaData->getRTVal().getFECRTValRef());
    }else{
      FTL::AutoProfilingPauseEvent pauseBracket(bracket);
      MArrayDataHandle arrayHandle = data.inputArrayValue(plug);
      pauseBracket.resume();
      unsigned int elements = arrayHandle.elementCount();

      FabricCore::RTVal value = getCB(getSetUD);
      if(!value.isArray())
        return;
      value.setArraySize(elements);

      for(unsigned int i = 0; i < elements; ++i){
        arrayHandle.jumpToArrayElement(i);
        MDataHandle childHandle = arrayHandle.inputValue();

        childHandle.asMatrix();
        MObject spliceMayaDataObj = childHandle.data();
        MFnPluginData mfn(spliceMayaDataObj);
        FabricSpliceMayaData *spliceMayaData = (FabricSpliceMayaData*)mfn.data();
        if(!spliceMayaData)
          return;
        value.setArrayElement(i, spliceMayaData->getRTVal());
      }

      setCB(getSetUD, value.getFECRTValRef());
    }
  }
  catch(FabricCore::Exception e)
  {
    mayaLogErrorFunc(e.getDesc_cstr());
    return;
  }
}

void dfgPortToPlug_compound_convertMat44(MMatrix & matrix, FabricCore::RTVal & rtVal)
{
  CORE_CATCH_BEGIN;
  Mat44ToMMatrix(rtVal, matrix);
  CORE_CATCH_END;
}

void dfgPortToPlug_compound_convertCompound(MFnCompoundAttribute & compound, MDataHandle & handle, FabricCore::RTVal & rtVal)
{
  CORE_CATCH_BEGIN;

  std::vector<FabricCore::RTVal> args(5);
  FTL::CStrRef valueType;

  valueType = rtVal.callMethod("String", "getValueType", 0, 0).getStringCString();

  // treat special cases
  if(compound.numChildren() == 3)
  {
    MString compoundName = compound.name();
    FabricCore::RTVal compoundNameRTVal = FabricSplice::constructStringRTVal(compoundName.asChar());

    MFnAttribute x(compound.child(0));
    MFnAttribute y(compound.child(1));
    MFnAttribute z(compound.child(2));

    if(x.name() == compoundName+"X" && y.name() == compoundName+"Y" && z.name() == compoundName+"Z")
    {
      MStatus attrStatus;
      MFnNumericAttribute nx(x.object(), &attrStatus);
      if(attrStatus == MS::kSuccess)
      {
        if(!compound.isArray())
        {
          if(valueType != "Vec3")
          {
            mayaLogErrorFunc("Incompatible param for compound attribute - expected a Vec3Param.");
            return;
          }

          FabricCore::RTVal value = rtVal.callMethod("Vec3", "getValue", 0, 0);
          if(handle.numericType() == MFnNumericData::k3Float || handle.numericType() == MFnNumericData::kFloat){
            handle.set3Float(
              (float)dfgGetFloat64FromRTVal(value.maybeGetMember("x")),
              (float)dfgGetFloat64FromRTVal(value.maybeGetMember("y")),
              (float)dfgGetFloat64FromRTVal(value.maybeGetMember("z")));
          } else {
            handle.set3Double(
              dfgGetFloat64FromRTVal(value.maybeGetMember("x")),
              dfgGetFloat64FromRTVal(value.maybeGetMember("y")),
              dfgGetFloat64FromRTVal(value.maybeGetMember("z")));
          }
        }
        else
        {
          if(valueType != "Vec3[]")
          {
            mayaLogErrorFunc("Incompatible param for compound attribute - expected a Vec3ArrayParam.");
            return;
          }

          MArrayDataHandle arrayHandle(handle);
          FabricCore::RTVal arrayValue = rtVal.maybeGetMember("values");
          unsigned int arraySize = arrayValue.getArraySize();

          MArrayDataBuilder arraybuilder = arrayHandle.builder();

          for(unsigned int i = 0; i < arraySize; ++i){
            FabricCore::RTVal value = rtVal.getArrayElement(i);
            MDataHandle elementHandle = arraybuilder.addElement(i);
            if(elementHandle.numericType() == MFnNumericData::k3Float || elementHandle.numericType() == MFnNumericData::kFloat){
              elementHandle.set3Float(
                (float)dfgGetFloat64FromRTVal(value.maybeGetMember("x")),
                (float)dfgGetFloat64FromRTVal(value.maybeGetMember("y")),
                (float)dfgGetFloat64FromRTVal(value.maybeGetMember("z")));
            } else {
              elementHandle.set3Double(
                dfgGetFloat64FromRTVal(value.maybeGetMember("x")),
                dfgGetFloat64FromRTVal(value.maybeGetMember("y")),
                dfgGetFloat64FromRTVal(value.maybeGetMember("z")));
            }
          }
          arrayHandle.set(arraybuilder);
          arrayHandle.setAllClean();
        }

        return;
      }
      else
      {
        MFnUnitAttribute ux(x.object(), &attrStatus);
        if(attrStatus == MS::kSuccess)
        {
          if(!compound.isArray())
          {
            if(valueType != "Euler")
            {
              mayaLogErrorFunc("Incompatible param for compound attribute - expected a EulerParam.");
              return;
            }

            FabricCore::RTVal value = rtVal.callMethod("Euler", "getValue", 0, 0);
            if(handle.numericType() == MFnNumericData::k3Float || handle.numericType() == MFnNumericData::kFloat){
              handle.set3Float(
                (float)dfgGetFloat64FromRTVal(value.maybeGetMember("x")),
                (float)dfgGetFloat64FromRTVal(value.maybeGetMember("y")),
                (float)dfgGetFloat64FromRTVal(value.maybeGetMember("z")));
            } else {
              handle.set3Double(
                dfgGetFloat64FromRTVal(value.maybeGetMember("x")),
                dfgGetFloat64FromRTVal(value.maybeGetMember("y")),
                dfgGetFloat64FromRTVal(value.maybeGetMember("z")));
            }
          }
          else
          {
            if(valueType != "Euler[]")
            {
              mayaLogErrorFunc("Incompatible param for compound attribute - expected a EulerArrayParam.");
              return;
            }

            MArrayDataHandle arrayHandle(handle);
            FabricCore::RTVal arrayValue = rtVal.maybeGetMember("values");
            unsigned int arraySize = arrayValue.getArraySize();

            MArrayDataBuilder arraybuilder = arrayHandle.builder();

            for(unsigned int i = 0; i < arraySize; ++i){
              FabricCore::RTVal value = rtVal.getArrayElement(i);
              MDataHandle elementHandle = arraybuilder.addElement(i);
              if(elementHandle.numericType() == MFnNumericData::k3Float || elementHandle.numericType() == MFnNumericData::kFloat){
                elementHandle.set3Float(
                  (float)dfgGetFloat64FromRTVal(value.maybeGetMember("x")),
                  (float)dfgGetFloat64FromRTVal(value.maybeGetMember("y")),
                  (float)dfgGetFloat64FromRTVal(value.maybeGetMember("z")));
              } else {
                elementHandle.set3Double(
                  dfgGetFloat64FromRTVal(value.maybeGetMember("x")),
                  dfgGetFloat64FromRTVal(value.maybeGetMember("y")),
                  dfgGetFloat64FromRTVal(value.maybeGetMember("z")));
              }
            }
            arrayHandle.set(arraybuilder);
            arrayHandle.setAllClean();
          }

          return;
        }
      }
    }
  }

  for(unsigned int i=0;i<compound.numChildren();i++)
  {
    MFnAttribute child(compound.child(i));
    MString childName = child.name();
    FabricCore::RTVal childNameRTVal = FabricSplice::constructStringRTVal(childName.asChar());

    if(!rtVal.callMethod("Boolean", "hasParam", 1, &childNameRTVal).getBoolean())
    {
      mayaLogFunc("Compound attribute child '"+childName+"' exists, but not found as child KL parameter.");
      continue;
    }

    FabricCore::RTVal childRTVal = rtVal.callMethod("Param", "getParam", 1, &childNameRTVal);
    valueType = childRTVal.callMethod("String", "getValueType", 0, 0).getStringCString();
    MStatus attrStatus;

    MFnNumericAttribute nAttr(child.object(), &attrStatus);
    if(attrStatus == MS::kSuccess)
    {
      if(nAttr.unitType() == MFnNumericData::kBoolean)
      {
        if(!nAttr.isArray())
        {
          if(valueType != "Boolean")
          {
            mayaLogErrorFunc("Incompatible param for compound attribute - expected a BooleanParam.");
            return;
          }

          childRTVal = rtVal.callMethod("BooleanParam", "getParam", 1, &childNameRTVal);
          MDataHandle childHandle(handle.child(child.object()));
          childHandle.setBool(childRTVal.callMethod("Boolean", "getValue", 0, 0).getBoolean());
        }
        else
        {
          if(valueType != "Boolean[]")
          {
            mayaLogErrorFunc("Incompatible param for compound attribute - expected a BooleanArrayParam.");
            return;
          }

          childRTVal = rtVal.callMethod("BooleanArrayParam", "getParam", 1, &childNameRTVal);
          FabricCore::RTVal arrayValues = childRTVal.maybeGetMember("values");
          MArrayDataHandle childHandle(handle.child(child.object()));
          MArrayDataBuilder arraybuilder = childHandle.builder();

          for(unsigned int j=0;j<arrayValues.getArraySize();j++)
          {
            MDataHandle elementHandle = arraybuilder.addElement(i);
            elementHandle.setBool(arrayValues.getArrayElement(i).getBoolean());
          }

          childHandle.set(arraybuilder);
          childHandle.setAllClean();
        }
      }
      else if(nAttr.unitType() == MFnNumericData::kInt)
      {
        if(!nAttr.isArray())
        {
          if(valueType != "SInt32")
          {
            mayaLogErrorFunc("Incompatible param for compound attribute - expected a SInt32Param.");
            return;
          }

          childRTVal = rtVal.callMethod("SInt32Param", "getParam", 1, &childNameRTVal);
          MDataHandle childHandle(handle.child(child.object()));
          childHandle.setInt(childRTVal.callMethod("SInt32", "getValue", 0, 0).getSInt32());
        }
        else
        {
          if(valueType != "SInt32[]")
          {
            mayaLogErrorFunc("Incompatible param for compound attribute - expected a SInt32ArrayParam.");
            return;
          }

          childRTVal = rtVal.callMethod("SInt32ArrayParam", "getParam", 1, &childNameRTVal);
          FabricCore::RTVal arrayValues = childRTVal.maybeGetMember("values");
          MArrayDataHandle childHandle(handle.child(child.object()));
          MArrayDataBuilder arraybuilder = childHandle.builder();

          for(unsigned int j=0;j<arrayValues.getArraySize();j++)
          {
            MDataHandle elementHandle = arraybuilder.addElement(i);
            elementHandle.setInt(arrayValues.getArrayElement(i).getSInt32());
          }

          childHandle.set(arraybuilder);
          childHandle.setAllClean();
        }
      }
      else if(nAttr.unitType() == MFnNumericData::kFloat)
      {
        if(!nAttr.isArray())
        {
          if(valueType != "Float64")
          {
            mayaLogErrorFunc("Incompatible param for compound attribute - expected a Float64Param.");
            return;
          }

          childRTVal = rtVal.callMethod("Float64Param", "getParam", 1, &childNameRTVal);
          MDataHandle childHandle(handle.child(child.object()));
          childHandle.setFloat(childRTVal.callMethod("Float64", "getValue", 0, 0).getFloat64());
        }
        else
        {
          if(valueType != "Float64[]")
          {
            mayaLogErrorFunc("Incompatible param for compound attribute - expected a Float64ArrayParam.");
            return;
          }

          childRTVal = rtVal.callMethod("Float64ArrayParam", "getParam", 1, &childNameRTVal);
          FabricCore::RTVal arrayValues = childRTVal.maybeGetMember("values");
          MArrayDataHandle childHandle(handle.child(child.object()));
          MArrayDataBuilder arraybuilder = childHandle.builder();

          for(unsigned int j=0;j<arrayValues.getArraySize();j++)
          {
            MDataHandle elementHandle = arraybuilder.addElement(i);
            elementHandle.setFloat(arrayValues.getArrayElement(i).getFloat64());
          }

          childHandle.set(arraybuilder);
          childHandle.setAllClean();
        }
      }
      else if(nAttr.unitType() == MFnNumericData::kDouble)
      {
        if(!nAttr.isArray())
        {
          if(valueType != "Float64")
          {
            mayaLogErrorFunc("Incompatible param for compound attribute - expected a Float64Param.");
            return;
          }

          childRTVal = rtVal.callMethod("Float64Param", "getParam", 1, &childNameRTVal);
          MDataHandle childHandle(handle.child(child.object()));
          childHandle.setDouble(childRTVal.callMethod("Float64", "getValue", 0, 0).getFloat64());
        }
        else
        {
          if(valueType != "Float64[]")
          {
            mayaLogErrorFunc("Incompatible param for compound attribute - expected a Float64ArrayParam.");
            return;
          }

          childRTVal = rtVal.callMethod("Float64ArrayParam", "getParam", 1, &childNameRTVal);
          FabricCore::RTVal arrayValues = childRTVal.maybeGetMember("values");
          MArrayDataHandle childHandle(handle.child(child.object()));
          MArrayDataBuilder arraybuilder = childHandle.builder();

          for(unsigned int j=0;j<arrayValues.getArraySize();j++)
          {
            MDataHandle elementHandle = arraybuilder.addElement(i);
            elementHandle.setDouble(arrayValues.getArrayElement(i).getFloat64());
          }

          childHandle.set(arraybuilder);
          childHandle.setAllClean();
        }
      }
      else if(nAttr.unitType() == MFnNumericData::k3Double) // vec3
      {
        if(!nAttr.isArray())
        {
          if(valueType != "Vec3")
          {
            mayaLogErrorFunc("Incompatible param for compound attribute - expected a Vec3Param.");
            return;
          }

          childRTVal = rtVal.callMethod("Vec3Param", "getParam", 1, &childNameRTVal);
          MDataHandle childHandle(handle.child(child.object()));

          FabricCore::RTVal value = childRTVal.maybeGetMember("value");
          MFloatVector v(
            dfgGetFloat64FromRTVal(value.maybeGetMember("x")),
            dfgGetFloat64FromRTVal(value.maybeGetMember("y")),
            dfgGetFloat64FromRTVal(value.maybeGetMember("z"))
          );
          childHandle.setMFloatVector(v);
        }
        else
        {
          if(valueType != "Vec3[]")
          {
            mayaLogErrorFunc("Incompatible param for compound attribute - expected a Vec3ArrayParam.");
            return;
          }

          childRTVal = rtVal.callMethod("Vec3ArrayParam", "getParam", 1, &childNameRTVal);
          FabricCore::RTVal arrayValues = childRTVal.maybeGetMember("values");
          MArrayDataHandle childHandle(handle.child(child.object()));
          MArrayDataBuilder arraybuilder = childHandle.builder();

          for(unsigned int j=0;j<arrayValues.getArraySize();j++)
          {
            FabricCore::RTVal value = childRTVal.maybeGetMember("value");
            MFloatVector v(
              dfgGetFloat64FromRTVal(value.maybeGetMember("x")),
              dfgGetFloat64FromRTVal(value.maybeGetMember("y")),
              dfgGetFloat64FromRTVal(value.maybeGetMember("z"))
            );
            MDataHandle elementHandle = arraybuilder.addElement(i);
            elementHandle.setMFloatVector(v);
          }

          childHandle.set(arraybuilder);
          childHandle.setAllClean();
        }
      }
      else if(nAttr.unitType() == MFnNumericData::k3Float) // color
      {
        if(!nAttr.isArray())
        {
          if(valueType != "Color")
          {
            mayaLogErrorFunc("Incompatible param for compound attribute - expected a ColorParam.");
            return;
          }

          childRTVal = rtVal.callMethod("ColorParam", "getParam", 1, &childNameRTVal);
          MDataHandle childHandle(handle.child(child.object()));

          FabricCore::RTVal value = childRTVal.maybeGetMember("value");
          MFloatVector v(
            dfgGetFloat64FromRTVal(value.maybeGetMember("r")),
            dfgGetFloat64FromRTVal(value.maybeGetMember("g")),
            dfgGetFloat64FromRTVal(value.maybeGetMember("b"))
          );
          childHandle.setMFloatVector(v);
        }
        else
        {
          if(valueType != "Color[]")
          {
            mayaLogErrorFunc("Incompatible param for compound attribute - expected a ColorArrayParam.");
            return;
          }

          childRTVal = rtVal.callMethod("ColorArrayParam", "getParam", 1, &childNameRTVal);
          FabricCore::RTVal arrayValues = childRTVal.maybeGetMember("values");
          MArrayDataHandle childHandle(handle.child(child.object()));
          MArrayDataBuilder arraybuilder = childHandle.builder();

          for(unsigned int j=0;j<arrayValues.getArraySize();j++)
          {
            FabricCore::RTVal value = childRTVal.maybeGetMember("value");
            MFloatVector v(
              dfgGetFloat64FromRTVal(value.maybeGetMember("r")),
              dfgGetFloat64FromRTVal(value.maybeGetMember("g")),
              dfgGetFloat64FromRTVal(value.maybeGetMember("b"))
            );
            MDataHandle elementHandle = arraybuilder.addElement(i);
            elementHandle.setMFloatVector(v);
          }

          childHandle.set(arraybuilder);
          childHandle.setAllClean();
        }
      }
      else
      {
        mayaLogErrorFunc("Unsupported numeric attribute '"+childName+"'.");
        return;
      }
    }
    else
    {
      MFnTypedAttribute tAttr(child.object(), &attrStatus);
      if(attrStatus == MS::kSuccess)
      {
        if(tAttr.attrType() == MFnData::kString)
        {
          if(!tAttr.isArray())
          {
            if(valueType != "String")
            {
              mayaLogErrorFunc("Incompatible param for compound attribute - expected a String=Param.");
              return;
            }

            childRTVal = rtVal.callMethod("StringParam", "getParam", 1, &childNameRTVal);
            MDataHandle childHandle(handle.child(child.object()));

            FabricCore::RTVal value = childRTVal.maybeGetMember("value");
            childHandle.setString(value.getStringCString());
          }
          else
          {
           if(valueType != "String[]")
            {
              mayaLogErrorFunc("Incompatible param for compound attribute - expected a StringArrayParam.");
              return;
            }

            childRTVal = rtVal.callMethod("StringArrayParam", "getParam", 1, &childNameRTVal);
            FabricCore::RTVal valuesRTVal = childRTVal.maybeGetMember("values");
            MArrayDataHandle childHandle(handle.child(child.object()));
            MArrayDataBuilder arraybuilder = childHandle.builder();

            for(unsigned int j=0;j<valuesRTVal.getArraySize();j++)
            {
              FabricCore::RTVal valueRT = valuesRTVal.getArrayElement(j);
              MDataHandle elementHandle = arraybuilder.addElement(j);
              elementHandle.setString(valueRT.getStringCString());
            }

            childHandle.set(arraybuilder);
            childHandle.setAllClean();
          }
        }
        else if(tAttr.attrType() == MFnData::kIntArray)
        {
          if(!tAttr.isArray())
          {
            if(valueType != "SInt32[]")
            {
              mayaLogErrorFunc("Incompatible param for compound attribute - expected a SInt32ArrayParam.");
              return;
            }

            MArrayDataHandle childHandle(handle.child(child.object()));

            childRTVal = rtVal.callMethod("SInt32ArrayParam", "getParam", 1, &childNameRTVal);
            FabricCore::RTVal valuesRTVal = childRTVal.maybeGetMember("values");
            FabricCore::RTVal dataRtVal = valuesRTVal.callMethod("Data", "data", 0, 0);

            MIntArray arrayValues;
            arrayValues.setLength(valuesRTVal.getArraySize());
            memcpy(&arrayValues[0], dataRtVal.getData(), sizeof(int32_t) * arrayValues.length());
            handle.set(MFnIntArrayData().create(arrayValues));
          }
          else
          {
            mayaLogErrorFunc("Arrays of MFnData::kIntArray are not supported for '"+childName+"'.");
          }
        }
        else if(tAttr.attrType() == MFnData::kDoubleArray)
        {
          if(!tAttr.isArray())
          {
            if(valueType != "Float64[]")
            {
              mayaLogErrorFunc("Incompatible param for compound attribute - expected a Float64ArrayParam.");
              return;
            }

            MArrayDataHandle childHandle(handle.child(child.object()));

            childRTVal = rtVal.callMethod("Float64ArrayParam", "getParam", 1, &childNameRTVal);
            FabricCore::RTVal valuesRTVal = childRTVal.maybeGetMember("values");
            FabricCore::RTVal dataRtVal = valuesRTVal.callMethod("Data", "data", 0, 0);

            MDoubleArray arrayValues;
            arrayValues.setLength(valuesRTVal.getArraySize());
            memcpy(&arrayValues[0], dataRtVal.getData(), sizeof(double) * arrayValues.length());
            handle.set(MFnDoubleArrayData().create(arrayValues));
          }
          else
          {
            mayaLogErrorFunc("Arrays of MFnData::kDoubleArray are not supported for '"+childName+"'.");
          }
        }
        else if(tAttr.attrType() == MFnData::kVectorArray)
        {
          if(!tAttr.isArray())
          {
            if(valueType != "Vec3[]")
            {
              mayaLogErrorFunc("Incompatible param for compound attribute - expected a Vec3ArrayParam.");
              return;
            }

            MArrayDataHandle childHandle(handle.child(child.object()));

            childRTVal = rtVal.callMethod("Vec3ArrayParam", "getParam", 1, &childNameRTVal);
            FabricCore::RTVal valuesRTVal = childRTVal.maybeGetMember("values");
            FabricCore::RTVal dataRtVal = valuesRTVal.callMethod("Data", "data", 0, 0);
            floatVec * data = (floatVec *)dataRtVal.getData();

            MVectorArray arrayValues;
            arrayValues.setLength(valuesRTVal.getArraySize());

            for(uint32_t i=0;i<arrayValues.length();i++)
              arrayValues.set(data[i], i);

            handle.set(MFnVectorArrayData().create(arrayValues));
          }
          else
          {
            mayaLogErrorFunc("Arrays of MFnData::kVectorArray are not supported for '"+childName+"'.");
          }
        }
        else
        {
          mayaLogErrorFunc("Unsupported typed attribute '"+childName+"'.");
          return;
        }
      }
      else
      {
        MFnMatrixAttribute mAttr(child.object(), &attrStatus);
        if(attrStatus == MS::kSuccess)
        {
          if(!mAttr.isArray())
          {
            if(valueType != "Mat44")
            {
              mayaLogErrorFunc("Incompatible param for compound attribute - expected a Mat44Param.");
              return;
            }

            childRTVal = rtVal.callMethod("Mat44Param", "getParam", 1, &childNameRTVal);
            MDataHandle childHandle(handle.child(child.object()));

            FabricCore::RTVal value = childRTVal.maybeGetMember("value");
            MMatrix m;
            dfgPortToPlug_compound_convertMat44(m, value);
            childHandle.setMMatrix(m);
          }
          else
          {
            if(valueType != "Mat44[]")
            {
              mayaLogErrorFunc("Incompatible param for compound attribute - expected a Mat44ArrayParam.");
              return;
            }

            childRTVal = rtVal.callMethod("Mat44ArrayParam", "getParam", 1, &childNameRTVal);
            FabricCore::RTVal arrayValues = childRTVal.maybeGetMember("values");
            MArrayDataHandle childHandle(handle.child(child.object()));
            MArrayDataBuilder arraybuilder = childHandle.builder();

            for(unsigned int j=0;j<arrayValues.getArraySize();j++)
            {
              FabricCore::RTVal value = childRTVal.maybeGetMember("value");
              MDataHandle elementHandle = arraybuilder.addElement(i);
              MMatrix m;
              dfgPortToPlug_compound_convertMat44(m, value);
              elementHandle.setMMatrix(m);
            }

            childHandle.set(arraybuilder);
            childHandle.setAllClean();
          }
        }
        else
        {
          MFnCompoundAttribute cAttr(child.object(), &attrStatus);
          if(attrStatus == MS::kSuccess)
          {
            MFnCompoundAttribute cAttr(compound.child(i));
            if(!cAttr.isArray())
            {
              if(valueType != "Compound")
              {
                mayaLogErrorFunc("Incompatible param for compound attribute - expected a CompoundParam.");
                return;
              }

              MDataHandle childHandle(handle.child(child.object()));
              childRTVal = rtVal.callMethod("CompoundParam", "getParam", 1, &childNameRTVal);
              dfgPortToPlug_compound_convertCompound(cAttr, childHandle, childRTVal);
            }
          }
        }
      }
    }
  }

  CORE_CATCH_END;
}

void dfgPortToPlug_compound(
  unsigned argIndex,
  char const *argName,
  char const *argTypeName,
  FEC_DFGPortType argOutsidePortType,
  uint64_t argRawDataSize,
  FEC_DFGBindingVisitArgs_GetCB getCB,
  FEC_DFGBindingVisitArgs_GetRawCB getRawCB,
  void *getSetUD,
  MPlug &plug, 
  MDataBlock &data)
{
  if(plug.isArray())
    return;

  MDataHandle handle = data.outputValue(plug);
  FabricCore::RTVal rtVal = getCB(getSetUD);
  MFnCompoundAttribute compound(plug.attribute());
  dfgPortToPlug_compound_convertCompound(compound, handle, rtVal);
  handle.setClean();
}

void dfgPortToPlug_bool(
  unsigned argIndex,
  char const *argName,
  char const *argTypeName,
  FEC_DFGPortType argOutsidePortType,
  uint64_t argRawDataSize,
  FEC_DFGBindingVisitArgs_GetCB getCB,
  FEC_DFGBindingVisitArgs_GetRawCB getRawCB,
  void *getSetUD,
  MPlug &plug, 
  MDataBlock &data)
{
  uint64_t elementDataSize = sizeof(bool);
  uint64_t numElements = argRawDataSize / elementDataSize;

  const bool * values;
  getRawCB(getSetUD, (const void**)&values);
  unsigned int offset = 0;

  if(plug.isArray()){
    MArrayDataHandle arrayHandle = data.outputArrayValue(plug);
    MArrayDataBuilder arraybuilder = arrayHandle.builder();

    for(unsigned int i = 0; i < numElements; ++i){
      MDataHandle handle = arraybuilder.addElement(i);
      handle.setBool(values[i]);
    }

    arrayHandle.set(arraybuilder);
    arrayHandle.setAllClean();
  }
  else{
    MDataHandle handle = data.outputValue(plug);
    handle.setBool(values[0]);
  }
}

void dfgPortToPlug_integer(
  unsigned argIndex,
  char const *argName,
  char const *argTypeName,
  FEC_DFGPortType argOutsidePortType,
  uint64_t argRawDataSize,
  FEC_DFGBindingVisitArgs_GetCB getCB,
  FEC_DFGBindingVisitArgs_GetRawCB getRawCB,
  void *getSetUD,
  MPlug &plug, 
  MDataBlock &data)
{
  uint64_t elementDataSize = sizeof(int32_t);
  uint64_t numElements = argRawDataSize / elementDataSize;

  const int32_t * values;
  getRawCB(getSetUD, (const void**)&values);
  unsigned int offset = 0;

  if(plug.isArray()){
    MArrayDataHandle arrayHandle = data.outputArrayValue(plug);
    MArrayDataBuilder arraybuilder = arrayHandle.builder();

    for(unsigned int i = 0; i < numElements; ++i){
      MDataHandle handle = arraybuilder.addElement(i);
      handle.setInt(values[i]);
    }

    arrayHandle.set(arraybuilder);
    arrayHandle.setAllClean();
  }else{
    MDataHandle handle = data.outputValue(plug);
    // todo
    // bool isNativeArray = FTL::CStrRef(binding.getExec().getExecPortMetadata(argName, "nativeArray")) == "true";
    if(MFnTypedAttribute(plug.attribute()).attrType() == MFnData::kIntArray) {// || isNativeArray) {

      MIntArray arrayValues;
      arrayValues.setLength(numElements);
      for(unsigned int i = 0; i < numElements; ++i)
        arrayValues[i] = values[i];

      handle.set(MFnIntArrayData().create(arrayValues));
    }else{
      handle.setInt(values[0]);
    }
  }
}

void dfgPortToPlug_scalar(
  unsigned argIndex,
  char const *argName,
  char const *argTypeName,
  FEC_DFGPortType argOutsidePortType,
  uint64_t argRawDataSize,
  FEC_DFGBindingVisitArgs_GetCB getCB,
  FEC_DFGBindingVisitArgs_GetRawCB getRawCB,
  void *getSetUD,
  MPlug &plug, 
  MDataBlock &data)
{
  CORE_CATCH_BEGIN;

  uint64_t elementDataSize = sizeof(float);
  uint64_t numElements = argRawDataSize / elementDataSize;

  const float * values;
  getRawCB(getSetUD, (const void**)&values);
  unsigned int offset = 0;

  // FTL::CStrRef scalarUnit = binding.getExec().getExecPortMetadata(argName, "scalarUnit");
  // todo: port metadata
  if(plug.isArray()){
    MArrayDataHandle arrayHandle = data.outputArrayValue(plug);
    MArrayDataBuilder arraybuilder = arrayHandle.builder();

    for(unsigned int i = 0; i < numElements; ++i){
      MDataHandle handle = arraybuilder.addElement(i);

      /*
      // todo
      if(scalarUnit == "time")
        handle.setMTime(MTime(values[i], MTime::kSeconds));
      else if(scalarUnit == "angle")
        handle.setMAngle(MAngle(values[i], MAngle::kRadians));
      else if(scalarUnit == "distance")
        handle.setMDistance(MDistance(values[i], MDistance::kMillimeters));
      else*/
      {
        if(handle.numericType() == MFnNumericData::kFloat)
          handle.setFloat(values[i]);
        else
          handle.setDouble(values[i]);
      }
    }

    arrayHandle.set(arraybuilder);
    arrayHandle.setAllClean();
  }
  else{
    MDataHandle handle = data.outputValue(plug);
    if(numElements > 1) {

      MDoubleArray doubleValues;
      doubleValues.setLength(numElements);

      for(unsigned int i=0;i<numElements;i++)
        doubleValues[i] = values[i];

      handle.set(MFnDoubleArrayData().create(doubleValues));
    }else{
      double value = values[0];
      if(value == DBL_MAX)
        return;
      /*
      if(scalarUnit == "time")
        handle.setMTime(MTime(value, MTime::kSeconds));
      else if(scalarUnit == "angle")
        handle.setMAngle(MAngle(value, MAngle::kRadians));
      else if(scalarUnit == "distance")
        handle.setMDistance(MDistance(value, MDistance::kMillimeters));
      else*/
      {
        if(handle.numericType() == MFnNumericData::kFloat)
          handle.setFloat(value);
        else
          handle.setDouble(value);
      }
    }
  }

  CORE_CATCH_END;
}

void dfgPortToPlug_string(
  unsigned argIndex,
  char const *argName,
  char const *argTypeName,
  FEC_DFGPortType argOutsidePortType,
  uint64_t argRawDataSize,
  FEC_DFGBindingVisitArgs_GetCB getCB,
  FEC_DFGBindingVisitArgs_GetRawCB getRawCB,
  void *getSetUD,
  MPlug &plug, 
  MDataBlock &data)
{
  FabricCore::RTVal rtVal = getCB(getSetUD);
  if(plug.isArray()){
    MArrayDataHandle arrayHandle = data.outputArrayValue(plug);
    MArrayDataBuilder arraybuilder = arrayHandle.builder();

    unsigned int elements = rtVal.getArraySize();
    for(unsigned int i = 0; i < elements; ++i){
      MDataHandle handle = arraybuilder.addElement(i);
      handle.setString(rtVal.getArrayElement(i).getStringCString());
    }

    arrayHandle.set(arraybuilder);
    arrayHandle.setAllClean();
  }
  else{
    MDataHandle handle = data.outputValue(plug);
    handle.setString(MString(rtVal.getStringCString()));
  }
}

void dfgPortToPlug_color(
  unsigned argIndex,
  char const *argName,
  char const *argTypeName,
  FEC_DFGPortType argOutsidePortType,
  uint64_t argRawDataSize,
  FEC_DFGBindingVisitArgs_GetCB getCB,
  FEC_DFGBindingVisitArgs_GetRawCB getRawCB,
  void *getSetUD,
  MPlug &plug,
  MDataBlock &data)
{
  uint64_t elementDataSize = sizeof(float) * 4;
  uint64_t numElements = argRawDataSize / elementDataSize;

  const float * values;
  getRawCB(getSetUD, (const void**)&values);
  unsigned int offset = 0;
 
  if(plug.isArray()){
    MArrayDataHandle arrayHandle = data.outputArrayValue(plug);
    MArrayDataBuilder arraybuilder = arrayHandle.builder();

    for(unsigned int i = 0; i < numElements; ++i){
      MDataHandle handle = arraybuilder.addElement(i);
      if(handle.numericType() == MFnNumericData::k3Float || handle.numericType() == MFnNumericData::kFloat){
        handle.setMFloatVector(MFloatVector(
          (float)values[offset+0], 
          (float)values[offset+1], 
          (float)values[offset+2]
        ));
      }else{
        handle.setMVector(MVector(
          (float)values[offset+0], 
          (float)values[offset+1], 
          (float)values[offset+2]
        ));
      }
      offset += 4;
    }

    arrayHandle.set(arraybuilder);
    arrayHandle.setAllClean();
  }
  else{
    MDataHandle handle = data.outputValue(plug);

    if(handle.numericType() == MFnNumericData::k3Float || handle.numericType() == MFnNumericData::kFloat){
      MFloatVector v(
        (float)values[offset+0], 
        (float)values[offset+1], 
        (float)values[offset+2]
      );
      handle.setMFloatVector(v);
    }else{
      MVector v(
        (float)values[offset+0], 
        (float)values[offset+1], 
        (float)values[offset+2]
      );
      handle.setMVector(v);
    }
  }
}

void dfgPortToPlug_vec3(
  unsigned argIndex,
  char const *argName,
  char const *argTypeName,
  FEC_DFGPortType argOutsidePortType,
  uint64_t argRawDataSize,
  FEC_DFGBindingVisitArgs_GetCB getCB,
  FEC_DFGBindingVisitArgs_GetRawCB getRawCB,
  void *getSetUD,
  MPlug &plug, 
  MDataBlock &data)
{
  uint64_t elementDataSize = sizeof(float) * 3;
  uint64_t numElements = argRawDataSize / elementDataSize;

  const float * values;
  getRawCB(getSetUD, (const void**)&values);
  unsigned int offset = 0;

  if(plug.isArray()){
    MArrayDataHandle arrayHandle = data.outputArrayValue(plug);
    MArrayDataBuilder arraybuilder = arrayHandle.builder();

    for(unsigned int i = 0; i < numElements; ++i){
      MDataHandle handle = arraybuilder.addElement(i);

      if(handle.numericType() == MFnNumericData::k3Float || handle.numericType() == MFnNumericData::kFloat){
        handle.set3Float(
          (float)values[offset+0], 
          (float)values[offset+1], 
          (float)values[offset+2]
        );
      } else {
        handle.set3Double(
          values[offset+0], 
          values[offset+1], 
          values[offset+2]
        );
      }
      offset+=3;
    }

    arrayHandle.set(arraybuilder);
    arrayHandle.setAllClean();
  }
  else{
    MDataHandle handle = data.outputValue(plug);
    // todo: nativeArray metadata support
    if(handle.type() == MFnData::kVectorArray) {

      MVectorArray arrayValues;
      arrayValues.setLength(numElements);
      for(unsigned int i = 0; i < numElements; ++i){
        arrayValues[i].x = values[offset++];
        arrayValues[i].y = values[offset++];
        arrayValues[i].z = values[offset++];
      }

      handle.set(MFnVectorArrayData().create(arrayValues));
    }else if(handle.type() == MFnData::kPointArray) {

      MPointArray arrayValues;
      arrayValues.setLength(numElements);
      for(unsigned int i = 0; i < numElements; ++i){
        arrayValues[i].x = values[offset++];
        arrayValues[i].y = values[offset++];
        arrayValues[i].z = values[offset++];
      }

      handle.set(MFnPointArrayData().create(arrayValues));
    }else{
      if(handle.numericType() == MFnNumericData::k3Float || handle.numericType() == MFnNumericData::kFloat){
        handle.set3Float(values[offset], values[offset+1], values[offset+2]);
      }else{
        handle.set3Double(values[offset], values[offset+1], values[offset+2]);
      }
    }
  }
}

void dfgPortToPlug_euler(
  unsigned argIndex,
  char const *argName,
  char const *argTypeName,
  FEC_DFGPortType argOutsidePortType,
  uint64_t argRawDataSize,
  FEC_DFGBindingVisitArgs_GetCB getCB,
  FEC_DFGBindingVisitArgs_GetRawCB getRawCB,
  void *getSetUD,
  MPlug &plug, 
  MDataBlock &data)
{

  uint64_t elementDataSize = sizeof(float) * 3;
  uint64_t numElements = argRawDataSize / elementDataSize;

  const float * values;
  getRawCB(getSetUD, (const void**)&values);
  unsigned int offset = 0;

  if(plug.isArray()){
    MArrayDataHandle arrayHandle = data.outputArrayValue(plug);
    MArrayDataBuilder arraybuilder = arrayHandle.builder();

    for(unsigned int i = 0; i < numElements; ++i){
      MDataHandle handle = arraybuilder.addElement(i);
      if(handle.numericType() == MFnNumericData::k3Float || handle.numericType() == MFnNumericData::kFloat){
        handle.set3Float(values[offset], values[offset+1], values[offset+2]);
        offset+=3;
      }else{
        handle.set3Double(values[offset], values[offset+1], values[offset+2]);
        offset+=3;
      }
    }

    arrayHandle.set(arraybuilder);
    arrayHandle.setAllClean();
  }
  else{
    MDataHandle handle = data.outputValue(plug);
    if(handle.numericType() == MFnNumericData::k3Float || handle.numericType() == MFnNumericData::kFloat){
      handle.set3Float(values[offset], values[offset+1], values[offset+2]);
    }else{
      handle.set3Double(values[offset], values[offset+1], values[offset+2]);
    }
  }
}

void dfgPortToPlug_mat44(
  unsigned argIndex,
  char const *argName,
  char const *argTypeName,
  FEC_DFGPortType argOutsidePortType,
  uint64_t argRawDataSize,
  FEC_DFGBindingVisitArgs_GetCB getCB,
  FEC_DFGBindingVisitArgs_GetRawCB getRawCB,
  void *getSetUD,
  MPlug &plug, 
  MDataBlock &data)
{
  uint64_t elementDataSize = sizeof(float) * 16;
  uint64_t numElements = argRawDataSize / elementDataSize;

  const float * values;
  getRawCB(getSetUD, (const void**)&values);

  FabricMayaProfilingEvent bracket("dfgPortToPlug_mat44");
  if(plug.isArray()){

    MArrayDataHandle arrayHandle = data.outputArrayValue(plug);
    MArrayDataBuilder arraybuilder = arrayHandle.builder();

    {
      FabricMayaProfilingEvent bracket("converting matrix array");
      unsigned int offset = 0;
      for(unsigned int i = 0; i < numElements; ++i){
        MDataHandle handle = arraybuilder.addElement(i);

        MMatrix mayaMat;
        Mat44ToMMatrix_data(&values[offset], mayaMat);
        offset += 16;

        handle.setMMatrix(mayaMat);
      }
    }

    arrayHandle.set(arraybuilder);
    arrayHandle.setAllClean();
  }
  else{
    MDataHandle handle = data.outputValue(plug);

    MMatrix mayaMat;
    {
      FabricMayaProfilingEvent bracket("converting matrix");
      Mat44ToMMatrix_data(values, mayaMat);
    }

    handle.setMMatrix(mayaMat);
  }
}

void dfgPortToPlug_PolygonMesh_singleMesh(MDataHandle handle, FabricCore::RTVal rtMesh)
{
  CORE_CATCH_BEGIN;

  unsigned int nbPoints   = 0;
  unsigned int nbPolygons = 0;
  unsigned int nbSamples  = 0;
  if(!rtMesh.isNullObject())
  {
    nbPoints   = rtMesh.callMethod("UInt64", "pointCount",         0, 0).getUInt64();
    nbPolygons = rtMesh.callMethod("UInt64", "polygonCount",       0, 0).getUInt64();
    nbSamples  = rtMesh.callMethod("UInt64", "polygonPointsCount", 0, 0).getUInt64();
  }

  MPointArray  mayaPoints;
  MVectorArray mayaNormals;
  MIntArray    mayaCounts;
  MIntArray    mayaIndices;

  #if MAYA_API_VERSION < 201500         // FE-5118 ("crash when saving scene with an empty polygon mesh")

  if (nbPoints < 3 || nbPolygons == 0)
  {
    // the rtMesh is either empty or has no polygons, so in order to
    // avoid a crash in Maya 2013 and 2014 we create a mesh with a
    // single triangle (and try to preserve the vertices, if any).

    if (nbPoints < 3)
    {
      // we only create the three vertices if there aren't enough.
      // (note: Maya correctly sets the vertices if at least one triangle is present).
      mayaPoints.setLength(3);
      mayaPoints[0] = MPoint(0, 0, 0, 0);
      mayaPoints[1] = MPoint(0, 0, 0, 0);
      mayaPoints[2] = MPoint(0, 0, 0, 0);
    }

    mayaCounts.setLength(1);
    mayaCounts[0] = 3;

    mayaIndices.setLength(3);
    mayaIndices[0] = 0;
    mayaIndices[1] = 1;
    mayaIndices[2] = 2;

    MFnMeshData meshDataFn;
    MObject meshObject;
    MFnMesh mesh;
    meshObject = meshDataFn.create();

    mesh.create( mayaPoints.length(), mayaCounts.length(), mayaPoints, mayaCounts, mayaIndices, meshObject );
    mesh.updateSurface();

    handle.set( meshObject );
    handle.setClean();
  }
  else

  #endif
  {
    mayaPoints.setLength(nbPoints);
    if(mayaPoints.length() > 0)
    {
      std::vector<FabricCore::RTVal> args(2);
      args[0] = FabricSplice::constructExternalArrayRTVal("Float64", mayaPoints.length() * 4, &mayaPoints[0]);
      args[1] = FabricSplice::constructUInt32RTVal(4); // components
      rtMesh.callMethod("", "getPointsAsExternalArray_d", 2, &args[0]);
    }

    mayaNormals.setLength(nbSamples);
    if(mayaNormals.length() > 0)
    {
      FabricCore::RTVal normalsVar = 
      FabricSplice::constructExternalArrayRTVal("Float64", mayaNormals.length() * 3, &mayaNormals[0]);
      rtMesh.callMethod("", "getNormalsAsExternalArray_d", 1, &normalsVar);
    }

    mayaCounts.setLength(nbPolygons);
    mayaIndices.setLength(nbSamples);
    if(mayaCounts.length() > 0 && mayaIndices.length() > 0)
    {
      std::vector<FabricCore::RTVal> args(2);
      args[0] = FabricSplice::constructExternalArrayRTVal("UInt32", mayaCounts.length(),  &mayaCounts[0]);
      args[1] = FabricSplice::constructExternalArrayRTVal("UInt32", mayaIndices.length(), &mayaIndices[0]);
      rtMesh.callMethod("", "getTopologyAsCountsIndicesExternalArrays", 2, &args[0]);
    }

    MFnMeshData meshDataFn;
    MObject meshObject;
    MFnMesh mesh;
    meshObject = meshDataFn.create();

    MIntArray normalFace, normalVertex;
    normalFace.setLength( mayaIndices.length() );
    normalVertex.setLength( mayaIndices.length() );

    int face = 0;
    int vertex = 0;
    int offset = 0;

    for( unsigned int i = 0; i < mayaIndices.length(); i++ ) {
      normalFace[i] = face;
      normalVertex[i] = mayaIndices[offset + vertex];
      vertex++;

      if( vertex == mayaCounts[face] ) {
        offset += mayaCounts[face];
        face++;
        vertex = 0;
      }
    }

    mesh.create( mayaPoints.length(), mayaCounts.length(), mayaPoints, mayaCounts, mayaIndices, meshObject );
    mesh.updateSurface();
    mayaPoints.clear();
    mesh.setFaceVertexNormals( mayaNormals, normalFace, normalVertex );

    if( !rtMesh.isNullObject() ) {

      if( rtMesh.callMethod( "Boolean", "hasUVs", 0, 0 ).getBoolean() ) {
        MFloatArray values( nbSamples * 2 );
        std::vector<FabricCore::RTVal> args( 2 );
        args[0] = FabricSplice::constructExternalArrayRTVal( "Float32", values.length(), &values[0] );
        args[1] = FabricSplice::constructUInt32RTVal( 2 ); // components
        rtMesh.callMethod( "", "getUVsAsExternalArray", 2, &args[0] );

        MFloatArray u, v;
        u.setLength( nbSamples );
        v.setLength( nbSamples );
        unsigned int offset = 0;
        for( unsigned int i = 0; i < u.length(); i++ ) {
          u[i] = values[offset++];
          v[i] = values[offset++];
        }
        values.clear();
        MString setName( "map1" );
        mesh.createUVSet( setName );
        mesh.setCurrentUVSetName( setName );

        mesh.setUVs( u, v );

        MIntArray indices( nbSamples );
        for( unsigned int i = 0; i < nbSamples; i++ )
          indices[i] = i;
        mesh.assignUVs( mayaCounts, indices );
      }

      if( rtMesh.callMethod( "Boolean", "hasVertexColors", 0, 0 ).getBoolean() ) {
        MColorArray values( nbSamples );
        std::vector<FabricCore::RTVal> args( 2 );
        args[0] = FabricSplice::constructExternalArrayRTVal( "Float32", values.length() * 4, &values[0] );
        args[1] = FabricSplice::constructUInt32RTVal( 4 ); // components
        rtMesh.callMethod( "", "getVertexColorsAsExternalArray", 2, &args[0] );

        MString setName( "colorSet" );
        mesh.createColorSet( setName );
        mesh.setCurrentColorSetName( setName );

        MIntArray face( nbSamples );

        unsigned int offset = 0;
        for( unsigned int i = 0; i < mayaCounts.length(); i++ ) {
          for( int j = 0; j < mayaCounts[i]; j++, offset++ ) {
            face[offset] = i;
          }
        }

        mesh.setFaceVertexColors( values, face, mayaIndices );
      }
    }

    handle.set( meshObject );
    handle.setClean();
  }

  CORE_CATCH_END;
}

void dfgPortToPlug_PolygonMesh(
  unsigned argIndex,
  char const *argName,
  char const *argTypeName,
  FEC_DFGPortType argOutsidePortType,
  uint64_t argRawDataSize,
  FEC_DFGBindingVisitArgs_GetCB getCB,
  FEC_DFGBindingVisitArgs_GetRawCB getRawCB,
  void *getSetUD,
  MPlug &plug, 
  MDataBlock &data)
{
  try
  {
    FabricCore::RTVal rtVal(getCB(getSetUD));
    if(plug.isArray())
    {
      MArrayDataHandle arrayHandle = data.outputArrayValue(plug);
      MArrayDataBuilder arraybuilder = arrayHandle.builder();

      unsigned int elements = rtVal.getArraySize();
      for(unsigned int i = 0; i < elements; ++i)
        dfgPortToPlug_PolygonMesh_singleMesh(arraybuilder.addElement(i), rtVal.getArrayElement(i));

      arrayHandle.set(arraybuilder);
      arrayHandle.setAllClean();
    }
    else
    {
      MDataHandle handle = data.outputValue(plug.attribute());
      dfgPortToPlug_PolygonMesh_singleMesh(handle, rtVal);
    }
  }
  catch(FabricCore::Exception e)
  {
    mayaLogErrorFunc(e.getDesc_cstr());
    return;
  }
  catch(FabricSplice::Exception e)
  {
    mayaLogErrorFunc(e.what());
    return;
  }
}

void dfgPortToPlug_Lines_singleLines(MDataHandle handle, FabricCore::RTVal rtVal)
{
  CORE_CATCH_BEGIN;

  unsigned int nbPoints = 0;
  unsigned int nbSegments = 0;
  if(!rtVal.isNullObject())
  {
    nbPoints   = rtVal.callMethod("UInt64", "pointCount", 0, 0).getUInt64();
    nbSegments = rtVal.callMethod("UInt64", "lineCount",  0, 0).getUInt64();
  }

  MPointArray mayaPoints(nbPoints);
  std::vector<double> mayaDoubles(nbPoints * 3);
  std::vector<uint32_t> mayaIndices(nbSegments*2);
  MDoubleArray mayaKnots(nbPoints);

  if(nbPoints > 0)
  {
    FabricCore::RTVal mayaDoublesVal = FabricSplice::constructExternalArrayRTVal("Float64", mayaDoubles.size(), &mayaDoubles[0]);
    rtVal.callMethod("", "_getPositionsAsExternalArray_d", 1, &mayaDoublesVal);
  }

  if(nbSegments > 0)
  {
    FabricCore::RTVal mayaIndicesVal = FabricSplice::constructExternalArrayRTVal("UInt32", mayaIndices.size(), &mayaIndices[0]);
    rtVal.callMethod("", "_getTopologyAsExternalArray", 1, &mayaIndicesVal);
  }

  size_t offset = 0;
  for(unsigned int i=0;i<nbPoints;i++) {
    mayaPoints[i].x = mayaDoubles[offset++];
    mayaPoints[i].y = mayaDoubles[offset++];
    mayaPoints[i].z = mayaDoubles[offset++];
    mayaPoints[i].w = 1.0;
    mayaKnots[i] = (double)i;
  }

  MFnNurbsCurveData curveDataFn;
  MObject curveObject;
  MFnNurbsCurve curve;
  curveObject = curveDataFn.create();

  MFnNurbsCurve::Form form = MFnNurbsCurve::kOpen;
  if(mayaIndices.size() > 1)
  {
    if(mayaIndices[0] == mayaIndices[mayaIndices.size()-1])
      form = MFnNurbsCurve::kClosed; 
  }

  curve.create(
    mayaPoints, mayaKnots, 1, 
    form,
    false,
    false,
    curveObject);

  handle.set(curveObject);
  handle.setClean();

  CORE_CATCH_END;
}

void dfgPortToPlug_Lines(
  unsigned argIndex,
  char const *argName,
  char const *argTypeName,
  FEC_DFGPortType argOutsidePortType,
  uint64_t argRawDataSize,
  FEC_DFGBindingVisitArgs_GetCB getCB,
  FEC_DFGBindingVisitArgs_GetRawCB getRawCB,
  void *getSetUD,
  MPlug &plug, MDataBlock &data)
{
  try
  {
    FabricCore::RTVal rtVal(getCB(getSetUD));
    if(plug.isArray())
    {
      MArrayDataHandle arrayHandle = data.outputArrayValue(plug);
      MArrayDataBuilder arraybuilder = arrayHandle.builder();

      unsigned int elements = rtVal.getArraySize();
      for(unsigned int i = 0; i < elements; ++i)
        dfgPortToPlug_Lines_singleLines(arraybuilder.addElement(i), rtVal.getArrayElement(i));

      arrayHandle.set(arraybuilder);
      arrayHandle.setAllClean();
    }
    else
    {
      MDataHandle handle = data.outputValue(plug.attribute());
      dfgPortToPlug_Lines_singleLines(handle, rtVal);
    }
  }
  catch(FabricCore::Exception e)
  {
    mayaLogErrorFunc(e.getDesc_cstr());
    return;
  }
  catch(FabricSplice::Exception e)
  {
    mayaLogErrorFunc(e.what());
    return;
  }
}

void dfgPortToPlug_spliceMayaData(
  unsigned argIndex,
  char const *argName,
  char const *argTypeName,
  FEC_DFGPortType argOutsidePortType,
  uint64_t argRawDataSize,
  FEC_DFGBindingVisitArgs_GetCB getCB,
  FEC_DFGBindingVisitArgs_GetRawCB getRawCB,
  void *getSetUD,
  MPlug &plug, MDataBlock &data)
{
  MStatus status;
  try{
    FabricCore::RTVal rtVal(getCB(getSetUD));

    if(!plug.isArray()){
      MFnFabricSpliceMayaData mfnpd;
      MObject spliceMayaDataObj = mfnpd.create(&status);
      FabricSpliceMayaData *spliceMayaData =(FabricSpliceMayaData *)mfnpd.data();
      spliceMayaData->setRTVal(rtVal);

      MDataHandle handle = data.outputValue(plug);
      handle.set(spliceMayaDataObj);
    } else {
      if(!rtVal.isArray())
        return;

      MArrayDataHandle arrayHandle = data.outputArrayValue(plug);
      MArrayDataBuilder arraybuilder = arrayHandle.builder();

      unsigned int elements = rtVal.getArraySize();
      for(unsigned int i = 0; i < elements; ++i)
      {
        MDataHandle handle = arraybuilder.addElement(i);
        MFnFabricSpliceMayaData mfnpd;
        MObject spliceMayaDataObj = mfnpd.create(&status);
        FabricSpliceMayaData *spliceMayaData =(FabricSpliceMayaData *)mfnpd.data();
        spliceMayaData->setRTVal(rtVal.getArrayElement(i));
        handle.set(spliceMayaDataObj);
      }

      arrayHandle.set(arraybuilder);
      arrayHandle.setAllClean();
    }
  }
  catch(FabricCore::Exception e)
  {
    mayaLogErrorFunc(e.getDesc_cstr());
    return;
  }
}

DFGPlugToArgFunc getDFGPlugToArgFunc(const FTL::StrRef &dataType)
{
  if(dataType == FTL_STR("Boolean"))               return dfgPlugToPort_bool;

  if (dataType == FTL_STR("Scalar"))               return dfgPlugToPort_scalar;
  if (dataType == FTL_STR("Float32"))              return dfgPlugToPort_scalar;
  if (dataType == FTL_STR("Float64"))              return dfgPlugToPort_scalar;

  if (dataType == FTL_STR("Integer"))              return dfgPlugToPort_integer;
  if (dataType == FTL_STR("SInt8"))                return dfgPlugToPort_integer;
  if (dataType == FTL_STR("SInt16"))               return dfgPlugToPort_integer;
  if (dataType == FTL_STR("SInt32"))               return dfgPlugToPort_integer;
  if (dataType == FTL_STR("SInt64"))               return dfgPlugToPort_integer;

  if (dataType == FTL_STR("Byte"))                 return dfgPlugToPort_integer;
  if (dataType == FTL_STR("UInt8"))                return dfgPlugToPort_integer;
  if (dataType == FTL_STR("UInt16"))               return dfgPlugToPort_integer;
  if (dataType == FTL_STR("Count"))                return dfgPlugToPort_integer;
  if (dataType == FTL_STR("Index"))                return dfgPlugToPort_integer;
  if (dataType == FTL_STR("Size"))                 return dfgPlugToPort_integer;
  if (dataType == FTL_STR("UInt32"))               return dfgPlugToPort_integer;
  if (dataType == FTL_STR("DataSize"))             return dfgPlugToPort_integer;
  if (dataType == FTL_STR("UInt64"))               return dfgPlugToPort_integer;

  if (dataType == FTL_STR("String"))               return dfgPlugToPort_string;

  if (dataType == FTL_STR("Vec3"))                 return dfgPlugToPort_vec3;

  if (dataType == FTL_STR("Euler"))                return dfgPlugToPort_euler;

  if (dataType == FTL_STR("Mat44"))                return dfgPlugToPort_mat44;

  if (dataType == FTL_STR("Color"))                return dfgPlugToPort_color;

  if (dataType == FTL_STR("PolygonMesh"))          return dfgPlugToPort_PolygonMesh;

  if (dataType == FTL_STR("Lines"))                return dfgPlugToPort_Lines;

  if (dataType == FTL_STR("KeyframeTrack"))        return dfgPlugToPort_KeyframeTrack;

  if (dataType == FTL_STR("SpliceMayaData"))      return dfgPlugToPort_spliceMayaData;

  if(dataType == FTL_STR("CompoundParam"))         return dfgPlugToPort_compound;
  if(dataType == FTL_STR("CompoundArrayParam"))    return dfgPlugToPort_compoundArray;

  return NULL;  
}

DFGArgToPlugFunc getDFGArgToPlugFunc(const FTL::StrRef &dataType)
{
  if(dataType == FTL_STR("Boolean"))               return dfgPortToPlug_bool;

  if (dataType == FTL_STR("Scalar"))               return dfgPortToPlug_scalar;
  if (dataType == FTL_STR("Float32"))              return dfgPortToPlug_scalar;
  if (dataType == FTL_STR("Float64"))              return dfgPortToPlug_scalar;

  if (dataType == FTL_STR("Integer"))              return dfgPortToPlug_integer;
  if (dataType == FTL_STR("SInt8"))                return dfgPortToPlug_integer;
  if (dataType == FTL_STR("SInt16"))               return dfgPortToPlug_integer;
  if (dataType == FTL_STR("SInt32"))               return dfgPortToPlug_integer;
  if (dataType == FTL_STR("SInt64"))               return dfgPortToPlug_integer;

  if (dataType == FTL_STR("Byte"))                 return dfgPortToPlug_integer;
  if (dataType == FTL_STR("UInt8"))                return dfgPortToPlug_integer;
  if (dataType == FTL_STR("UInt16"))               return dfgPortToPlug_integer;
  if (dataType == FTL_STR("Count"))                return dfgPortToPlug_integer;
  if (dataType == FTL_STR("Index"))                return dfgPortToPlug_integer;
  if (dataType == FTL_STR("Size"))                 return dfgPortToPlug_integer;
  if (dataType == FTL_STR("UInt32"))               return dfgPortToPlug_integer;
  if (dataType == FTL_STR("DataSize"))             return dfgPortToPlug_integer;
  if (dataType == FTL_STR("UInt64"))               return dfgPortToPlug_integer;

  if (dataType == FTL_STR("String"))               return dfgPortToPlug_string;

  if (dataType == FTL_STR("Vec3"))                 return dfgPortToPlug_vec3;

  if (dataType == FTL_STR("Euler"))                return dfgPortToPlug_euler;

  if (dataType == FTL_STR("Mat44"))                return dfgPortToPlug_mat44;

  if (dataType == FTL_STR("Color"))                return dfgPortToPlug_color;

  if (dataType == FTL_STR("PolygonMesh"))          return dfgPortToPlug_PolygonMesh;

  if (dataType == FTL_STR("Lines"))                return dfgPortToPlug_Lines;

  if (dataType == FTL_STR("SpliceMayaData"))      return dfgPortToPlug_spliceMayaData;

  if(dataType == FTL_STR("CompoundParam"))         return dfgPortToPlug_compound;

  return NULL;  
}
