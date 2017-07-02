//
// Copyright (c) 2010-2017 Fabric Software Inc. All rights reserved.
//

#pragma once

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

class FabricConversion 
{
public:
// ***************** Maya to RTVal ***************** // 
// template
template<typename FPTy, typename ArrayTy>
static FabricCore::RTVal MayaToRTValArray(const char *type, ArrayTy const&array) {

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
static FabricCore::RTVal MayaToRTValVec(unsigned int dim, const char *type, VectorTy const&vector) {

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
static FabricCore::RTVal MayaToRTValVecArray(unsigned int dim, const char *type, VectorArrayTy const&array) {
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

template<typename MatrixTy, typename FPTy>
static void MayaToRTValMatrixData(MatrixTy const &matrix, FPTy *fpArray) {

  for(unsigned int i=0; i<4; ++i)
    for(unsigned int j=0; j<4; ++j)
      fpArray[4*i + j] = (FPTy)matrix[j][i];
}

template<typename FPTy, typename MatrixTy>
static FabricCore::RTVal MayaToRTValMatrix(const char *type, MatrixTy const&matrix) {
  FabricCore::RTVal rtVal;

  CONVERSION_CATCH_BEGIN;

  rtVal = FabricSplice::constructRTVal(type);
  FabricCore::RTVal dataRtVal = rtVal.callMethod("Data", "data", 0, 0);
  FPTy* data = (FPTy*)dataRtVal.getData();
  MayaToRTValMatrixData(matrix, data);
 
  CONVERSION_CATCH_END(
    QString(
      "FabricConversion::MayaToRTValMatrix::" + QString(type)
      ).toUtf8().constData());

  return rtVal;
}

template<typename FPTy, typename MatrixArrayTy>
static FabricCore::RTVal MayaToRTValMatrixArray(const char *type, MatrixArrayTy const&array) {
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
static FabricCore::RTVal MStringToString(MString const&str);

static FabricCore::RTVal MStringArrayToStringArray(MStringArray const&strArray);

// Array
static FabricCore::RTVal MInt32ArrayToSIntArray(MIntArray const&array) { return MayaToRTValArray<int>("SInt32", array); } 

static FabricCore::RTVal MFloatArrayToFloat32Array(MFloatArray const&array) { return MayaToRTValArray<float>("Float32", array); } 

static FabricCore::RTVal MFloatArrayToFloat64Array(MFloatArray const&array) { return MayaToRTValArray<double>("Float64", array); } 

static FabricCore::RTVal MDoubleArrayToFloat32Array(MDoubleArray const&array) { return MayaToRTValArray<float>("float", array); } 

static FabricCore::RTVal MDoubleArrayToFloat64Array(MDoubleArray const&array) { return MayaToRTValArray<double>("Float64", array); } 

// Vec2
static FabricCore::RTVal MFloatVectorToVec2_i(MFloatVector const&vector) { return MayaToRTValVec<int>(2, "Vec2_i", vector); }

static FabricCore::RTVal MFloatVectorToVec2(MFloatVector const&vector) { return MayaToRTValVec<float>(2, "Vec2", vector); }

static FabricCore::RTVal MVectorToVec2(MVector const&vector) { return MayaToRTValVec<float>(2, "Vec2", vector); }

static FabricCore::RTVal MVectorToVec2_d(MVector const& vector) { return MayaToRTValVec<double>(2, "Vec2_d", vector); }

static FabricCore::RTVal MFloatVectorArrayToVec2_iArray(MFloatVectorArray const& array) { return MayaToRTValVecArray<int>(2, "Vec2_i", array); }

static FabricCore::RTVal MFloatVectorArrayToVec2Array(MFloatVectorArray const& array) { return MayaToRTValVecArray<float>(2, "Vec2", array); }

static FabricCore::RTVal MVectorArrayToVec2Array(MVectorArray const& array) { return MayaToRTValVecArray<float>(2, "Vec2", array); }

static FabricCore::RTVal MVectorArrayToVec2_dArray(MVectorArray const& array) { return MayaToRTValVecArray<double>(2, "Vec2_d", array); }

// Vec3
static FabricCore::RTVal MFloatVectorToVec3_i(MFloatVector const&vector) { return MayaToRTValVec<int>(3, "Vec3_i", vector); }

static FabricCore::RTVal MFloatVectorToVec3(MFloatVector const&vector) { return MayaToRTValVec<float>(3, "Vec3", vector); }

static FabricCore::RTVal MVectorToVec3(MVector const&vector) { return MayaToRTValVec<float>(3, "Vec3", vector); }

static FabricCore::RTVal MVectorToVec3_d(MVector const& vector) { return MayaToRTValVec<double>(3, "Vec3_d", vector); }

static FabricCore::RTVal MFloatVectorArrayToVec3_iArray(MFloatVectorArray const& array) { return MayaToRTValVecArray<int>(3, "Vec3_i", array); }

static FabricCore::RTVal MFloatVectorArrayToVec3Array(MFloatVectorArray const& array) { return MayaToRTValVecArray<float>(3, "Vec3", array); }

static FabricCore::RTVal MVectorArrayToVec3Array(MVectorArray const& array) { return MayaToRTValVecArray<float>(3, "Vec3", array); }

static FabricCore::RTVal MVectorArrayToVec3_dArray(MVectorArray const& array) { return MayaToRTValVecArray<double>(3, "Vec3_d", array); }

// Vec4
static FabricCore::RTVal MFloatPointToVec4_i(MFloatPoint const&point) { return MayaToRTValVec<int>(4, "Vec4_i", point); }

static FabricCore::RTVal MFloatPointToVec4(MFloatPoint const&point) { return MayaToRTValVec<float>(4, "Vec4", point); }

static FabricCore::RTVal MPointToVec4(MPoint const&point) { return MayaToRTValVec<float>(4, "Vec4", point); }

static FabricCore::RTVal MPointToVec4_d(MPoint const& point) { return MayaToRTValVec<double>(4, "Vec4_d", point); }

static FabricCore::RTVal MFloatPointArrayToVec4_iArray(MFloatPointArray const& array) { return MayaToRTValVecArray<int>(4, "Vec4_i", array); }

static FabricCore::RTVal MFloatPointArrayToVec4Array(MFloatPointArray const& array) { return MayaToRTValVecArray<float>(4, "Vec4", array); }

static FabricCore::RTVal MPointArrayToVec4Array(MPointArray const& array) { return MayaToRTValVecArray<float>(4, "Vec4", array); }

static FabricCore::RTVal MPointArrayToVec4_dArray(MPointArray const& array) { return MayaToRTValVecArray<double>(4, "Vec4_d", array); }

// Color
static FabricCore::RTVal MFloatVectorToColor(MFloatVector const&vector) { return MayaToRTValVec<float>(3, "Color", vector); }

static FabricCore::RTVal MVectorToColor(MVector const&vector) { return MayaToRTValVec<float>(3, "Color", vector); }
 
static FabricCore::RTVal MFloatVectorArrayToColorArray(MFloatVectorArray const& array) { return MayaToRTValVecArray<float>(3, "Color", array); }

static FabricCore::RTVal MVectorArrayToColorArray(MVectorArray const& array) { return MayaToRTValVecArray<float>(3, "Color", array); }

static FabricCore::RTVal MFloatPointToColor(MFloatPoint const&point) { return MayaToRTValVec<float>(4, "Color", point); }

static FabricCore::RTVal MPointToColor(MPoint const&point) { return MayaToRTValVec<float>(4, "Color", point); }
 
static FabricCore::RTVal MFloatPointArrayToColorArray(MFloatPointArray const& array) { return MayaToRTValVecArray<float>(4, "Color", array); }

static FabricCore::RTVal MPointArrayToColorArray(MPointArray const& array) { return MayaToRTValVecArray<float>(4, "Color", array); }

// Euler
static FabricCore::RTVal MFloatVectorToEuler(MFloatVector const&vector) { return MayaToRTValVec<float>(3, "Euler", vector); }

static FabricCore::RTVal MVectorToEuler(MVector const&vector) { return MayaToRTValVec<float>(3, "Euler", vector); }
 
static FabricCore::RTVal MVectorToEuler_d(MVector const&vector) { return MayaToRTValVec<double>(3, "Euler_d", vector); }

static FabricCore::RTVal MFloatVectorArrayToEulerArray(MFloatVectorArray const& array) { return MayaToRTValVecArray<float>(3, "Euler", array); }

static FabricCore::RTVal MVectorArrayToEulerArray(MVectorArray const& array) { return MayaToRTValVecArray<float>(3, "Euler", array); }

static FabricCore::RTVal MVectorArrayToEuler_dArray(MVectorArray const& array) { return MayaToRTValVecArray<double>(3, "Euler_d", array); }

static FabricCore::RTVal MFloatPointToEuler(MFloatPoint const&point) { return MayaToRTValVec<float>(4, "Euler", point); }
 
// Matrix 
static FabricCore::RTVal MFloatMatrixToMat44(MFloatMatrix const&matrix) { return MayaToRTValMatrix<float>("Mat44", matrix); }

static FabricCore::RTVal MMatrixToMat44(MMatrix const&matrix) { return MayaToRTValMatrix<float>("Mat44", matrix); }

static FabricCore::RTVal MFloatMatrixToMat44_d(MFloatMatrix const&matrix) { return MayaToRTValMatrix<double>("Mat44_d", matrix); }

static FabricCore::RTVal MMatrixToMat44_d(MMatrix const&matrix) { return MayaToRTValMatrix<double>("Mat44_d", matrix); }

static FabricCore::RTVal MMatrixToMat44Array(MMatrixArray const&array) { return MayaToRTValMatrixArray<float>("Mat44", array); }
 
static FabricCore::RTVal MMatrixToMat44_dArray(MMatrixArray const&array) { return MayaToRTValMatrixArray<double>("Mat44_d", array); }

// Xfo 
static FabricCore::RTVal MFloatMatrixToXfo(MFloatMatrix const&matrix) { FabricCore::RTVal matrixVal = MFloatMatrixToMat44(matrix); return FabricSplice::constructRTVal("Xfo", 1, &matrixVal); }

static FabricCore::RTVal MMatrixToXfo(MMatrix const&matrix) { FabricCore::RTVal matrixVal = MMatrixToMat44(matrix); return FabricSplice::constructRTVal("Xfo", 1, &matrixVal); }
 
static FabricCore::RTVal MFloatMatrixToXfo_d(MFloatMatrix const&matrix) { FabricCore::RTVal matrixVal = MFloatMatrixToMat44_d(matrix); return FabricSplice::constructRTVal("Xfo_d", 1, &matrixVal); }

static FabricCore::RTVal MMatrixToXfo_d(MMatrix const&matrix) { FabricCore::RTVal matrixVal = MMatrixToMat44_d(matrix); return FabricSplice::constructRTVal("Xfo_d", 1, &matrixVal); }

static FabricCore::RTVal MMatrixToXfoArray(MMatrixArray const&array);

static FabricCore::RTVal MMatrixToXfo_dArray(MMatrixArray const&array);

// Geometry 
/// \internal
static bool MFnMeshToMesch(MFnMesh &mesh, FabricCore::RTVal rtVal);

static FabricCore::RTVal MFnMeshToMesch(MFnMesh &mesh);

static bool MFnNurbsCurveToLine(MFnNurbsCurve &curve, FabricCore::RTVal rtVal);

static FabricCore::RTVal MFnNurbsCurveToLine(MFnNurbsCurve &curve);

static bool MFnNurbsCurveToCurve(unsigned int index, MFnNurbsCurve &curve, FabricCore::RTVal rtVal);
// ***************** Maya to RTVal ***************** // 


// ***************** RTVal to Maya ***************** // 
// template
template<typename ArrayTy, typename FPTy>
static ArrayTy RTValToMayaArray(const char *type, FabricCore::RTVal rtVal) {
  
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
static VectorTy RTValToMayaVec(unsigned int dim, const char *type, FabricCore::RTVal rtVal) {
  
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
static VectorTy RTValToMayaPoint(unsigned int dim, const char *type, FabricCore::RTVal rtVal) {
  
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
static VectorArrayTy RTValToMayaVecArray(unsigned int dim, const char *type, FabricCore::RTVal rtVal) {

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
static PointArrayTy RTValToMayaPointArray(unsigned int dim, const char *type, FabricCore::RTVal rtVal) {

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
static void RTValToMayaMatrixData(FPTy const *data, MatrixTy &matrix) {
  FPTy vals[4][4] = {
    { data[0], data[4], data[8],  data[12] },
    { data[1], data[5], data[9],  data[13] },
    { data[2], data[6], data[10], data[14] },
    { data[3], data[7], data[11], data[15] }
  };
  matrix = MatrixTy(vals);
}

template<typename MatrixTy, typename FPTy>
static MatrixTy RTValToMayaMatrix(const char *type, FabricCore::RTVal rtVal) {
  
  assert( rtVal.hasType( type ) );

  MatrixTy matrix;

  CONVERSION_CATCH_BEGIN;

  FabricCore::RTVal dataRtVal = rtVal.callMethod("Data", "data", 0, 0);
  FPTy *data = (FPTy*)dataRtVal.getData();
  RTValToMayaMatrixData(data, matrix);
 
  CONVERSION_CATCH_END(
    QString(
      "FabricConversion::RTValToMayaMatrix::" + QString(type)
      ).toUtf8().constData());

  return matrix;
}

template<typename MatrixArrayTy, typename MatrixTy, typename FPTy>
static MatrixArrayTy RTValToMayaMatrixArray(const char *type, FabricCore::RTVal rtVal) {

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
static MString StringToMString(FabricCore::RTVal rtVal);

static MStringArray StringArrayToMStringArray(FabricCore::RTVal rtVal);

// Array
static MIntArray SInt32ArrayToMIntArray(FabricCore::RTVal rtVal) { return RTValToMayaArray<MIntArray, int>("SInt32", rtVal); } 

static MIntArray UInt32ArrayToMIntArray(FabricCore::RTVal rtVal) { return RTValToMayaArray<MIntArray, int>("UInt32", rtVal); } 

static MFloatArray Float32ArrayToMFloatArray(FabricCore::RTVal rtVal) { return RTValToMayaArray<MFloatArray, float>("Float32", rtVal); } 

static MFloatArray Float64ArrayToMFloatArray(FabricCore::RTVal rtVal) { return RTValToMayaArray<MFloatArray, double>("Float64", rtVal); } 

static MDoubleArray Float32ArrayToMDoubleArray(FabricCore::RTVal rtVal) { return RTValToMayaArray<MDoubleArray, float>("Float32", rtVal); } 

static MDoubleArray Float64ArrayToMDoubleArray(FabricCore::RTVal rtVal) { return RTValToMayaArray<MDoubleArray, double>("Float64", rtVal); } 

// Vec2
static MFloatVector Vec2_iToMFloatVector(FabricCore::RTVal rtVal) { return RTValToMayaVec<MFloatVector, int>(2, "Vec2_i", rtVal); }

static MFloatVector Vec2ToMFloatVector(FabricCore::RTVal rtVal) { return RTValToMayaVec<MFloatVector, float>(2, "Vec2", rtVal); }

static MVector Vec2ToMVector(FabricCore::RTVal rtVal) { return RTValToMayaVec<MVector, float>(2, "Vec2", rtVal); }
 
static MVector Vec2_dToMVector(FabricCore::RTVal rtVal) { return RTValToMayaVec<MVector, double>(2, "Vec2_d", rtVal); }

static MFloatVectorArray Vec2_iArrayToMFloatVectorArray(FabricCore::RTVal rtVal) { return RTValToMayaVecArray<MFloatVectorArray, MFloatVector, int>(2, "Vec2_i", rtVal); }

static MFloatVectorArray Vec2ArrayToMFloatVectorArray(FabricCore::RTVal rtVal) { return RTValToMayaVecArray<MFloatVectorArray, MFloatVector, float>(2, "Vec2", rtVal); }

static MVectorArray Vec2ArrayToMVectorArray(FabricCore::RTVal rtVal) { return RTValToMayaVecArray<MVectorArray, MVector, float>(2, "Vec2", rtVal); }

static MVectorArray Vec2_dArrayToMVectorArray(FabricCore::RTVal rtVal) { return RTValToMayaVecArray<MVectorArray, MVector, double>(2, "Vec2_d", rtVal); }

// Vec3
static MFloatVector Vec3_iToMFloatVector(FabricCore::RTVal rtVal) { return RTValToMayaVec<MFloatVector, int>(3, "Vec3_i", rtVal); }

static MFloatVector Vec3ToMFloatVector(FabricCore::RTVal rtVal) { return RTValToMayaVec<MFloatVector, float>(3, "Vec3", rtVal); }

static MVector Vec3ToMVector(FabricCore::RTVal rtVal) { return RTValToMayaVec<MVector, float>(3, "Vec3", rtVal); }
 
static MVector Vec3_dToMVector(FabricCore::RTVal rtVal) { return RTValToMayaVec<MVector, double>(3, "Vec3_d", rtVal); }

static MFloatVectorArray Vec3_iArrayToMFloatVectorArray(FabricCore::RTVal rtVal) { return RTValToMayaVecArray<MFloatVectorArray, MFloatVector, int>(3, "Vec3_i", rtVal); }

static MFloatVectorArray Vec3ArrayToMFloatVectorArray(FabricCore::RTVal rtVal) { return RTValToMayaVecArray<MFloatVectorArray, MFloatVector, float>(3, "Vec3", rtVal); }

static MVectorArray Vec3ArrayToMVectorArray(FabricCore::RTVal rtVal) { return RTValToMayaVecArray<MVectorArray, MVector, float>(3, "Vec3", rtVal); }

static MVectorArray Vec3_dArrayToMVectorArray(FabricCore::RTVal rtVal) { return RTValToMayaVecArray<MVectorArray, MVector, double>(3, "Vec3_d", rtVal); }

// Vec4
static MFloatPoint Vec4_iToMFloatPoint(FabricCore::RTVal rtVal) { return RTValToMayaPoint<MFloatPoint, int>(4, "Vec4_i", rtVal); }

static MFloatPoint Vec4ToMFloatPoint(FabricCore::RTVal rtVal) { return RTValToMayaPoint<MFloatPoint, float>(4, "Vec4", rtVal); }

static MPoint Vec4ToMPoint(FabricCore::RTVal rtVal) { return RTValToMayaPoint<MPoint, float>(4, "Vec4", rtVal); }
 
static MPoint Vec4_dToMPoint(FabricCore::RTVal rtVal) { return RTValToMayaPoint<MPoint, double>(4, "Vec4_d", rtVal); }

static MFloatPointArray Vec4_iArrayToMFloatPointArray(FabricCore::RTVal rtVal) { return RTValToMayaPointArray<MFloatPointArray, MFloatPoint, int>(4, "Vec4_i", rtVal); }

static MFloatPointArray Vec4ArrayToMFloatPointArray(FabricCore::RTVal rtVal) { return RTValToMayaPointArray<MFloatPointArray, MFloatPoint, float>(4, "Vec4", rtVal); }

static MPointArray Vec4ArrayToMPointArray(FabricCore::RTVal rtVal) { return RTValToMayaPointArray<MPointArray, MPoint, float>(4, "Vec4", rtVal); }

static MPointArray Vec4_dArrayToMPointArray(FabricCore::RTVal rtVal) { return RTValToMayaPointArray<MPointArray, MPoint, double>(4, "Vec4_d", rtVal); }

// Color
static MFloatVector ColorToMFloatVector(FabricCore::RTVal rtVal) { return RTValToMayaVec<MFloatVector, float>(3, "Color", rtVal); }

static MVector ColorToMVector(FabricCore::RTVal rtVal) {  return RTValToMayaVec<MVector, float>(3, "Color", rtVal); }
 
static MFloatVectorArray ColorArrayToMFloatVectorArray(FabricCore::RTVal rtVal) { return RTValToMayaVecArray<MFloatVectorArray, MFloatVector, float>(3, "Color", rtVal); }

static MVectorArray ColorArrayToMVectorArray(FabricCore::RTVal rtVal) { return RTValToMayaVecArray<MVectorArray, MVector, float>(3, "Color", rtVal); }

static MFloatPoint ColorToMFloatPoint(FabricCore::RTVal rtVal) { return RTValToMayaPoint<MFloatPoint, float>(4, "Color", rtVal); }

static MPoint ColorToMPoint(FabricCore::RTVal rtVal) { return RTValToMayaPoint<MPoint, float>(4, "Color", rtVal); }
 
static MFloatPointArray ColorArrayToMFloatPointArray(FabricCore::RTVal rtVal) { return RTValToMayaPointArray<MFloatPointArray, MFloatPoint, float>(4, "Color", rtVal); }

static MPointArray ColorArrayToMPointArray(FabricCore::RTVal rtVal)  { return RTValToMayaPointArray<MPointArray, MPoint, float>(4, "Color", rtVal); }

// Euler 
static MFloatVector EulerToMFloatVector(FabricCore::RTVal rtVal) { return RTValToMayaVec<MFloatVector, float>(3, "Euler", rtVal); }

static MVector EulerToMVector(FabricCore::RTVal rtVal) { return RTValToMayaVec<MVector, float>(3, "Euler", rtVal); }

static MVector Euler_dToMVector(FabricCore::RTVal rtVal) { return RTValToMayaVec<MVector, double>(3, "Euler_d", rtVal); }

static MFloatVectorArray EulerArrayToMFloatVectorArray(FabricCore::RTVal rtVal) { return RTValToMayaVecArray<MFloatVectorArray, MFloatVector, float>(3, "Euler", rtVal); }
 
static MVectorArray EulerArrayToMVectorArray(FabricCore::RTVal rtVal) { return RTValToMayaVecArray<MVectorArray, MVector, float>(3, "Euler", rtVal); }

static MVectorArray Euler_dArrayToMVectorArray(FabricCore::RTVal rtVal) { return RTValToMayaVecArray<MVectorArray, MVector, double>(3, "Euler_d", rtVal); }
 
// Matrix 
static MFloatMatrix Mat44ToMFloatMatrix(FabricCore::RTVal rtVal) { return RTValToMayaMatrix<MFloatMatrix, float>("Mat44", rtVal); }

static MMatrix Mat44ToMMatrix(FabricCore::RTVal rtVal) { return RTValToMayaMatrix<MMatrix, float>("Mat44", rtVal); }

static MFloatMatrix Mat44_dToMFloatMatrix(FabricCore::RTVal rtVal) { return RTValToMayaMatrix<MFloatMatrix, double>("Mat44_d", rtVal); }

static MMatrix Mat44_dToMMatrix(FabricCore::RTVal rtVal) { return RTValToMayaMatrix<MMatrix, double>("Mat44_d", rtVal); }

static MMatrixArray Mat44ArrayToMMatrixArray(FabricCore::RTVal rtVal) { return RTValToMayaMatrixArray<MMatrixArray, MMatrix, float>("Mat44", rtVal); }

static MMatrixArray Mat44_dArrayToMMatrixArray(FabricCore::RTVal rtVal) { return RTValToMayaMatrixArray<MMatrixArray, MMatrix, double>("Mat44_d", rtVal); }

// Xfo 
static MFloatMatrix XfoToMFloatMatrix(FabricCore::RTVal rtVal) { assert( rtVal.hasType( "Xfo" ) ); return Mat44ToMFloatMatrix( rtVal.callMethod("Mat44", "toMat44", 0, 0) ); }

static MMatrix XfoToMMatrix(FabricCore::RTVal rtVal) { assert( rtVal.hasType( "Xfo" ) ); return Mat44ToMMatrix( rtVal.callMethod("Mat44", "toMat44", 0, 0) ); }

static MFloatMatrix Xfo_dToMFloatMatrix(FabricCore::RTVal rtVal) { assert( rtVal.hasType( "Xfo_d" ) ); return Mat44_dToMFloatMatrix( rtVal.callMethod("Mat44_d", "toMat44_d", 0, 0) ); }

static MMatrix Xfo_dToMMatrix(FabricCore::RTVal rtVal) { assert( rtVal.hasType( "Xfo_d" ) ); return Mat44ToMMatrix( rtVal.callMethod("Mat44_d", "toMat44_d", 0, 0) ); }

static MMatrixArray XfoArrayToMMatrixArray(FabricCore::RTVal rtVal);

static MMatrixArray Xfo_dArrayToMMatrixArray(FabricCore::RTVal rtVal);

// Geometry 
static MObject MeschToMFnMesh(FabricCore::RTVal rtVal, bool insideCompute, MFnMesh &mesh);

static MObject MeschToMFnMesh(FabricCore::RTVal rtVal, bool insideCompute);

static MObject LinesToMFnNurbsCurve(FabricCore::RTVal rtVal, MFnNurbsCurve &curve);

static MObject LinesToMFnNurbsCurve(FabricCore::RTVal rtVal);

static MObject CurveToMFnNurbsCurve(FabricCore::RTVal rtVal, int index, MFnNurbsCurve &curve);

static MObject CurveToMFnNurbsCurve(FabricCore::RTVal rtVal, int index);

static MObject CurveToMFnNurbsCurve(FabricCore::RTVal rtVal, MFnNurbsCurve &curve);

static MObject CurveToMFnNurbsCurve(FabricCore::RTVal rtVal);
// ***************** RTVal to Maya ***************** // 
};
