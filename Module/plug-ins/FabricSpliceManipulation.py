import sys
import json
import maya.cmds as cmds
import maya.OpenMaya as OpenMaya
import maya.OpenMayaMPx as OpenMayaMPx

class fabricSpliceManipCmd(OpenMayaMPx.MPxCommand):

  undoDataStack = []
  markedAttributes = []

  def __init__(self):
    OpenMayaMPx.MPxCommand.__init__(self)

  def isUndoable(self):
    return self.state == 0

  def doIt(self, argList):
    # self.data = None
    # self.undoData = None
    # if argList.length() == 0:
    #   self.setResult(0)
    #   return
    # jsonData = argList.asString(0)
    # if not jsonData:
    #   self.setResult(0)
    #   return
    # data = None
    # try:
    #   data = json.loads(jsonData.replace("'", '"'))
    # except ValueError as e:
    #   print str(e)
    #   self.setResult(0)
    #   return

    # self.data = data

    # self.state = int(self.data['context']['state'])
    # if self.state == 0:
    #   fabricSpliceManipCmd.undoDataStack.append({
    #     'names': [],
    #     'oldValues': [],
    #     'newValues': []
    #   })
    # self.undoData = fabricSpliceManipCmd.undoDataStack[len(fabricSpliceManipCmd.undoDataStack)-1]

    # # get the splice node to perform the changes on
    # self.spliceNode = self.data['target']
    # if not self.spliceNode:
    #   self.setResult(0)
    #   return
    # selList = OpenMaya.MSelectionList()
    # selList.add(self.spliceNode)
    # spliceNodeObj = OpenMaya.MObject()
    # selList.getDependNode(0, spliceNodeObj)
    # if spliceNodeObj.isNull():
    #   self.setResult(0)
    #   return
    # self.spliceNodeDepNode = OpenMaya.MFnDependencyNode(spliceNodeObj)

    # if self.state == 0: # setup

    #   # check if shift was pressed
    #   modifierKey = int(self.data['context']['modifierKey'])
    #   if not modifierKey & 1: # Modifier_Key_Shift
    #     # remove the keyable setting
    #     prevMarkedAttributes = []
    #     if len(fabricSpliceManipCmd.markedAttributes) > 0:
    #       prevMarkedAttributes = fabricSpliceManipCmd.markedAttributes[len(fabricSpliceManipCmd.markedAttributes)-1]
    #     for attribute in prevMarkedAttributes:
    #       cmds.setAttr(attribute, keyable=False)

    #   # put it onto the stack
    #   self.data['markedAttributes'] = []
    #   fabricSpliceManipCmd.markedAttributes.append(self.data['markedAttributes'])

    #   paramNames = self.data['result']['paramNames']
    #   for paramName in paramNames:
    #     plug = self.spliceNodeDepNode.findPlug(paramName)
    #     if not plug or plug.isNull():
    #       continue

    #     self.undoData['names'].append(plug.name())
    #     value = cmds.getAttr(plug.name())
    #     self.undoData['oldValues'].append(value)
    #     self.undoData['newValues'].append(value)

    #     self.data['markedAttributes'].append(self.spliceNode+'.'+paramName)

    #   for attribute in self.data['markedAttributes']:
    #     cmds.setAttr(attribute, keyable=True)

    #   # select the node
    #   cmds.select(self.spliceNode)

    #   # refresh the animation editor eventually
    #   whichPanel = cmds.getPanel(scriptType = "graphEditor")
    #   if len(whichPanel) > 0:
    #     graphEd = whichPanel[0] + 'GraphEd'
    #     outlineEd = whichPanel[0] + "OutlineEd"
    #     outlineConn = whichPanel[0] + "FromOutliner"
    #     showAnimCurveOnly = cmds.outlinerEditor(outlineEd, query=True, showAnimCurvesOnly=True)
    #     cmds.outlinerEditor(outlineEd, edit=True, showAnimCurvesOnly=(not showAnimCurveOnly))
    #     cmds.outlinerEditor(outlineEd, edit=True, showAnimCurvesOnly=showAnimCurveOnly)

    # if self.state == 1: # drag
    #   paramNames = self.data['result']['paramNames']
    #   paramValues = self.data['result']['paramValues']
    #   for i in range(len(paramNames)):
    #     self.undoData['newValues'][i] = float(paramValues[i])
    #   self.setResult(1)
    #   self.redoIt()
    #   cmds.refresh()
    # else:
    #   self.setResult(0)

    return 0

  def redoIt(self):
    # if not self.data:
    #   return
    # for i in range(len( self.undoData['names'])):
    #   cmds.setAttr(self.undoData['names'][i], self.undoData['newValues'][i])
    return 0

  def undoIt(self):
    # if not self.data:
    #   return
    # for i in range(len( self.undoData['names'])):
    #   cmds.setAttr(self.undoData['names'][i], self.undoData['oldValues'][i])
    return 0

def fabricSpliceManipCmdCreator():
  return OpenMayaMPx.asMPxPtr( fabricSpliceManipCmd() )

# Initialize the script plug-in
def initializePlugin(mobject):
  mplugin = OpenMayaMPx.MFnPlugin(mobject)
  try:
    mplugin.registerCommand( "fabricSpliceManip", fabricSpliceManipCmdCreator )
  except Exception as e:
    sys.stderr.write( "Failed to initializePlugin: %s\n" % str(e) )

  # try:
  #   cmds.fabricSplice("setGlobalManipulationCommand", "fabricSpliceManip")
  # except Exception as e:
  #   pass

# Uninitialize the script plug-in
def uninitializePlugin(mobject):
  mplugin = OpenMayaMPx.MFnPlugin(mobject)
  try:
    mplugin.deregisterCommand( "fabricSpliceManip" )
  except Exception as e:
    sys.stderr.write( "Failed to uninitializePlugin: %s\n" % str(e) )

  # try:
  #   cmds.fabricSplice("setGlobalManipulationCommand", "")
  # except Exception as e:
  #   pass
