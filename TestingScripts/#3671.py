# The result of this script should be that no output is generated.

import maya.cmds as cmds 
import maya.OpenMaya as OpenMaya 
import maya.OpenMayaMPx as OpenMayaMPx

node = cmds.createNode('spliceMayaNode') 
cmds.select(node, r=True)

# setup the compound attribute 
cmds.addAttr(node, ln='values', at='compound', nc=4) 
cmds.addAttr(node, ln='a', at='bool', p='values') 
cmds.addAttr(node, ln='b', at='long', p='values') 
cmds.addAttr(node, ln='c', at='float', p='values') 
cmds.addAttr(node, ln='d', at='double3', p='values') 
cmds.addAttr(node, ln='dX', at='double', p='d') 
cmds.addAttr(node, ln='dY', at='double', p='d') 
cmds.addAttr(node, ln='dZ', at='double', p='d')

# create a port for the compound 
cmds.fabricSplice('addIOPort', node, '{"portName":"values", "dataType":"CompoundParam", "addMayaAttr":false}') 
# create another input port to dirty and trigger node evaluation without touching the compound 
cmds.fabricSplice('addInputPort', node, '{"portName":"myInput", "dataType":"Scalar", "addMayaAttr":true}') 
# create output port 
cmds.fabricSplice('addOutputPort', node, '{"portName":"result", "dataType":"Scalar", "addMayaAttr":true}')

# dirty the second input and trigger node evaluation 
cmds.setAttr(node+'.myInput',2.0) 
cmds.getAttr(node+'.result') # used to crash Maya in 1.14.0

# [Splice] Compound attribute child 'a' exists, but not found as child KL parameter. # 
# [Splice] Compound attribute child 'b' exists, but not found as child KL parameter. # 
# [Splice] Compound attribute child 'c' exists, but not found as child KL parameter. # 
# [Splice] Compound attribute child 'd' exists, but not found as child KL parameter. # 
# Result: 0.0 #

# now set a child of the compound attr and trigger evaluation again 
cmds.setAttr(node+'.a', True) 
cmds.getAttr(node+'.result')

# [KL]: [FABRIC:MT:mayaGraph_DGNode:testCompoundOp] a is of type Boolean # 
# [KL]: [FABRIC:MT:mayaGraph_DGNode:testCompoundOp] b is of type SInt32 # 
# [KL]: [FABRIC:MT:mayaGraph_DGNode:testCompoundOp] c is of type Float64 # 
# [KL]: [FABRIC:MT:mayaGraph_DGNode:testCompoundOp] d is of type Vec3 # 
# Result: 0.0 #

