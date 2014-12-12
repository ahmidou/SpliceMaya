# The result of this script should be that no output is generated.

import maya.cmds as cmds 
import maya.OpenMaya as OpenMaya 
import maya.OpenMayaMPx as OpenMayaMPx

node = cmds.createNode('spliceMayaNode') 
cmds.select(node, r=True)


# create a port for the compound 
cmds.fabricSplice('addIOPort', node, '{"portName":"value1", "dataType":"Scalar", "addMayaAttr":true}') 
cmds.fabricSplice('addIOPort', node, '{"portName":"value2", "dataType":"Boolean", "addMayaAttr":true}') 
# create output port 
cmds.fabricSplice('addOutputPort', node, '{"portName":"result", "dataType":"Scalar", "addMayaAttr":true}')

# dirty the first input and trigger node evaluation 
cmds.setAttr(node+'.value1',2.0) 
cmds.getAttr(node+'.result') # used to crash Maya in 1.14.0


#  trigger evaluation again 
cmds.setAttr(node+'.value2', True) 
cmds.getAttr(node+'.result')
