import maya.cmds as cmds
import maya.OpenMaya as om

nodes = cmds.ls(type="dfgMayaNode")
nodes += cmds.ls(type="canvasNode")

sel = om.MSelectionList()
for node in nodes:
  sel.add(node)

for i in range(sel.length()):

  node = om.MObject()
  sel.getDependNode(i, node)
  node = om.MFnDependencyNode(node)
  
  attrs = cmds.listAttr(node.name(), userDefined=True)

  compounds = []  
  for attr in attrs:
    plug = node.findPlug(attr)
    if not plug.isCompound():
        continue
    compounds += [attr]
   
  cmds.select(node.name())
  for attr in compounds:
    plug = node.findPlug(attr)
    count = plug.numChildren()
      
    lastChild = plug.child(count-1)

    writable = cmds.attributeQuery(attr, node=node.name(), writable=True)
    readable = cmds.attributeQuery(attr, node=node.name(), readable=True)

    srcConns = om.MPlugArray()
    dstConns = om.MPlugArray()
    plug.connectedTo(srcConns, True, False)
    plug.connectedTo(dstConns, False, True)

    names = []
    paths = []
    types = []
    values = []
    childSrcConns = []
    childDstConns = []

    for i in range(count):
      child = plug.child(i)
      names += [child.name().partition('.')[2]]
      paths += [child.name()]
      values += [cmds.getAttr(child.name())]
      types += [cmds.getAttr(child.name(), type=True)]

      childSrcConn = om.MPlugArray()
      childDstConn = om.MPlugArray()
      child.connectedTo(childSrcConn, True, False)
      child.connectedTo(childDstConn, False, True)
      childSrcConns += [childSrcConn]
      childDstConns += [childDstConn]
    
    cmds.deleteAttr(node.name(), attribute=attr)

    # we might need a special case her for Vec3??
    cmds.addAttr(longName=attr, numberOfChildren=count, attributeType="compound", writable=writable, readable=readable) 
      
    for i in range(count):
      cmds.addAttr(longName=names[i], attributeType=types[i], parent=attr, writable=writable, readable=readable)

    plug = node.findPlug(attr)

    for i in range(srcConns.length()):
      cmds.connectAttr(dstConns[i].name(), plug.name())
    for i in range(dstConns.length()):
      cmds.connectAttr(plug.name(), dstConns[i].name())
    for i in range(count):
      cmds.setAttr(paths[i], values[i])
      for j in range(childSrcConns[i].length()):
        cmds.connectAttr(childSrcConns[i][j].name(), paths[i])
      for j in range(childDstConns[i].length()):
        cmds.connectAttr(paths[i], childDstConns[i][j].name())
