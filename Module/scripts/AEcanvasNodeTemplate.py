from maya import cmds, OpenMaya, OpenMayaUI
from pymel.core import *

class AEcanvasBaseTemplate(ui.AETemplate):
  def __init__(self, nodeName):
    ui.AETemplate.__init__(self, nodeName)

    self.__nodeName = nodeName

    cmds.editorTemplate(beginScrollLayout = True, collapse = False)
   
    # DFG layout containing the 'Open DFG Editor' button
    cmds.editorTemplate(beginLayout = "Fabric Engine - Canvas", collapse = False)
    self.callCustom(self.new, self.replace, '')
    cmds.editorTemplate(endLayout = True)
   
    # Add the other attributes (among which are dynamic attributes for the dfg node)
    # to the end of the template before the 'endScrollLayout' so Maya doesn't mess
    # up the scroll area.
    self.addExtraControls(label="Other Attributes")
   
    # Wrap up the node behaviour in it's default mode
    mel.AEdependNodeTemplate(self.__nodeName)
   
    cmds.editorTemplate(endScrollLayout = True)

  def __openDFGEditor(self, arg):

    # [FE-5728]
    # look for the first 'canvas' node in the current selection and store its name in the variable nodeName.
    # (note: we cannot use self.__nodeName as it only gets set once per session)
    nodes = list(set(cmds.ls(sl=True)))
    numRel = 3 + len(nodes)
    nodeName = ""
    for node in nodes:
      # is this a Canvas node?
      nodeType = cmds.nodeType(node)
      if nodeType.startswith('canvas'):
        nodeName = node
        break
      # add relative/connected nodes to 'nodes'.
      if numRel:
        numRel -= 1
        relcons = cmds.listRelatives(node, shapes=True)
        if relcons:
          relcons = list(set(relcons))
          for rc in relcons:
            if rc in nodes:
              relcons.remove(rc)
          nodes += relcons
        relcons = cmds.listConnections(node, destination=True)
        if relcons:
          relcons = list(set(relcons))
          for rc in relcons:
            if rc in nodes:
              relcons.remove(rc)
          nodes += relcons

    # check the result.
    if not nodeName:
      # no matching node was found.
      cmds.warning("unable to find a Canvas node in the current selection.")
    else:
      if cmds.referenceQuery(nodeName, isNodeReferenced=True):
        # the node is a reference node.
        cmds.confirmDialog(t="Canvas", m="Editing Canvas graphs is not possible with referenced nodes.", ma="center", b="Okay", icn="information")
      else:
        # looking good => open Canvas.
        cmds.fabricDFG(action="showUI", node=nodeName)
   
  def new(self, attr):
    cmds.setUITemplate("attributeEditorTemplate", pushTemplate=True)
    cmds.button(label='Open Canvas', command=self.__openDFGEditor)
    cmds.setUITemplate(popTemplate=True)
   
  def replace(self, attr):
    pass

class AEcanvasNodeTemplate(AEcanvasBaseTemplate):
  _nodeType = 'canvasNode'

class AEcanvasDeformerTemplate(AEcanvasBaseTemplate):
  _nodeType = 'canvasDeformer'
