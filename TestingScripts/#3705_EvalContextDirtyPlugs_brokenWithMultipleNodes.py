
import json
from maya import cmds

cmds.file(new=True,f=True)


##############################################
## Set up the loader node.

influenceGeoms_Node = cmds.createNode("spliceMayaNode", name = "captainAtom_InfluenceGeoms")

cmds.fabricSplice('addInputPort', influenceGeoms_Node, json.dumps({'portName':'filePath', 'dataType':'String', 'addMayaAttr': True}))
cmds.fabricSplice('addInputPort', influenceGeoms_Node, json.dumps({'portName':'displaySkinningDebugging', 'dataType':'Boolean', 'addMayaAttr': True}))
cmds.fabricSplice('addInputPort', influenceGeoms_Node, json.dumps({'portName':'iterations', 'dataType':'Integer', 'addMayaAttr': True}))
cmds.fabricSplice('addInputPort', influenceGeoms_Node, json.dumps({'portName':'useDeltaMushMask', 'dataType':'Boolean', 'addMayaAttr': True}))
cmds.fabricSplice('addInputPort', influenceGeoms_Node, json.dumps({'portName':'displayDeltaMushMask', 'dataType':'Boolean', 'addMayaAttr': True}))
cmds.fabricSplice('addInputPort', influenceGeoms_Node, json.dumps({'portName':'displayDeltaMushDebugging', 'dataType':'Boolean', 'addMayaAttr': True}))
cmds.fabricSplice('addInputPort', influenceGeoms_Node, json.dumps({'portName':'displayGeometries', 'dataType':'Boolean', 'addMayaAttr': True}))

cmds.fabricSplice('addOutputPort', influenceGeoms_Node, json.dumps({'portName':'eval', 'dataType':'Scalar', 'addMayaAttr': True}))

cmds.fabricSplice('addKLOperator', influenceGeoms_Node, '{"opName":"captainAtom_InfluenceGeoms"}', """

require BulletHelpers;

operator captainAtom_InfluenceGeoms(
  String filePath,
  Boolean displaySkinningDebugging,
  Integer iterations,
  Boolean useDeltaMushMask,
  Boolean displayDeltaMushMask,
  Boolean displayDeltaMushDebugging,
  Boolean displayGeometries,
  EvalContext context
) {
  report("captainAtom_InfluenceGeoms:" + context._dirtyInputs);
}
""")



##############################################
## Set up the loader node for the render geoms

renderGeoms_Node = cmds.createNode("spliceMayaNode", name = "captainAtom_RenderGeoms")

cmds.fabricSplice('addInputPort', renderGeoms_Node, json.dumps({'portName':'filePath', 'dataType':'String', 'addMayaAttr': True}))
cmds.fabricSplice('addInputPort', renderGeoms_Node, json.dumps({'portName':'displayDebugging', 'dataType':'Boolean', 'addMayaAttr': True}))
cmds.fabricSplice('addInputPort', renderGeoms_Node, json.dumps({'portName':'displayGeometries', 'dataType':'Boolean', 'addMayaAttr': True}))
cmds.fabricSplice('addInputPort', renderGeoms_Node, json.dumps({'portName':'ineval', 'dataType':'Scalar', 'addMayaAttr': True}))
cmds.fabricSplice('addOutputPort', renderGeoms_Node, json.dumps({'portName':'eval', 'dataType':'Scalar', 'addMayaAttr': True}))
cmds.setAttr(renderGeoms_Node + '.displayGeometries', 1);

cmds.fabricSplice('addKLOperator', renderGeoms_Node, '{"opName":"captainAtom_RenderGeoms"}', """

require BulletHelpers;

operator captainAtom_RenderGeoms(
  String filePath,
  Boolean displayDebugging,
  Boolean displayGeometries,
  EvalContext context,
  Scalar eval
) {
  report("captainAtom_RenderGeoms:" + context._dirtyInputs);
}
""")

##############################################
## Set up the eval locator.

forceEvalLocator = cmds.createNode("locator", name = "forceEval")

cmds.connectAttr(influenceGeoms_Node + '.eval', renderGeoms_Node + '.ineval')
cmds.connectAttr(renderGeoms_Node + '.eval', forceEvalLocator + '.localPosition.localPositionY')
