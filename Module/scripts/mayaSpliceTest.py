
from optparse import OptionParser
import os

parser = OptionParser()
parser.add_option(
  "--mv", "--maya-version", 
  dest="mayaVersion",
  default='2013',
  help="maya version (ex. 2013)")

(options, args) = parser.parse_args()

mayaVersion = options.mayaVersion

def testGenericAttributes():
  from maya import cmds, OpenMaya

  cmds.file(newFile = True, force = True)

  node = cmds.createNode("spliceMayaNode")
  cmds.addAttr(longName = 'in1', attributeType = "float", keyable = True, storable = True)
  cmds.addAttr(longName = 'in2', attributeType = "float", keyable = True, storable = True)
  cmds.addAttr(longName = 'out', attributeType = "float", keyable = True)

  cmds.fabricSplice('addInputPort', node, 'in1', 'Scalar')
  cmds.fabricSplice('addInputPort', node, 'in2', 'Scalar')
  cmds.fabricSplice('addOutputPort', node, 'out', 'Scalar')
  cmds.fabricSplice('addKLOperator', node, 'helloWorldOp')
  cmds.fabricSplice('setKLOperatorCode', node, 'helloWorldOp', """
    operator helloWorldOp(Scalar in1, Scalar in2, io Scalar out) {
      report('computing KL in maya!');
      report(in1 + " + " + in2);
      out = in1 + in2;
    }
    """)

  cmds.setAttr(node + '.in1', 1.0)
  cmds.setAttr(node + '.in2', 2.0)
  assert cmds.getAttr(node + '.out') == 3.0
  
  cmds.file(rename = 'test.ma')
  cmds.file(f = True, save = True, type = 'mayaAscii')

  cmds.file(newFile = True, force = True)
  cmds.file('test.ma', o = True)

  cmds.setAttr(node + '.in1', 3.0)
  cmds.setAttr(node + '.in2', 2.0)
  assert cmds.getAttr(node + '.out') == 5.0

  cmds.file(newFile = True, force = True)
  node = cmds.createNode("spliceMayaNode")
  cmds.addAttr(longName = 'in1', attributeType = "long", keyable = True, storable = True)
  cmds.addAttr(longName = 'in2', attributeType = "long", keyable = True, storable = True)
  cmds.addAttr(longName = 'out', attributeType = "long", keyable = True)

  cmds.fabricSplice('addInputPort', node, 'in1', 'Integer')
  cmds.fabricSplice('addInputPort', node, 'in2', 'Integer')
  cmds.fabricSplice('addOutputPort', node, 'out', 'Integer')
  cmds.fabricSplice('addKLOperator', node, 'helloWorldOp')
  cmds.fabricSplice('setKLOperatorCode', node, 'helloWorldOp', """
    operator helloWorldOp(Integer in1, Integer in2, io Integer out) {
      report('computing KL in maya!');
      report(in1 + " + " + in2);
      out = in1 + in2;
    }
    """)

  cmds.setAttr(node + '.in1', 3)
  cmds.setAttr(node + '.in2', 2)
  assert cmds.getAttr(node + '.out')

  cmds.file(newFile = True, force = True)
  node = cmds.createNode("spliceMayaNode")
  cmds.addAttr(longName = 'in1', attributeType = "long", keyable = True, storable = True)
  cmds.addAttr(longName = 'in2', dataType = "string", keyable = True, storable = True)
  cmds.addAttr(longName = 'out', dataType = "string", keyable = True)

  cmds.fabricSplice('addInputPort', node, 'in1', 'Integer')
  cmds.fabricSplice('addInputPort', node, 'in2', 'String')
  cmds.fabricSplice('addOutputPort', node, 'out', 'String')
  cmds.fabricSplice('addKLOperator', node, 'helloWorldOp')
  cmds.fabricSplice('setKLOperatorCode', node, 'helloWorldOp', """
    operator helloWorldOp(Integer in1, String in2, io String out) {
      report('computing KL in maya!');
      report(in1 + " + " + in2);
      out = in1 + " times " + in2;
    }
    """)

  cmds.setAttr(node + '.in1', 8)
  cmds.setAttr(node + '.in2', "hello", type = "string")
  print cmds.getAttr(node + '.out')


def testVec():
  from maya import cmds, OpenMaya

  cmds.file(newFile = True, force = True)

  node = cmds.createNode("spliceMayaNode")
  cmds.addAttr(longName = 'in1', attributeType = "float3", keyable = True, storable = True)
  cmds.addAttr(longName='in1X', attributeType='float', parent='in1' )
  cmds.addAttr(longName='in1Y', attributeType='float', parent='in1' )
  cmds.addAttr(longName='in1Z', attributeType='float', parent='in1' )

  cmds.addAttr(longName = 'outX', attributeType = "float", keyable = True, storable = True)
  cmds.addAttr(longName = 'outY', attributeType = "float", keyable = True, storable = True)
  cmds.addAttr(longName = 'outZ', attributeType = "float", keyable = True, storable = True)

  cmds.fabricSplice('addInputPort', node, 'in1', 'Vec3')
  cmds.fabricSplice('addOutputPort', node, 'outX', 'Scalar')
  cmds.fabricSplice('addOutputPort', node, 'outY', 'Scalar')
  cmds.fabricSplice('addOutputPort', node, 'outZ', 'Scalar')
  cmds.fabricSplice('addKLOperator', node, 'decomposeVec3')
  cmds.fabricSplice('setKLOperatorCode', node, 'decomposeVec3', """
    operator decomposeVec3(Vec3 in1, io Scalar outX, io Scalar outY, io Scalar outZ) {
      report('computing KL in maya!');
      report(in1);

      outX = in1.x;
      outY = in1.y;
      outZ = in1.z;
    }
    """)

  cmds.setAttr(node + '.in1X', 2.0)
  cmds.setAttr(node + '.in1Y', 5.0)
  cmds.setAttr(node + '.in1Z', 8.0)
  
  assert cmds.getAttr(node + '.outX') == 2.0
  assert cmds.getAttr(node + '.outY') == 5.0
  assert cmds.getAttr(node + '.outZ') == 8.0

def testMatrix():
  from maya import cmds, OpenMaya

  cmds.file(newFile = True, force = True)

  node = cmds.createNode("spliceMayaNode")
  cmds.addAttr(longName = 'in1', attributeType = "matrix", storable = True)
  cmds.addAttr(longName = 'outHeight', attributeType = "float", storable = True)
  
  cmds.fabricSplice('addInputPort', node, 'in1', 'Mat44')
  cmds.fabricSplice('addOutputPort', node, 'outHeight', 'Scalar')
  cmds.fabricSplice('addKLOperator', node, 'extractHeight')
  cmds.fabricSplice('setKLOperatorCode', node, 'extractHeight', """
    use Xfo;

    operator extractHeight(Mat44 in1, io Scalar outHeight) {
      report('computing KL in maya!');
      report(in1);

      Xfo xfo;
      xfo.setFromMat44(in1);

      outHeight = xfo.tr.y;
    }
    """)
  
  locator = cmds.spaceLocator()[0]
  cmds.connectAttr(locator + '.matrix', node + '.in1')

  cmds.setAttr(locator + '.translateY', 5.0)
  assert cmds.getAttr(node + '.outHeight') == 5.0

def testEuler():
  from maya import cmds, OpenMaya

  cmds.file(newFile = True, force = True)

  node = cmds.createNode("spliceMayaNode")
  cmds.addAttr(longName = 'in1', attributeType = "double3", keyable = True, storable = True)
  cmds.addAttr(longName = 'in1X', attributeType = 'doubleAngle', parent = 'in1' )
  cmds.addAttr(longName = 'in1Y', attributeType = 'doubleAngle', parent = 'in1' )
  cmds.addAttr(longName = 'in1Z', attributeType = 'doubleAngle', parent = 'in1' )

  cmds.addAttr(longName = 'out', attributeType = "double3", keyable = True, storable = True)
  cmds.addAttr(longName = 'outX', attributeType = 'doubleAngle', parent = 'out' )
  cmds.addAttr(longName = 'outY', attributeType = 'doubleAngle', parent = 'out' )
  cmds.addAttr(longName = 'outZ', attributeType = 'doubleAngle', parent = 'out' )

  cmds.fabricSplice('addInputPort', node, 'in1', 'Euler')
  cmds.fabricSplice('addOutputPort', node, 'out', 'Euler')
  cmds.fabricSplice('addKLOperator', node, 'testEuler')
  cmds.fabricSplice('setKLOperatorCode', node, 'testEuler', """
    use Euler;

    operator testEuler(Euler in1, io Euler out) {
      report('computing KL in maya!');
      report(in1);

      out.x = in1.x;
    }
    """)

  locator = cmds.spaceLocator()[0]
  cmds.connectAttr(locator + '.rotate', node + '.in1')

  cmds.setAttr(locator + '.rotateX', 45.0)
  round(cmds.getAttr(node + '.outX')) == cmds.getAttr(locator + '.rotateX')

def testBaseTypesArray():
  from maya import cmds, OpenMaya

  cmds.file(newFile = True, force = True)

  node = cmds.createNode("spliceMayaNode")
  cmds.addAttr(longName = 'in1', attributeType = "float", multi = True)
  cmds.addAttr(longName = 'out', attributeType = "float", multi = True)
  
  cmds.fabricSplice('addInputPort', node, 'in1', 'Scalar[]')
  cmds.fabricSplice('addOutputPort', node, 'out', 'Scalar[]')
  cmds.fabricSplice('addKLOperator', node, 'testArray')
  cmds.fabricSplice('setKLOperatorCode', node, 'testArray', """
    operator testArray(Scalar in1[], io Scalar out[]) {
      report('computing KL in maya!');
      report(in1);

      out.resize(in1.size());
      for(Size i = 0; i < in1.size(); ++i){
        out[i] = in1[i] + 1.0;
      }
    }
    """)

  values = [1.0, 2.0, 3.0]
  cmds.setAttr(node + '.in1[0]', values[0])
  cmds.setAttr(node + '.in1[1]', values[1])
  cmds.setAttr(node + '.in1[2]', values[2])

  assert cmds.getAttr(node + '.out[0]') == values[0] + 1.0
  assert cmds.getAttr(node + '.out[1]') == values[1] + 1.0
  assert cmds.getAttr(node + '.out[2]') == values[2] + 1.0

  cmds.file(newFile = True, force = True)

  node = cmds.createNode("spliceMayaNode")
  cmds.addAttr(longName = 'in1', attributeType = "long", multi = True)
  cmds.addAttr(longName = 'out', attributeType = "long", multi = True)
  
  cmds.fabricSplice('addInputPort', node, 'in1', 'Integer[]')
  cmds.fabricSplice('addOutputPort', node, 'out', 'Integer[]')
  cmds.fabricSplice('addKLOperator', node, 'testArray')
  cmds.fabricSplice('setKLOperatorCode', node, 'testArray', """
    operator testArray(Integer in1[], io Integer out[]) {
      report('computing KL in maya!');
      report(in1);

      out.resize(in1.size());
      for(Size i = 0; i < in1.size(); ++i){
        out[i] = in1[i] + 1;
      }
    }
    """)

  values = [1, 2, 3]
  cmds.setAttr(node + '.in1[0]', values[0])
  cmds.setAttr(node + '.in1[1]', values[1])
  cmds.setAttr(node + '.in1[2]', values[2])

  assert cmds.getAttr(node + '.out[0]') == values[0] + 1
  assert cmds.getAttr(node + '.out[1]') == values[1] + 1
  assert cmds.getAttr(node + '.out[2]') == values[2] + 1

def testVecArray():
  from maya import cmds, OpenMaya

  cmds.file(newFile = True, force = True)

  node = cmds.createNode("spliceMayaNode")
  cmds.addAttr(longName='in1', attributeType="double3", multi = True)
  cmds.addAttr(longName='in1X', attributeType='double', parent='in1' )
  cmds.addAttr(longName='in1Y', attributeType='double', parent='in1' )
  cmds.addAttr(longName='in1Z', attributeType='double', parent='in1' )

  cmds.addAttr(longName='out', attributeType="double3", multi = True)
  cmds.addAttr(longName='outX', attributeType='double', parent='out' )
  cmds.addAttr(longName='outY', attributeType='double', parent='out' )
  cmds.addAttr(longName='outZ', attributeType='double', parent='out' )

  cmds.fabricSplice('addInputPort', node, 'in1', 'Vec3[]')
  cmds.fabricSplice('addOutputPort', node, 'out', 'Vec3[]')
  cmds.fabricSplice('addKLOperator', node, 'testArray')
  cmds.fabricSplice('setKLOperatorCode', node, 'testArray', """
    operator testArray(Vec3 in1[], io Vec3 out[]) {
      report('computing KL in maya!');
      report(in1);

      out.resize(in1.size());
      for(Size i = 0; i < in1.size(); ++i){
        out[i].x = in1[i].x + 1.0;
        out[i].y = in1[i].y + 1.0;
        out[i].z = in1[i].z + 1.0;
      }

      report(out);
    }
    """)

  cmds.setAttr(node + '.in1[0].in1X', 1.0)
  cmds.setAttr(node + '.in1[0].in1Y', 2.0)
  cmds.setAttr(node + '.in1[0].in1Z', 3.0)
  
  res = cmds.getAttr(node + '.out[0].outX')
  assert res == 2.0

def testOutMultiDirtying():
  from maya import cmds, OpenMaya

  cmds.file(newFile = True, force = True)

  node = cmds.createNode("spliceMayaNode")
  addMayaAttribute = True
  cmds.fabricSplice('addInputPort', node, 'in1', 'Scalar', addMayaAttribute)
  cmds.fabricSplice('addOutputPort', node, 'out', 'Scalar[]', addMayaAttribute, 'Array (Multi)')
  cmds.fabricSplice('addKLOperator', node, 'testArray')
  cmds.fabricSplice('setKLOperatorCode', node, 'testArray', """
    require Vec3;
    operator testArray(Scalar in1, io Scalar out[]) {
      report('computing KL in maya!');

      out.resize(1);
      out[0] = in1;
      report(out);
    }
    """)

  locator = cmds.spaceLocator()[0];
  cmds.connectAttr(node + '.out[0]', locator + '.translateX')

  cmds.setAttr(node + '.in1', 5.0)
  print cmds.getAttr(locator + '.translateX')

  cmds.setAttr(node + '.in1', 6.0)
  print cmds.getAttr(locator + '.translateX')

def testMatrixArray():
  from maya import cmds, OpenMaya

  cmds.file(newFile = True, force = True)

  node = cmds.createNode("spliceMayaNode")
  cmds.addAttr(longName = 'in1', attributeType = "matrix", multi = True)
  cmds.addAttr(longName = 'out', attributeType = "matrix", multi = True)
  
  cmds.fabricSplice('addInputPort', node, 'in1', 'Mat44[]')
  cmds.fabricSplice('addOutputPort', node, 'out', 'Mat44[]')
  cmds.fabricSplice('addKLOperator', node, 'testArray')
  cmds.fabricSplice('setKLOperatorCode', node, 'testArray', """
    use Xfo;

    operator testArray(Mat44 in1[], io Mat44 out[]) {
      report('computing KL in maya!');
      report(in1);

      Xfo xfo;
      xfo.setFromMat44(in1[0]);
      xfo.tr.y += 1.0;

      out.resize(1);
      out[0] = xfo.toMat44();
    }
    """)
  
  locator = cmds.spaceLocator()[0]
  cmds.connectAttr(locator + '.matrix', node + '.in1[0]')

  cmds.setAttr(locator + '.translateY', 5.0)
  assert cmds.getAttr(node + '.out[0]')[13] == 6.0

def testEulerArray():
  from maya import cmds, OpenMaya

  cmds.file(newFile = True, force = True)

  node = cmds.createNode("spliceMayaNode")
  cmds.addAttr(longName = 'in1', attributeType = "double3")
  cmds.addAttr(longName = 'in1X', attributeType = 'doubleAngle', parent = 'in1' )
  cmds.addAttr(longName = 'in1Y', attributeType = 'doubleAngle', parent = 'in1' )
  cmds.addAttr(longName = 'in1Z', attributeType = 'doubleAngle', parent = 'in1' )

  cmds.addAttr(longName = 'out', attributeType = "double3", keyable = True, storable = True)
  cmds.addAttr(longName = 'outX', attributeType = 'doubleAngle', parent = 'out' )
  cmds.addAttr(longName = 'outY', attributeType = 'doubleAngle', parent = 'out' )
  cmds.addAttr(longName = 'outZ', attributeType = 'doubleAngle', parent = 'out' )

  cmds.fabricSplice('addInputPort', node, 'in1', 'Euler')
  cmds.fabricSplice('addOutputPort', node, 'out', 'Euler')
  cmds.fabricSplice('addKLOperator', node, 'testEuler')
  cmds.fabricSplice('setKLOperatorCode', node, 'testEuler', """
    use Euler;

    operator testEuler(Euler in1, io Euler out) {
      report('computing KL in maya!');
      report(in1);

      out.x = in1.x;
    }
    """)

  locator = cmds.spaceLocator()[0]
  cmds.connectAttr(locator + '.rotate', node + '.in1')

  cmds.setAttr(locator + '.rotateX', 45.0)
  assert round(cmds.getAttr(node + '.outX')) == cmds.getAttr(locator + '.rotateX')

def testDeformer():
  from maya import cmds, OpenMaya

  cmds.file(newFile = True, force = True)
  
  sphere = cmds.polySphere()[0]
  originalPoint = cmds.pointPosition(sphere + '.pt[0]')

  deformer = cmds.deformer(type = "spliceMayaDeformer")[0]
  cmds.select(deformer, replace = True)
  cmds.addAttr(longName = 'in1', attributeType = "float", keyable = True, storable = True)

  cmds.fabricSplice('addInputPort', deformer, 'in1', 'Scalar')
  cmds.fabricSplice('addInputPort', deformer, 'mesh0', 'PolygonMesh')
  cmds.fabricSplice('addKLOperator', deformer, 'testDeformer')
  cmds.fabricSplice('setKLOperatorCode', deformer, 'testDeformer', """
    require PolygonMesh;

    operator testDeformer(Scalar in1, 
      io PolygonMesh mesh0, io Vec3 mesh0_normals[], io Vec4 mesh0_points[], io SInt32 mesh0_vcount[], io SInt32 mesh0_vlist[]) {
      report('computing KL in maya!');

      mesh0 = PolygonMesh(mesh0_points, mesh0_normals, mesh0_vcount, mesh0_vlist);
      mesh0_points[0].y += in1;
    }
    """)

  cmds.setAttr(deformer + '.in1', 5.0)
  deformedPoint = cmds.pointPosition(sphere + '.pt[0]')

  assert round(originalPoint[1] + 5.0) == round(deformedPoint[1])

def testOperatorFile():
  from maya import cmds, OpenMaya

  cmds.file(newFile = True, force = True)

  node = cmds.createNode("spliceMayaNode")
  cmds.addAttr(longName = 'in1', attributeType = "float")
  cmds.addAttr(longName = 'in2', attributeType = "float")
  cmds.addAttr(longName = 'out', attributeType = "float")

  cmds.fabricSplice('addInputPort',node,'in1','Scalar')
  cmds.fabricSplice('addInputPort',node,'in2','Scalar')
  cmds.fabricSplice('addOutputPort',node,'out','Scalar')
  cmds.fabricSplice('addKLOperator',node,'helloWorldOp')
  
  testScriptsPath = os.path.split(os.path.realpath(__file__))[0]
  testScriptFileName = os.path.join(testScriptsPath, 'test.kl')

  f = file(testScriptFileName, 'w')
  f.write("""
    operator helloWorldOp(Scalar in1, Scalar in2, io Scalar out) {
      report('computing KL in maya!');
      report(in1 + " + " + in2);
      out = in1 + in2;
    }
  """)
  f.close()

  cmds.fabricSplice('setKLOperatorFile',node,'helloWorldOp', testScriptFileName)

  cmds.setAttr(node + '.in1', 1.0)
  cmds.setAttr(node + '.in2', 2.0)
  assert cmds.getAttr(node + '.out') == 3.0

  cmds.file(rename = 'test.ma')
  cmds.file(f = True, save = True, type = 'mayaAscii')

  f = file(testScriptFileName, 'w')
  f.write("""
    operator helloWorldOp(Scalar in1, Scalar in2, io Scalar out) {
      report('computing KL in maya!');
      report(in1 + " + " + in2);
      out = in1 + in2 + 1.0;
    }
  """)
  f.close()

  cmds.file(newFile = True, force = True)
  cmds.file('test.ma', o = True)

  cmds.setAttr(node + '.in1', 2.0)
  cmds.setAttr(node + '.in2', 2.0)
  assert cmds.getAttr(node + '.out') == 5.0

def testDuplicateNode():
  from maya import cmds, OpenMaya

  cmds.file(newFile = True, force = True)
  
  node = cmds.createNode("spliceMayaNode")
  cmds.addAttr(longName = 'in1', attributeType = "float")
  cmds.addAttr(longName = 'in2', attributeType = "float")
  cmds.addAttr(longName = 'out', attributeType = "float")

  cmds.fabricSplice('addInputPort',node,'in1','Scalar')
  cmds.fabricSplice('addInputPort',node,'in2','Scalar')
  cmds.fabricSplice('addOutputPort',node,'out','Scalar')
  cmds.fabricSplice('addKLOperator',node,'helloWorldOp')
  cmds.fabricSplice('setKLOperatorCode', node, 'helloWorldOp', """
    operator helloWorldOp(Scalar in1, Scalar in2, io Scalar out) {
      report('computing KL in maya!');
      report(in1 + " + " + in2);
      out = in1 + in2;
    }
    """)

  duplicateNode = cmds.duplicate(node)[0]
  cmds.setAttr(duplicateNode + '.in1', 1.0)
  cmds.setAttr(duplicateNode + '.in2', 2.0)
  assert cmds.getAttr(duplicateNode + '.out') == 3.0

def testSpliceMayaData():
  from maya import cmds, OpenMaya

  cmds.file(newFile = True, force = True)
  
  node = cmds.createNode("spliceMayaNode")
  addMayaAttribute = True
  cmds.fabricSplice('addInputPort', node, 'in1', 'Scalar', addMayaAttribute, 'Single Value')
  cmds.fabricSplice('addInputPort', node, 'in2', 'Scalar', addMayaAttribute, 'Single Value')
  addSpliceMayaAttribute = True
  cmds.fabricSplice('addOutputPort', node, 'out', 'Bone', addMayaAttribute, 'Single Value', addSpliceMayaAttribute)
  cmds.fabricSplice('addKLOperator',node,'helloWorldOp')
  cmds.fabricSplice('setKLOperatorCode', node, 'helloWorldOp', """
    operator helloWorldOp(Scalar in1, Scalar in2, io Bone out) {
      report('computing KL in maya!1');
      report(in1 + " + " + in2);
      out.name = "hello" + String(Integer(in1));
    }
    """)

  node1 = cmds.createNode("spliceMayaNode")
  cmds.fabricSplice('addInputPort', node1, 'in1', 'Bone', addMayaAttribute, 'Single Value', addSpliceMayaAttribute)
  cmds.fabricSplice('addInputPort', node1, 'in2', 'Scalar', addMayaAttribute, 'Single Value')
  cmds.fabricSplice('addOutputPort', node1, 'out', 'String', addMayaAttribute, 'Single Value')
  cmds.fabricSplice('addKLOperator',node1,'helloWorldOp1')
  cmds.fabricSplice('setKLOperatorCode', node1, 'helloWorldOp1', """
    operator helloWorldOp1(Bone in1, Scalar in2, io String out) {
      report('computing KL in maya!2');
      report(in1.name);
      out = in1.name;
    }
    """)

  cmds.connectAttr(node + '.out', node1 + '.in1')

  cmds.setAttr(node + '.in1', 1.0)
  assert cmds.getAttr(node1 + '.out') == 'hello1'

def testVec3ArrayPersistence():
  from maya import cmds, OpenMaya

  cmds.file(newFile = True, force = True)

  node = cmds.createNode("spliceMayaNode")
  
  addMayaAttribute = True
  cmds.fabricSplice('addInputPort', node, 'in1', 'Scalar', addMayaAttribute)
  cmds.fabricSplice('addOutputPort', node, 'out', 'Scalar', addMayaAttribute)
  cmds.fabricSplice('addOutputPort', node, 'cache', 'Vec3[]', False, 'Array (Multi)')
  cmds.fabricSplice('setPortPersistence', node, 'cache', True)
  cmds.fabricSplice('addKLOperator', node, 'testArray')
  cmds.fabricSplice('setKLOperatorCode', node, 'testArray', """
    operator testArray(Scalar in1, io Scalar out, io Vec3 cache[]) {
      report('computing KL in maya!');
      report(cache);

      if(cache.size() == 0){
        cache.resize(1);
        cache[0].x = in1;
      }
      
      out = cache[0].x;
    }
    """)

  cmds.setAttr(node + '.in1', 1.0)
  assert cmds.getAttr(node + '.out') == 1.0

  cmds.file(rename = 'test.ma')
  cmds.file(f = True, save = True, type = 'mayaAscii')

  cmds.file(newFile = True, force = True)
  cmds.file('test.ma', o = True)

  cmds.setAttr(node + '.in1', 2.0)
  assert cmds.getAttr(node + '.out') == 1.0

if __name__ == '__main__':
  import maya.standalone
  maya.standalone.initialize(name='python')

  from maya import cmds
  import platform
  if platform.system() == 'Linux':
    cmds.loadPlugin('libFabricMaya' + mayaVersion)
  else:
    cmds.loadPlugin('FabricMaya' + mayaVersion)

  testGenericAttributes()
  testVec()
  testMatrix()
  testEuler()
  testBaseTypesArray()
  testVecArray()
  testOutMultiDirtying()
  testMatrixArray()
  testEulerArray()
  testDeformer()
  testOperatorFile()
  testDuplicateNode()
  testSpliceMayaData()
  testVec3ArrayPersistence()