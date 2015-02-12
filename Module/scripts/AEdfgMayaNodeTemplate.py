from maya import cmds, OpenMaya, OpenMayaUI
from pymel.core import *

class AEdfgMayaBaseTemplate(ui.AETemplate):
  def __init__(self, nodeName):
    ui.AETemplate.__init__(self, nodeName)

    self.__nodeName = nodeName

    cmds.editorTemplate(beginScrollLayout = True, collapse = False)
   
    # DFG layout containing the 'Open DFG Editor' button
    cmds.editorTemplate(beginLayout = "DFG", collapse = False)
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
    selectedNames = cmds.ls(sl=True)
    nodeName = self.__nodeName
    for selectedName in selectedNames:
      nodeType = cmds.nodeType(selectedName)
      if nodeType.startswith('dfg'):
        nodeName = selectedName
        break
    cmds.fabricDFG(action="showUI", node=nodeName)
   
  def new(self, attr):
    cmds.setUITemplate("attributeEditorTemplate", pushTemplate=True)
    cmds.button(label='Open DFG Editor', command=self.__openDFGEditor)
    cmds.setUITemplate(popTemplate=True)
   
  def replace(self, attr):
    pass

class AEdfgMayaNodeTemplate(AEdfgMayaBaseTemplate):
  _nodeType = 'dfgMayaNode'
