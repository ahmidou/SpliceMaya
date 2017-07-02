//
// Copyright (c) 2010-2017 Fabric Software Inc. All rights reserved.
//

#ifndef __FABRIC_MAYA_CONVERSION__
#define __FABRIC_MAYA_CONVERSION__

#include <QString>
#include <FabricSplice.h>
#include "FabricSpliceHelpers.h"

#include <FabricCore.h>
#include <maya/MPoint.h>
#include <maya/MObject.h>
#include <maya/MString.h>
#include <maya/MVector.h>
#include <maya/MMatrix.h>
#include <maya/MFnMesh.h>
#include <maya/MIntArray.h>
#include <maya/MFloatArray.h>
#include <maya/MPointArray.h>
#include <maya/MFloatPoint.h>
#include <maya/MDoubleArray.h>
#include <maya/MFloatVector.h>
#include <maya/MVectorArray.h>
#include <maya/MFloatMatrix.h>
#include <maya/MMatrixArray.h>
#include <maya/MFnNurbsCurve.h>
#include <maya/MFloatPointArray.h>
#include <maya/MFloatVectorArray.h>

#define CONVERSION_CATCH_BEGIN try {

#define CONVERSION_CATCH_END(methodName) } \
  catch (FabricCore::Exception &e) { \
    mayaLogErrorFunc( \
      QString( \
        QString(methodName) + " " + QString(e.getDesc_cstr()) \
        ).toUtf8().constData() \
      ); \
  }

namespace FabricMaya {
namespace Conversion {

// ***************** Maya to RTVal ***************** // 
// template
template<typename FPTy, typename ArrayTy>
FabricCore::RTVal MayaToRTValArray(const char *type, ArrayTy const&array) {

  FabricCore::RTVal rtVal;

  CONVERSION_CATCH_BEGIN;

  rtVal = FabricSplice::constructVariableArrayRTVal(type);
  rtVal.setArraySize(array.length());

  FabricCore::RTVal dataRtVal = rtVal.callMethod("Data", "data", 0, 0);
  FPTy* data = (FPTy*)dataRtVal.getData();
  array.get(data);

  CONVERSION_CATCH_END(
    QString(
      "FabricConversion::MayaToRTValArray::" + QString(type)
      ).toUtf8().constData());

  return rtVal;
}
 
template<typename FPTy, typename VectorTy> 
FabricCore::RTVal MayaToRTValVec(unsigned int dim, const char *type, VectorTy const&vector) {

  FabricCore::RTVal rtVal;

  CONVERSION_CATCH_BEGIN;

  rtVal = FabricSplice::constructRTVal(type);
  FabricCore::RTVal dataRtVal = rtVal.callMethod("Data", "data", 0, 0);
  FPTy* data = (FPTy*)dataRtVal.getData();
 
  for(unsigned int i=0; i<dim; ++i)
    data[i] = (FPTy)vector[i];
 
  CONVERSION_CATCH_END(
    QString(
      "FabricConversion::MayaToRTValVec::" + QString(type)
      ).toUtf8().constData());

  return rtVal;
}
 
template<typename FPTy, typename VectorArrayTy>
FabricCore::RTVal MayaToRTValVecArray(unsigned int dim, const char *type, VectorArrayTy const&array) {
  FabricCore::RTVal rtVal;

  CONVERSION_CATCH_BEGIN;

  rtVal = FabricSplice::constructVariableArrayRTVal(type);
  rtVal.setArraySize(array.length());

  for(unsigned int i = 0; i < array.length(); ++i)
    rtVal.setArrayElement(i, MayaToRTValVec<FPTy>(dim, type, array[i]));
 
  CONVERSION_CATCH_END(
    QString(
      "FabricConversion::MayaToRTValVecArray::" + QString(type)
      ).toUtf8().constData());

  return rtVal;
}
 
template<typename FPTy, typename MatrixTy>
FabricCore::RTVal MayaToRTValMatrix(const char *type, MatrixTy const&matrix) {
  FabricCore::RTVal rtVal;

  CONVERSION_CATCH_BEGIN;

  rtVal = FabricSplice::constructRTVal(type);
  FabricCore::RTVal dataRtVal = rtVal.callMethod("Data", "data", 0, 0);
  FPTy* data = (FPTy*)dataRtVal.getData();
  
  for(unsigned int i=0; i<4; ++i)
    for(unsigned int j=0; j<4; ++j)
      data[4*i + j] = (FPTy)matrix[j][i];
 
  CONVERSION_CATCH_END(
    QString(
      "FabricConversion::MayaToRTValMatrix::" + QString(type)
      ).toUtf8().constData());

  return rtVal;
}

template<typename FPTy, typename MatrixArrayTy>
FabricCore::RTVal MayaToRTValMatrixArray(const char *type, MatrixArrayTy const&array) {
  FabricCore::RTVal rtVal;

  CONVERSION_CATCH_BEGIN;

  rtVal = FabricSplice::constructVariableArrayRTVal(type);
  rtVal.setArraySize(array.length());

  for(unsigned int i = 0; i < array.length(); ++i)
    rtVal.setArrayElement(i, MayaToRTValMatrix<FPTy>(type, array[i]));
 
  CONVERSION_CATCH_END(
    QString(
      "FabricConversion::MayaToRTValMatrixArray::" + QString(type)
      ).toUtf8().constData());

  return rtVal;
}
 
// String
inline FabricCore::RTVal StringToMString(MString const&str);

inline FabricCore::RTVal MStringArrayToStringArray(MStringArray const&strArray);

// Array
inline FabricCore::RTVal MInt32ArrayToSIntArray(MIntArray const&array) { return MayaToRTValArray<int>("SInt32", array); } 

inline FabricCore::RTVal MFloatArrayToFloat32Array(MFloatArray const&array) { return MayaToRTValArray<float>("Float32", array); } 

inline FabricCore::RTVal MFloatArrayToFloat64Array(MFloatArray const&array) { return MayaToRTValArray<double>("Float64", array); } 

inline FabricCore::RTVal MDoubleArrayToFloat32Array(MDoubleArray const&array) { return MayaToRTValArray<float>("float", array); } 

inline FabricCore::RTVal MDoubleArrayToFloat64Array(MDoubleArray const&array) { return MayaToRTValArray<double>("Float64", array); } 

// Vec2
inline FabricCore::RTVal MFloatVectorToVec2_i(MFloatVector const&vector) { return MayaToRTValVec<int>(2, "Vec2_i", vector); }

inline FabricCore::RTVal MFloatVectorToVec2(MFloatVector const&vector) { return MayaToRTValVec<float>(2, "Vec2", vector); }

inline FabricCore::RTVal MVectorToVec2(MVector const&vector) { return MayaToRTValVec<float>(2, "Vec2", vector); }

inline FabricCore::RTVal MVectorToVec2_d(MVector const& vector) { return MayaToRTValVec<double>(2, "Vec2_d", vector); }

inline FabricCore::RTVal MFloatVectorArrayToVec2_iArray(MFloatVectorArray const& array) { return MayaToRTValVecArray<int>(2, "Vec2_i", array); }

inline FabricCore::RTVal MFloatVectorArrayToVec2Array(MFloatVectorArray const& array) { return MayaToRTValVecArray<float>(2, "Vec2", array); }

inline FabricCore::RTVal MVectorArrayToVec2Array(MVectorArray const& array) { return MayaToRTValVecArray<float>(2, "Vec2", array); }

inline FabricCore::RTVal MVectorArrayToVec2_dArray(MVectorArray const& array) { return MayaToRTValVecArray<double>(2, "Vec2_d", array); }

// Vec3
inline FabricCore::RTVal MFloatVectorToVec3_i(MFloatVector const&vector) { return MayaToRTValVec<int>(3, "Vec3_i", vector); }

inline FabricCore::RTVal MFloatVectorToVec3(MFloatVector const&vector) { return MayaToRTValVec<float>(3, "Vec3", vector); }

inline FabricCore::RTVal MVectorToVec3(MVector const&vector) { return MayaToRTValVec<float>(3, "Vec3", vector); }

inline FabricCore::RTVal MVectorToVec3_d(MVector const& vector) { return MayaToRTValVec<double>(3, "Vec3_d", vector); }

inline FabricCore::RTVal MFloatVectorArrayToVec3_iArray(MFloatVectorArray const& array) { return MayaToRTValVecArray<int>(3, "Vec3_i", array); }

inline FabricCore::RTVal MFloatVectorArrayToVec3Array(MFloatVectorArray const& array) { return MayaToRTValVecArray<float>(3, "Vec3", array); }

inline FabricCore::RTVal MVectorArrayToVec3Array(MVectorArray const& array) { return MayaToRTValVecArray<float>(3, "Vec3", array); }

inline FabricCore::RTVal MVectorArrayToVec3_dArray(MVectorArray const& array) { return MayaToRTValVecArray<double>(3, "Vec3_d", array); }

// Vec4
inline FabricCore::RTVal MFloatPointToVec4_i(MFloatPoint const&point) { return MayaToRTValVec<int>(4, "Vec4_i", point); }

inline FabricCore::RTVal MFloatPointToVec4(MFloatPoint const&point) { return MayaToRTValVec<float>(4, "Vec4", point); }

inline FabricCore::RTVal MPointToVec4(MPoint const&point) { return MayaToRTValVec<float>(4, "Vec4", point); }

inline FabricCore::RTVal MPointToVec4_d(MPoint const& point) { return MayaToRTValVec<double>(4, "Vec4_d", point); }

inline FabricCore::RTVal MFloatPointArrayToVec4_iArray(MFloatPointArray const& array) { return MayaToRTValVecArray<int>(4, "Vec4_i", array); }

inline FabricCore::RTVal MFloatPointArrayToVec4Array(MFloatPointArray const& array) { return MayaToRTValVecArray<float>(4, "Vec4", array); }

inline FabricCore::RTVal MPointArrayToVec4Array(MPointArray const& array) { return MayaToRTValVecArray<float>(4, "Vec4", array); }

inline FabricCore::RTVal MPointArrayToVec4_dArray(MPointArray const& array) { return MayaToRTValVecArray<double>(4, "Vec4_d", array); }

// Color
inline FabricCore::RTVal MFloatVectorToColor(MFloatVector const&vector) { return MayaToRTValVec<float>(3, "Color", vector); }

inline FabricCore::RTVal MVectorToColor(MVector const&vector) { return MayaToRTValVec<float>(3, "Color", vector); }
 
inline FabricCore::RTVal MFloatVectorArrayToColorArray(MFloatVectorArray const& array) { return MayaToRTValVecArray<float>(3, "Color", array); }

inline FabricCore::RTVal MVectorArrayToColorArray(MVectorArray const& array) { return MayaToRTValVecArray<float>(3, "Color", array); }

inline FabricCore::RTVal MFloatPointToColor(MFloatPoint const&point) { return MayaToRTValVec<float>(4, "Color", point); }

inline FabricCore::RTVal MPointToColor(MPoint const&point) { return MayaToRTValVec<float>(4, "Color", point); }
 
inline FabricCore::RTVal MFloatPointArrayToColorArray(MFloatPointArray const& array) { return MayaToRTValVecArray<float>(4, "Color", array); }

inline FabricCore::RTVal MPointArrayToColorArray(MPointArray const& array) { return MayaToRTValVecArray<float>(4, "Color", array); }

// Euler
inline FabricCore::RTVal MFloatVectorToEuler(MFloatVector const&vector) { return MayaToRTValVec<float>(3, "Euler", vector); }

inline FabricCore::RTVal MVectorToEuler(MVector const&vector) { return MayaToRTValVec<float>(3, "Euler", vector); }
 
inline FabricCore::RTVal MVectorToEuler_d(MVector const&vector) { return MayaToRTValVec<double>(3, "Euler_d", vector); }

inline FabricCore::RTVal MFloatVectorArrayToEulerArray(MFloatVectorArray const& array) { return MayaToRTValVecArray<float>(3, "Euler", array); }

inline FabricCore::RTVal MVectorArrayToEulerArray(MVectorArray const& array) { return MayaToRTValVecArray<float>(3, "Euler", array); }

inline FabricCore::RTVal MVectorArrayToEuler_dArray(MVectorArray const& array) { return MayaToRTValVecArray<double>(3, "Euler_d", array); }

inline FabricCore::RTVal MFloatPointToEuler(MFloatPoint const&point) { return MayaToRTValVec<float>(4, "Euler", point); }
 
// Matrix 
inline FabricCore::RTVal MFloatMatrixToMat44(MFloatMatrix const&matrix) { return MayaToRTValMatrix<float>("Mat44", matrix); }

inline FabricCore::RTVal MMatrixToMat44(MMatrix const&matrix) { return MayaToRTValMatrix<float>("Mat44", matrix); }

inline FabricCore::RTVal MFloatMatrixToMat44_d(MFloatMatrix const&matrix) { return MayaToRTValMatrix<double>("Mat44_d", matrix); }

inline FabricCore::RTVal MMatrixToMat44_d(MMatrix const&matrix) { return MayaToRTValMatrix<double>("Mat44_d", matrix); }

inline FabricCore::RTVal MMatrixToMat44Array(MMatrixArray const&array) { return MayaToRTValMatrixArray<float>("Mat44", array); }
 
inline FabricCore::RTVal MMatrixToMat44_dArray(MMatrixArray const&array) { return MayaToRTValMatrixArray<double>("Mat44_d", array); }

// Xfo 
inline FabricCore::RTVal MFloatMatrixToXfo(MFloatMatrix const&matrix) { FabricCore::RTVal matrixVal = MFloatMatrixToMat44(matrix); return FabricSplice::constructRTVal("Xfo", 1, &matrixVal); }

inline FabricCore::RTVal MMatrixToXfo(MMatrix const&matrix) { FabricCore::RTVal matrixVal = MMatrixToMat44(matrix); return FabricSplice::constructRTVal("Xfo", 1, &matrixVal); }
 
inline FabricCore::RTVal MFloatMatrixToXfo_d(MFloatMatrix const&matrix) { FabricCore::RTVal matrixVal = MFloatMatrixToMat44_d(matrix); return FabricSplice::constructRTVal("Xfo_d", 1, &matrixVal); }

inline FabricCore::RTVal MMatrixToXfo_d(MMatrix const&matrix) { FabricCore::RTVal matrixVal = MMatrixToMat44_d(matrix); return FabricSplice::constructRTVal("Xfo_d", 1, &matrixVal); }

inline FabricCore::RTVal MMatrixToXfoArray(MMatrixArray const&array);

inline FabricCore::RTVal MMatrixToXfo_dArray(MMatrixArray const&array);

// Geometry 
/// \internal
inline void MFnMeshToMesch_single(MFnMesh &mesh, FabricCore::RTVal rtMesh);

inline FabricCore::RTVal MFnMeshToMesch(MFnMesh &mesh);

// ***************** Maya to RTVal ***************** // 


// ***************** RTVal to Maya ***************** // 
// template
template<typename ArrayTy, typename FPTy>
ArrayTy RTValToMayaArray(const char *type, FabricCore::RTVal rtVal) {
  
  QString arrayType = QString(type + QString("[]") );
  assert( rtVal.hasType( arrayType.toUtf8().constData() ) );
  
  ArrayTy array;

  CONVERSION_CATCH_BEGIN;

  FabricCore::RTVal dataRtVal = rtVal.callMethod("Data", "data", 0, 0);
  FPTy *data = (FPTy*)dataRtVal.getData();
  array = ArrayTy(data, rtVal.getArraySize());

  CONVERSION_CATCH_END(
    QString(
      "FabricConversion::RTValToMayaArray::" + arrayType
      ).toUtf8().constData());

  return array;
}

template<typename VectorTy, typename FPTy>
VectorTy RTValToMayaVec(unsigned int dim, const char *type, FabricCore::RTVal rtVal) {
  
  assert( rtVal.hasType( type ) );

  VectorTy vector;

  CONVERSION_CATCH_BEGIN;

  FabricCore::RTVal dataRtVal = rtVal.callMethod("Data", "data", 0, 0);
  FPTy *data = (FPTy*)dataRtVal.getData();
  
  if(dim == 2)
    vector = VectorTy( double(data[0]), double(data[1]), double(0) );

  if(dim == 3)
    vector = VectorTy( double(data[0]), double(data[1]), double(data[2]) );
 
  CONVERSION_CATCH_END(
    QString(
      "FabricConversion::RTValToMayaVec::" + QString(type)
      ).toUtf8().constData());

  return vector;
}

template<typename VectorTy, typename FPTy>
VectorTy RTValToMayaPoint(unsigned int dim, const char *type, FabricCore::RTVal rtVal) {
  
  assert( rtVal.hasType( type ) );

  VectorTy point;

  CONVERSION_CATCH_BEGIN;

  FabricCore::RTVal dataRtVal = rtVal.callMethod("Data", "data", 0, 0);
  FPTy *data = (FPTy*)dataRtVal.getData();
  
  if(dim == 2)
    point = VectorTy( double(data[0]), double(data[1]), double(0), double(0) );

  if(dim == 3)
    point = VectorTy( double(data[0]), double(data[1]), double(data[2]), double(0) );

  if(dim == 4)
    point = VectorTy( double(data[0]), double(data[1]), double(data[2]), double(data[3]) );

  CONVERSION_CATCH_END(
    QString(
      "FabricConversion::RTValToMayaPoint::" + QString(type)
      ).toUtf8().constData());

  return point;
}

template<typename VectorArrayTy, typename VectorTy, typename FPTy>
VectorArrayTy RTValToMayaVecArray(unsigned int dim, const char *type, FabricCore::RTVal rtVal) {

  QString arrayType = QString(type + QString("[]") );
  assert( rtVal.hasType( arrayType.toUtf8().constData() ) );

  VectorArrayTy vectors;

  CONVERSION_CATCH_BEGIN;

  unsigned int elements = rtVal.isNullObject() ? 0 : rtVal.getArraySize();

  vectors.setLength(elements);
  for(unsigned int i = 0; i < elements; ++i)
    vectors.set(RTValToMayaVec<VectorTy, FPTy>(dim, type, rtVal.getArrayElementRef(i)), i);
 
  CONVERSION_CATCH_END(
    QString(
      "FabricConversion::RTValToMayaVecArray::" + arrayType
      ).toUtf8().constData());

  return vectors;
}

template<typename PointArrayTy, typename PointTy, typename FPTy>
PointArrayTy RTValToMayaPointArray(unsigned int dim, const char *type, FabricCore::RTVal rtVal) {

  QString arrayType = QString(type + QString("[]") );
  assert( rtVal.hasType( arrayType.toUtf8().constData() ) );

  PointArrayTy points;

  CONVERSION_CATCH_BEGIN;

  unsigned int elements = rtVal.isNullObject() ? 0 : rtVal.getArraySize();

  points.setLength(elements);
  for(unsigned int i = 0; i < elements; ++i)
    points.set(RTValToMayaPoint<PointTy, FPTy>(dim, type, rtVal.getArrayElementRef(i)), i);
 
  CONVERSION_CATCH_END(
    QString(
      "FabricConversion::RTValToMayaPointArray::" + arrayType
      ).toUtf8().constData());

  return points;
}

template<typename MatrixTy, typename FPTy>
MatrixTy RTValToMayaMatrix(const char *type, FabricCore::RTVal rtVal) {
  
  assert( rtVal.hasType( type ) );

  MatrixTy matrix;

  CONVERSION_CATCH_BEGIN;

  FabricCore::RTVal dataRtVal = rtVal.callMethod("Data", "data", 0, 0);
  FPTy *data = (FPTy*)dataRtVal.getData();
  
  FPTy vals[4][4] = {
    { data[0], data[4], data[8],  data[12] },
    { data[1], data[5], data[9],  data[13] },
    { data[2], data[6], data[10], data[14] },
    { data[3], data[7], data[11], data[15] }
  };

  matrix = MatrixTy(vals);
 
  CONVERSION_CATCH_END(
    QString(
      "FabricConversion::RTValToMayaMatrix::" + QString(type)
      ).toUtf8().constData());

  return matrix;
}

template<typename MatrixArrayTy, typename MatrixTy, typename FPTy>
MatrixArrayTy RTValToMayaMatrixArray(const char *type, FabricCore::RTVal rtVal) {

  QString arrayType = QString(type + QString("[]") );
  assert( rtVal.hasType( arrayType.toUtf8().constData() ) );

  MatrixArrayTy matrices;

  CONVERSION_CATCH_BEGIN;

  unsigned int elements = rtVal.isNullObject() ? 0 : rtVal.getArraySize();

  matrices.setLength(elements);
  for(unsigned int i = 0; i < elements; ++i)
    matrices.set(RTValToMayaMatrix<MatrixTy, FPTy>(type, rtVal.getArrayElementRef(i)), i);
 
  CONVERSION_CATCH_END(
    QString(
      "FabricConversion::RTValToMayaMatrixArray::" + arrayType
      ).toUtf8().constData());

  return matrices;
}

// String
inline MString StringToMString(FabricCore::RTVal rtVal);

inline MStringArray StringArrayToMStringArray(FabricCore::RTVal rtVal);

// Array
inline MIntArray SInt32ArrayToMIntArray(FabricCore::RTVal rtVal) { return RTValToMayaArray<MIntArray, int>("SInt32", rtVal); } 

inline MIntArray UInt32ArrayToMIntArray(FabricCore::RTVal rtVal) { return RTValToMayaArray<MIntArray, int>("UInt32", rtVal); } 

inline MFloatArray Float32ArrayToMFloatArray(FabricCore::RTVal rtVal) { return RTValToMayaArray<MFloatArray, float>("Float32", rtVal); } 

inline MFloatArray Float64ArrayToMFloatArray(FabricCore::RTVal rtVal) { return RTValToMayaArray<MFloatArray, double>("Float64", rtVal); } 

inline MDoubleArray Float32ArrayToMDoubleArray(FabricCore::RTVal rtVal) { return RTValToMayaArray<MDoubleArray, float>("Float32", rtVal); } 

inline MDoubleArray Float64ArrayToMDoubleArray(FabricCore::RTVal rtVal) { return RTValToMayaArray<MDoubleArray, double>("Float64", rtVal); } 

// Vec2
inline MFloatVector Vec2_iToMFloatVector(FabricCore::RTVal rtVal) { return RTValToMayaVec<MFloatVector, int>(2, "Vec2_i", rtVal); }

inline MFloatVector Vec2ToMFloatVector(FabricCore::RTVal rtVal) { return RTValToMayaVec<MFloatVector, float>(2, "Vec2", rtVal); }

inline MVector Vec2ToMVector(FabricCore::RTVal rtVal) { return RTValToMayaVec<MVector, float>(2, "Vec2", rtVal); }
 
inline MVector Vec2_dToMVector(FabricCore::RTVal rtVal) { return RTValToMayaVec<MVector, double>(2, "Vec2_d", rtVal); }

inline MFloatVectorArray Vec2_iArrayToMFloatVectorArray(FabricCore::RTVal rtVal) { return RTValToMayaVecArray<MFloatVectorArray, MFloatVector, int>(2, "Vec2_i", rtVal); }

inline MFloatVectorArray Vec2ArrayToMFloatVectorArray(FabricCore::RTVal rtVal) { return RTValToMayaVecArray<MFloatVectorArray, MFloatVector, float>(2, "Vec2", rtVal); }

inline MVectorArray Vec2ArrayToMVectorArray(FabricCore::RTVal rtVal) { return RTValToMayaVecArray<MVectorArray, MVector, float>(2, "Vec2", rtVal); }

inline MVectorArray Vec2_dArrayToMVectorArray(FabricCore::RTVal rtVal) { return RTValToMayaVecArray<MVectorArray, MVector, double>(2, "Vec2_d", rtVal); }

// Vec3
inline MFloatVector Vec3_iToMFloatVector(FabricCore::RTVal rtVal) { return RTValToMayaVec<MFloatVector, int>(3, "Vec3_i", rtVal); }

inline MFloatVector Vec3ToMFloatVector(FabricCore::RTVal rtVal) { return RTValToMayaVec<MFloatVector, float>(3, "Vec3", rtVal); }

inline MVector Vec3ToMVector(FabricCore::RTVal rtVal) { return RTValToMayaVec<MVector, float>(3, "Vec3", rtVal); }
 
inline MVector Vec3_dToMVector(FabricCore::RTVal rtVal) { return RTValToMayaVec<MVector, double>(3, "Vec3_d", rtVal); }

inline MFloatVectorArray Vec3_iArrayToMFloatVectorArray(FabricCore::RTVal rtVal) { return RTValToMayaVecArray<MFloatVectorArray, MFloatVector, int>(3, "Vec3_i", rtVal); }

inline MFloatVectorArray Vec3ArrayToMFloatVectorArray(FabricCore::RTVal rtVal) { return RTValToMayaVecArray<MFloatVectorArray, MFloatVector, float>(3, "Vec3", rtVal); }

inline MVectorArray Vec3ArrayToMVectorArray(FabricCore::RTVal rtVal) { return RTValToMayaVecArray<MVectorArray, MVector, float>(3, "Vec3", rtVal); }

inline MVectorArray Vec3_dArrayToMVectorArray(FabricCore::RTVal rtVal) { return RTValToMayaVecArray<MVectorArray, MVector, double>(3, "Vec3_d", rtVal); }

// Vec4
inline MFloatPoint Vec4_iToMFloatPoint(FabricCore::RTVal rtVal) { return RTValToMayaPoint<MFloatPoint, int>(4, "Vec4_i", rtVal); }

inline MFloatPoint Vec4ToMFloatPoint(FabricCore::RTVal rtVal) { return RTValToMayaPoint<MFloatPoint, float>(4, "Vec4", rtVal); }

inline MPoint Vec4ToMPoint(FabricCore::RTVal rtVal) { return RTValToMayaPoint<MPoint, float>(4, "Vec4", rtVal); }
 
inline MPoint Vec4_dToMPoint(FabricCore::RTVal rtVal) { return RTValToMayaPoint<MPoint, double>(4, "Vec4_d", rtVal); }

inline MFloatPointArray Vec4_iArrayToMFloatPointArray(FabricCore::RTVal rtVal) { return RTValToMayaPointArray<MFloatPointArray, MFloatPoint, int>(4, "Vec4_i", rtVal); }

inline MFloatPointArray Vec4ArrayToMFloatPointArray(FabricCore::RTVal rtVal) { return RTValToMayaPointArray<MFloatPointArray, MFloatPoint, float>(4, "Vec4", rtVal); }

inline MPointArray Vec4ArrayToMPointArray(FabricCore::RTVal rtVal) { return RTValToMayaPointArray<MPointArray, MPoint, float>(4, "Vec4", rtVal); }

inline MPointArray Vec4_dArrayToMPointArray(FabricCore::RTVal rtVal) { return RTValToMayaPointArray<MPointArray, MPoint, double>(4, "Vec4_d", rtVal); }

// Color
inline MFloatVector ColorToMFloatVector(FabricCore::RTVal rtVal) { return RTValToMayaVec<MFloatVector, float>(3, "Color", rtVal); }

inline MVector ColorToMVector(FabricCore::RTVal rtVal) {  return RTValToMayaVec<MVector, float>(3, "Color", rtVal); }
 
inline MFloatVectorArray ColorArrayToMFloatVectorArray(FabricCore::RTVal rtVal) { return RTValToMayaVecArray<MFloatVectorArray, MFloatVector, float>(3, "Color", rtVal); }

inline MVectorArray ColorArrayToMVectorArray(FabricCore::RTVal rtVal) { return RTValToMayaVecArray<MVectorArray, MVector, float>(3, "Color", rtVal); }

inline MFloatPoint ColorToMFloatPoint(FabricCore::RTVal rtVal) { return RTValToMayaPoint<MFloatPoint, float>(4, "Color", rtVal); }

inline MPoint ColorToMPoint(FabricCore::RTVal rtVal) { return RTValToMayaPoint<MPoint, float>(4, "Color", rtVal); }
 
inline MFloatPointArray ColorArrayToMFloatPointArray(FabricCore::RTVal rtVal) { return RTValToMayaPointArray<MFloatPointArray, MFloatPoint, float>(4, "Color", rtVal); }

inline MPointArray ColorArrayToMPointArray(FabricCore::RTVal rtVal)  { return RTValToMayaPointArray<MPointArray, MPoint, float>(4, "Color", rtVal); }

// Euler 
inline MFloatVector EulerToMFloatVector(FabricCore::RTVal rtVal) { return RTValToMayaVec<MFloatVector, float>(3, "Euler", rtVal); }

inline MVector EulerToMVector(FabricCore::RTVal rtVal) { return RTValToMayaVec<MVector, float>(3, "Euler", rtVal); }

inline MVector Euler_dToMVector(FabricCore::RTVal rtVal) { return RTValToMayaVec<MVector, double>(3, "Euler_d", rtVal); }

inline MFloatVectorArray EulerArrayToMFloatVectorArray(FabricCore::RTVal rtVal) { return RTValToMayaVecArray<MFloatVectorArray, MFloatVector, float>(3, "Euler", rtVal); }
 
inline MVectorArray EulerArrayToMVectorArray(FabricCore::RTVal rtVal) { return RTValToMayaVecArray<MVectorArray, MVector, float>(3, "Euler", rtVal); }

inline MVectorArray Euler_dArrayToMVectorArray(FabricCore::RTVal rtVal) { return RTValToMayaVecArray<MVectorArray, MVector, double>(3, "Euler_d", rtVal); }
 
// Matrix 
inline MFloatMatrix Mat44ToMFloatMatrix(FabricCore::RTVal rtVal) { return RTValToMayaMatrix<MFloatMatrix, float>("Mat44", rtVal); }

inline MMatrix Mat44ToMMatrix(FabricCore::RTVal rtVal) { return RTValToMayaMatrix<MMatrix, float>("Mat44", rtVal); }

inline MFloatMatrix Mat44_dToMFloatMatrix(FabricCore::RTVal rtVal) { return RTValToMayaMatrix<MFloatMatrix, double>("Mat44_d", rtVal); }

inline MMatrix Mat44_dToMMatrix(FabricCore::RTVal rtVal) { return RTValToMayaMatrix<MMatrix, double>("Mat44_d", rtVal); }

inline MMatrixArray Mat44ArrayToMMatrixArray(FabricCore::RTVal rtVal) { return RTValToMayaMatrixArray<MMatrixArray, MMatrix, float>("Mat44", rtVal); }

inline MMatrixArray Mat44_dArrayToMMatrixArray(FabricCore::RTVal rtVal) { return RTValToMayaMatrixArray<MMatrixArray, MMatrix, double>("Mat44_d", rtVal); }

// Xfo 
inline MFloatMatrix XfoToMFloatMatrix(FabricCore::RTVal rtVal) { assert( rtVal.hasType( "Xfo" ) ); return Mat44ToMFloatMatrix( rtVal.callMethod("Mat44", "toMat44", 0, 0) ); }

inline MMatrix XfoToMMatrix(FabricCore::RTVal rtVal) { assert( rtVal.hasType( "Xfo" ) ); return Mat44ToMMatrix( rtVal.callMethod("Mat44", "toMat44", 0, 0) ); }

inline MFloatMatrix Xfo_dToMFloatMatrix(FabricCore::RTVal rtVal) { assert( rtVal.hasType( "Xfo_d" ) ); return Mat44_dToMFloatMatrix( rtVal.callMethod("Mat44_d", "toMat44_d", 0, 0) ); }

inline MMatrix Xfo_dToMMatrix(FabricCore::RTVal rtVal) { assert( rtVal.hasType( "Xfo_d" ) ); return Mat44ToMMatrix( rtVal.callMethod("Mat44_d", "toMat44_d", 0, 0) ); }

inline MMatrixArray XfoArrayToMMatrixArray(FabricCore::RTVal rtVal);

inline MMatrixArray Xfo_dArrayToMMatrixArray(FabricCore::RTVal rtVal);

// Geometry 
/// \internal
inline MObject MeschToMFnMesh(FabricCore::RTVal rtVal, bool insideCompute, MFnMesh &mesh);

inline MObject MeschToMFnMesh(FabricCore::RTVal rtVal, bool insideCompute);

/// \internal
inline MObject LinesToMFnNurbsCurve_single(FabricCore::RTVal rtVal, MFnNurbsCurve &curve);

inline MObject LinesToMFnNurbsCurve(FabricCore::RTVal rtVal);

/// \internal
inline MObject CurveToMFnNurbsCurve_single(FabricCore::RTVal rtVal, int index, MFnNurbsCurve &curve);

inline MObject CurveToMFnNurbsCurve(FabricCore::RTVal rtVal, MFnNurbsCurve &curve);

inline MObject CurveToMFnNurbsCurve(FabricCore::RTVal rtVal);
// ***************** RTVal to Maya ***************** // 

} // namespace Conversion 
} // namespace FabricMaya

#endif // __FABRIC_MAYA_CONVERSION__
