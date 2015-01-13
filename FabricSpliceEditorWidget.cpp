
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QFont>
#include <QtGui/QPushButton>
#include <QtGui/QColor>
#include <QtGui/QFileDialog>
#include <QtGui/QTabWidget>
#include <QtGui/QDockWidget>

#include "FabricSpliceEditorWidget.h"
#include "FabricSpliceMayaNode.h"
#include "FabricSpliceKLSourceCodeWidget.h"
#include "FabricSpliceLogoWidget.h"
#include "FabricSpliceAddOpDialog.h"
#include "FabricSpliceConfirmDialog.h"
#include "FabricSplicePortDialog.h"
#include "plugin.h"
#include <FabricSplice.h>

#include <maya/MGlobal.h>
#include <maya/MCommandResult.h>
#include <maya/MFnNumericAttribute.h>

#include <sstream>

FabricSpliceEditorWidget::widgetMap FabricSpliceEditorWidget::gWidgets;
bool FabricSpliceEditorWidget::gUpdatePosted = false;

using namespace FabricSplice;

std::string stripString(const std::string & name, char charToRemove)
{
  if(name.length() == 0)
    return name;

  size_t start = 0;
  size_t end = name.length()-1;

  while(name[start] == charToRemove && start != end)
    start++;
  while(name[end] == charToRemove && start != end)
    end--;

  return name.substr(start, end - start + 1);
}

std::vector<std::string> splitString(const std::string & name, char delimiter, bool stripSpaces)
{
  std::stringstream ss(name);
  std::string item;
  std::vector<std::string> result;
  while(std::getline(ss, item, delimiter))
  {
    if(stripSpaces)
      result.push_back(stripString(item, ' '));
    else
      result.push_back(item);
  }
  return result;
}

FabricSpliceEditorWidget::FabricSpliceEditorWidget(QWidget * parent, const QString & name)
:QTabWidget(parent)
{
#ifdef _WIN32
    mName = name.toLocal8Bit().constData();
#else
    mName = name.toUtf8().constData();
#endif
  if(gWidgets.find(mName) == gWidgets.end())
    gWidgets.insert(widgetPair(mName, this));

  setMinimumWidth(250);
  setMinimumHeight(250);
  resize(380, 250);

  QWidget * page1 = new QWidget(this);
  QWidget * page2 = new QWidget(this);

  addTab(page1, "Ports");
  addTab(page2, "KL Editor");

  // setup layout
  page1->setLayout(new QVBoxLayout());
  page2->setLayout(new QVBoxLayout());

  page1->layout()->setContentsMargins(3, 3, 3, 3);
  page2->layout()->setContentsMargins(3, 3, 3, 3);

  // setup sub widgets
  MString moduleFolder = getModuleFolder();
  MString logoPath = moduleFolder + "/ui/FE_logo.png";
  FabricSpliceLogoWidget * logo1 = new FabricSpliceLogoWidget(logoPath.asChar(), this);
  FabricSpliceLogoWidget * logo2 = new FabricSpliceLogoWidget(logoPath.asChar(), this);
  page1->layout()->addWidget(logo1);
  page2->layout()->addWidget(logo2);

  mNodeCombo = new FabricSpliceComboWidget(page1);
  mNodeCombo->setEditable(true);
  page1->layout()->addWidget(mNodeCombo);

  QLabel * label = new QLabel("Contained Ports", page1);
  page1->layout()->addWidget(label);

  mAttrList = new FabricSpliceListWidget(page1);
  page1->layout()->addWidget(mAttrList);
  mAttrList->setFont(QFont("Courier"));

  QWidget * attrListButtons = new QWidget(page1);
  page1->layout()->addWidget(attrListButtons);

  attrListButtons->setLayout(new QHBoxLayout());
  attrListButtons->layout()->setContentsMargins(0, 0, 0, 0);

  mAddAttrButton = new FabricSpliceButtonWidget("Add", attrListButtons);
  attrListButtons->layout()->addWidget(mAddAttrButton);
  mRemoveAttrButton = new FabricSpliceButtonWidget("Remove", attrListButtons);
  attrListButtons->layout()->addWidget(mRemoveAttrButton);

  label = new QLabel("Contained Operators", page1);
  page1->layout()->addWidget(label);

  mOperatorList = new FabricSpliceListWidget(page1);
  page1->layout()->addWidget(mOperatorList);
  mOperatorList->setFont(QFont("Courier"));

  QWidget * opListButtons = new QWidget(page1);
  page1->layout()->addWidget(opListButtons);

  opListButtons->setLayout(new QHBoxLayout());
  opListButtons->layout()->setContentsMargins(0, 0, 0, 0);

  mAddOpButton = new FabricSpliceButtonWidget("Add", opListButtons);
  opListButtons->layout()->addWidget(mAddOpButton);
  mEditOpButton = new FabricSpliceButtonWidget("Edit", opListButtons);
  opListButtons->layout()->addWidget(mEditOpButton);
  mRemoveOpButton = new FabricSpliceButtonWidget("Remove", opListButtons);
  opListButtons->layout()->addWidget(mRemoveOpButton);

  label = new QLabel("Available Ports", page2);
  page2->layout()->addWidget(label);

  mPortText = new QTextEdit(page2);
  mPortText->setReadOnly(true);
  // mPortText->setTextColor(QColor(0, 0, 0));
  mPortText->setMaximumHeight(45);
  page2->layout()->addWidget(mPortText);

  label = new QLabel("KL SourceCode", page2);
  page2->layout()->addWidget(label);

  int fontSize = 10;
  mSourceCode = new FabricSpliceKLSourceCodeWidget(page2, fontSize);
  page2->layout()->addWidget(mSourceCode);

  mErrorLog = new QTextEdit(page2);
  mErrorLog->setReadOnly(true);
  mErrorLog->setTextColor(QColor(255, 0, 0));
  mErrorLog->setMaximumHeight(65);
  mErrorLog->setFontPointSize(fontSize);
  page2->layout()->addWidget(mErrorLog);
  mErrorLog->hide();

  QWidget * klButtons = new QWidget(page2);
  page2->layout()->addWidget(klButtons);

  klButtons->setLayout(new QHBoxLayout());
  klButtons->layout()->setContentsMargins(0, 0, 0, 0);

  mCompileButton = new FabricSpliceButtonWidget("Compile", klButtons);
  klButtons->layout()->addWidget(mCompileButton);

  // callbacks
  mNodeCombo->setCallback(onNodeChanged, this);
  mAddAttrButton->setCallback(addAttrPressed, this);
  mRemoveAttrButton->setCallback(removeAttrPressed, this);
  mAddOpButton->setCallback(addOpPressed, this);
  mEditOpButton->setCallback(editOpPressed, this);
  mRemoveOpButton->setCallback(removeOpPressed, this);
  mCompileButton->setCallback(compilePressed, this);

  // update all widgets
  update();
  onOperatorChanged(this);
  onNodeChanged(this);
}

FabricSpliceEditorWidget::~FabricSpliceEditorWidget()
{
}

FabricSpliceEditorWidget * FabricSpliceEditorWidget::getFirst()
{
  widgetIt it = gWidgets.begin();
  if(it != gWidgets.end())
    return it->second;
  return NULL;
}

FabricSpliceEditorWidget * FabricSpliceEditorWidget::getFirstOrOpen(bool onIdle)
{
  std::vector<std::string> toRemove;
  for(widgetIt it = gWidgets.begin(); it != gWidgets.end(); it++)
  {
    if(!it->second->isVisible())
      toRemove.push_back(it->first);
  }
  for(size_t i=0;i<toRemove.size();i++)
  {
    widgetIt it = gWidgets.find(toRemove[i]);
    if(it != gWidgets.end())
      it->second->close();
  }

  FabricSpliceEditorWidget * widget = getFirst();
  if(widget == NULL)
  {
    MString cmd = "source \"FabricSpliceUI.mel\"; showSpliceEditor(\"";
    cmd += getPluginName();
    cmd += "\");";
    MStatus commandStatus;
    if(onIdle)
      commandStatus = MGlobal::executeCommandOnIdle(cmd, false);
    else
      commandStatus = MGlobal::executeCommand(cmd, false);
    if (commandStatus != MStatus::kSuccess)
      MGlobal::displayError("Failed to load FabricSpliceUI.mel. Adjust your MAYA_SCRIPT_PATH!");
    widget = getFirst();
    if(widget)
      widget->bringToFront();
  }
  return widget;
}

std::string FabricSpliceEditorWidget::getCurrentNodeName()
{
  return mNodeCombo->getStdText();
}

FabricSpliceBaseInterface * FabricSpliceEditorWidget::getCurrentBaseInterface()
{
  std::string nodeName = getCurrentNodeName();
  if(nodeName.length() == 0)
    return NULL;

  FabricSpliceBaseInterface * node = FabricSpliceBaseInterface::getInstanceByName(nodeName);
  if(node == NULL)
    return NULL;
  return node;
}

void FabricSpliceEditorWidget::onNodeChanged(void * userData)
{
  MStatus status;
  MAYASPLICE_CATCH_BEGIN(&status);

  FabricSpliceEditorWidget * editor = (FabricSpliceEditorWidget*)userData;
  editor->mAttrList->clear();
  editor->mOperatorList->clear();

  FabricSpliceBaseInterface * interf = editor->getCurrentBaseInterface();

  editor->mNodeCombo->setEnabled(interf != NULL);
  editor->mAddAttrButton->setEnabled(interf != NULL);
  editor->mRemoveAttrButton->setEnabled(interf != NULL);
  editor->mAddOpButton->setEnabled(interf != NULL);
  editor->mEditOpButton->setEnabled(interf != NULL);
  editor->mRemoveOpButton->setEnabled(interf != NULL);
  editor->mCompileButton->setEnabled(interf != NULL);

  if(interf == NULL)
  {
    onOperatorChanged(editor);
    return;
  }

  std::string portText = interf->getSpliceGraph().generateKLOperatorParameterList().getStringData();

  MStringArray ports = interf->getPortNames();
  for(unsigned int i=0;i<ports.length();i++)
  {
    FabricSplice::DGPort port = interf->getPort(ports[i].asChar());

    std::string dataType = port.getDataType();
    std::string arraySuffix;
    if(port.isArray())
      arraySuffix += "[]";
    std::string member = port.getMember();

    std::string itemText = member + " - " + dataType + arraySuffix;
    if(port.getMode() == FabricSplice::Port_Mode_IN)
      itemText = "--> "+itemText;
    else if(port.getMode() == FabricSplice::Port_Mode_OUT)
      itemText = "<-- "+itemText;
    else
      itemText = "<-> "+itemText;

    editor->mAttrList->addItem(itemText.c_str());
  }
  editor->mPortText->setText(portText.c_str());

  editor->mOpName = "";
  MStringArray operators = interf->getKLOperatorNames();
  for(unsigned int i=0;i<operators.length();i++)
  {
    editor->mOperatorList->addItem(operators[i].asChar());
    if(i == 0)
    {
      MStringArray parts;
      operators[i].split('-', parts);
      editor->mOpName = parts[1].substring(1, parts[1].length()).asChar();
    }
  }

  MAYASPLICE_CATCH_END(&status);
}

void FabricSpliceEditorWidget::onOperatorChanged(void * userData)
{
  MStatus status;
  MAYASPLICE_CATCH_BEGIN(&status);

  FabricSpliceEditorWidget * editor = (FabricSpliceEditorWidget*)userData;
  std::string opName = editor->mOpName;

  FabricSpliceBaseInterface * interf = editor->getCurrentBaseInterface();

  std::string code;
  if(interf != NULL && opName.length() > 0)
    code = interf->getSpliceGraph().getKLOperatorSourceCode(opName.c_str());
  editor->mSourceCode->setSourceCode(opName, code);
  editor->mSourceCode->setEnabled(!interf->getSpliceGraph().isKLOperatorFileBased(opName.c_str()));
  editor->mErrorLog->setText("");
  editor->mErrorLog->hide();

  MAYASPLICE_CATCH_END(&status);
}

void FabricSpliceEditorWidget::compilePressed(void * userData)
{
  FabricSpliceEditorWidget * editor = (FabricSpliceEditorWidget*)userData;

  std::string opName = editor->mOpName;

  // using STL here was crashing due to some string destructor problem
  std::string code(editor->mSourceCode->getSourceCode());
  editor->mErrorLog->setText("");
  editor->mErrorLog->hide();

  MStatus status;
  MAYASPLICE_CATCH_BEGIN(&status);

  FabricSpliceBaseInterface * node = editor->getCurrentBaseInterface();
  if(node == NULL)
    return;

  node->setKLOperatorCode(opName.c_str(), code.c_str(), "");

  MAYASPLICE_CATCH_END(&status);
}

void FabricSpliceEditorWidget::addAttrPressed(void * userData)
{
  MStatus status;
  MAYASPLICE_CATCH_BEGIN(&status);

  FabricSpliceEditorWidget * editor = (FabricSpliceEditorWidget*)userData;

  FabricSplicePortDialog dialog(editor);
  dialog.exec();

  if(dialog.succeeded())
  {
    std::string name;
    int mode = -1;
    std::string dataType;
    std::string extension;
    std::string arrayType;
    bool addAttribute = false;
    bool useSpliceMayaData = false;
    int persistenceType = 0;

    FabricCore::Variant nameVar = dialog.getValue("name");
    FabricCore::Variant modeVar = dialog.getValue("mode");
    FabricCore::Variant dataTypeVar = dialog.getValue("dataType");
    FabricCore::Variant extensionVar = dialog.getValue("extension");
    FabricCore::Variant arrayTypeVar = dialog.getValue("arrayType");
    FabricCore::Variant addAttributeVar = dialog.getValue("addAttribute");
    FabricCore::Variant useSpliceMayaDataVar = dialog.getValue("useSpliceMayaData");
    FabricCore::Variant persistenceTypeVar = dialog.getValue("persistenceType");
    if(nameVar.isString())
      name = nameVar.getStringData();
    if(modeVar.isSInt32())
      mode = modeVar.getSInt32();
    if(dataTypeVar.isString())
      dataType = dataTypeVar.getStringData();
    if(extensionVar.isString())
      extension = extensionVar.getStringData();
    if(arrayTypeVar.isString())
      arrayType = arrayTypeVar.getStringData();
    if(addAttributeVar.isBoolean())
      addAttribute = addAttributeVar.getBoolean();
    if(useSpliceMayaDataVar.isBoolean())
    {
      useSpliceMayaData = useSpliceMayaDataVar.getBoolean();
      addAttribute = addAttribute || useSpliceMayaData;
    }
    if(persistenceTypeVar.isSInt32())
      persistenceType = persistenceTypeVar.getSInt32();

    if(name.length() == 0 || mode < 0 || dataType.length() == 0 || arrayType.length() == 0)
      return;

    FabricSpliceBaseInterface * node = editor->getCurrentBaseInterface();
    if(node == NULL)
      return;

    bool portPersistence = false;
    if(dataType == "Boolean")
      portPersistence = true;
    else if(dataType == "Integer")
      portPersistence = true;
    else if(dataType == "Scalar")
      portPersistence = true;
    else if(dataType == "String")
      portPersistence = true;
    else if(dataType == "Vec3")
      portPersistence = true;
    else if(dataType == "Euler")
      portPersistence = true;
    else if(dataType == "Mat44")
      portPersistence = true;

    if(extension.length() == 0)
    {
      if(dataType == "KeyframeTrack")
        extension = "Animation";
    }

    if(arrayType != "Single Value")
      dataType += "[]";

    node->addPort(name.c_str(), dataType.c_str(), (FabricSplice::Port_Mode)mode, "DGNode", true, extension.c_str(), FabricCore::Variant());

    DGPort port = node->getPort(name.c_str());
    if(arrayType == "Array (Native)")
      port.setOption("nativeArray", FabricCore::Variant::CreateBoolean(true));

    if(addAttribute){
      if(useSpliceMayaData){
        dataType = "SpliceMayaData";
        port.setOption("opaque", FabricCore::Variant::CreateBoolean(true));
      }
      node->addMayaAttribute(name.c_str(), dataType.c_str(), arrayType.c_str(), (FabricSplice::Port_Mode)mode);
    } else {
      port.setOption("internal", FabricCore::Variant::CreateBoolean(true));
    }

    if(persistenceType != 0){
      node->setPortPersistence(MString(name.c_str()), persistenceType == 1);
    } else if(portPersistence){
      node->setPortPersistence(MString(name.c_str()), true);
    }

    editor->update();
  }

  MAYASPLICE_CATCH_END(&status);
}

void FabricSpliceEditorWidget::removeAttrPressed(void * userData)
{
  MStatus status;
  MAYASPLICE_CATCH_BEGIN(&status);

  FabricSpliceEditorWidget * editor = (FabricSpliceEditorWidget*)userData;

  std::string portName = editor->mAttrList->getStdText();
  if(portName.length() == 0)
    return;
  portName = portName.substr(4, portName.find(' ', 4) - 4);

  FabricSpliceConfirmDialog dialog(editor);
  dialog.exec();

  if(dialog.succeeded())
  {
    FabricSpliceBaseInterface * node = editor->getCurrentBaseInterface();
    if(node == NULL)
      return;
    node->removeMayaAttribute(portName.c_str());
    node->removePort(portName.c_str());
    editor->update();
  }

  MAYASPLICE_CATCH_END(&status);
}

void FabricSpliceEditorWidget::addOpPressed(void * userData)
{
  MStatus status;

  FabricSpliceEditorWidget * editor = (FabricSpliceEditorWidget*)userData;

  FabricSpliceAddOpDialog dialog(editor);
  dialog.exec();

  if(dialog.succeeded())
  {
    FabricCore::Variant nameVar = dialog.getValue("name");
    if(nameVar.isString())
    {
      std::string name = nameVar.getStringData();
      FabricSpliceBaseInterface * node = editor->getCurrentBaseInterface();
      if(node == NULL)
        return;

      std::string code = node->getSpliceGraph().generateKLOperatorSourceCode(name.c_str()).getStringData();

      MAYASPLICE_CATCH_BEGIN(&status);
        node->addKLOperator(name.c_str(), code.c_str(), "", "DGNode", FabricCore::Variant::CreateDict());
      MAYASPLICE_CATCH_END(&status);

      MAYASPLICE_CATCH_BEGIN(&status);
        code = node->getSpliceGraph().getKLOperatorSourceCode(name.c_str());
        editor->mSourceCode->setSourceCode(name, code);
        editor->mSourceCode->setEnabled(!node->getSpliceGraph().isKLOperatorFileBased(name.c_str()));
      MAYASPLICE_CATCH_END(&status);

      editor->update();
      editor->setOperatorName(name);
    }
  }
}

void FabricSpliceEditorWidget::editOpPressed(void * userData)
{
  MStatus status;
  MAYASPLICE_CATCH_BEGIN(&status);

  FabricSpliceEditorWidget * editor = (FabricSpliceEditorWidget*)userData;
  MString opName = editor->mOperatorList->getStdText().c_str();
  if(opName.length() == 0)
    return;

  MStringArray parts;
  opName.split('-', parts);
  opName = parts[1].substring(1, parts[1].length());

  editor->setOperatorName(opName.asChar());
  editor->setCurrentIndex(1);

  MAYASPLICE_CATCH_END(&status);
}

void FabricSpliceEditorWidget::removeOpPressed(void * userData)
{
  MStatus status;
  MAYASPLICE_CATCH_BEGIN(&status);

  FabricSpliceEditorWidget * editor = (FabricSpliceEditorWidget*)userData;

  MString opName = editor->mOperatorList->getStdText().c_str();
  if(opName.length() == 0)
    return;

  MStringArray parts;
  opName.split('-', parts);
  MString dgNode = parts[0].substring(0, parts[0].length()-2);
  opName = parts[1].substring(1, parts[1].length());

  FabricSpliceConfirmDialog dialog(editor);
  dialog.exec();

  if(dialog.succeeded())
  {
    FabricSpliceBaseInterface * node = editor->getCurrentBaseInterface();
    if(node == NULL)
      return;
    node->removeKLOperator(opName, dgNode);
    editor->update();
  }

  MAYASPLICE_CATCH_END(&status);
}

void FabricSpliceEditorWidget::update(MStatus * status)
{
  updateNodeDropDown(status);
}

void FabricSpliceEditorWidget::clear(MStatus * status)
{
  mAttrList->clear();
  mOperatorList->clear();
  mNodeCombo->clear();
  mSourceCode->setSourceCode("", "");
  mPortText->setText("");
}

void FabricSpliceEditorWidget::updateNodeDropDown(MStatus * status)
{
  MAYASPLICE_CATCH_BEGIN(status);

  std::string currentNodeName(mNodeCombo->getStdText());
  int prevNodeIndex = -1;
  mNodeCombo->clear();

  std::map<std::string, std::string> items;

  std::vector<FabricSpliceBaseInterface*> instances = FabricSpliceBaseInterface::getInstances();
  for(size_t i=0;i<instances.size();i++)
  {
    MFnDependencyNode thisNode(instances[i]->getThisMObject());

    std::string nodeName(thisNode.name().asChar());
    items.insert(std::pair<std::string, std::string>(nodeName, nodeName));
  }

  size_t i=0;
  for(std::map<std::string, std::string>::iterator it = items.begin(); it != items.end(); it++)
  {
    mNodeCombo->addItem(it->first.c_str());    
    if(currentNodeName != "" && currentNodeName == it->first && prevNodeIndex == -1)
      prevNodeIndex = i;
    i++;
  }

  if(prevNodeIndex >= 0)
    mNodeCombo->setCurrentIndex(prevNodeIndex);

  onNodeChanged(this);
  MAYASPLICE_CATCH_END(status);
}

void FabricSpliceEditorWidget::setNodeDropDown(std::string nodeName, MStatus * status)
{
  MAYASPLICE_CATCH_BEGIN(status);
  updateNodeDropDown(status);

  for(int i=0;i<mNodeCombo->count();i++)
  {
    if(mNodeCombo->itemStdText(i) == nodeName)
    {
      mNodeCombo->setCurrentIndex(i);
      onNodeChanged(this);
      break;
    }
  }

  setCurrentIndex(0);
  MAYASPLICE_CATCH_END(status);
}

void FabricSpliceEditorWidget::setOperatorName(const std::string & opName, MStatus * status)
{
  MAYASPLICE_CATCH_BEGIN(status);

  mOpName = opName;
  onOperatorChanged(this);

  MAYASPLICE_CATCH_END(status);
}

void  FabricSpliceEditorWidget::reportCompilerError(unsigned int row, unsigned int col, const char * file, const char * level, const char * desc, MStatus * status)
{
  MAYASPLICE_CATCH_BEGIN(status);

  // ensure to only show errors for my file
  std::string opName = mOpName;
  if(opName + ".kl" != file && opName != file)
    return;

  // todo: make this nice
  MString rowStr; rowStr.set(row);
  while(rowStr.length() < 4)
    rowStr = "0" + rowStr;
  MString singleMessage = "Line "+rowStr+": "+MString(desc);
  QString errorMessage = mErrorLog->toPlainText();
  if(errorMessage.length() > 0)
    errorMessage += "\n";
  errorMessage += singleMessage.asChar();
  mErrorLog->setText(errorMessage);
  mErrorLog->show();

  MAYASPLICE_CATCH_END(status);
}

QWidget * FabricSpliceEditorWidget::creator(QWidget * parent, const QString & name)
{
  return new FabricSpliceEditorWidget(parent, name);
}

void FabricSpliceEditorWidget::postUpdateAll(MStatus * status)
{
  if(gUpdatePosted)
    return;
  MString cmd = "fabricSpliceEditor -a \"update\";";
  MStatus cmdStatus = MGlobal::executeCommandOnIdle(cmd, false);
  if(status != NULL)
    *status = cmdStatus;
  gUpdatePosted = true;
}

void FabricSpliceEditorWidget::postClearAll(MStatus * status)
{
  if(gUpdatePosted)
    return;
  MString cmd = "fabricSpliceEditor -a \"clear\";";
  MStatus cmdStatus = MGlobal::executeCommandOnIdle(cmd, false);
  if(status != NULL)
    *status = cmdStatus;
  gUpdatePosted = true;
}

void FabricSpliceEditorWidget::updateAll(MStatus * status)
{
  for(widgetIt it = gWidgets.begin(); it != gWidgets.end(); it++)
    it->second->update(status);
  gUpdatePosted = false;
}

void FabricSpliceEditorWidget::clearAll(MStatus * status)
{
  for(widgetIt it = gWidgets.begin(); it != gWidgets.end(); it++)
    it->second->clear(status);
  gUpdatePosted = false;
}

void FabricSpliceEditorWidget::updateAllNodeDropDowns(MStatus * status)
{
  for(widgetIt it = gWidgets.begin(); it != gWidgets.end(); it++)
    it->second->updateNodeDropDown(status);
}

void FabricSpliceEditorWidget::setAllNodeDropDowns(std::string nodeName, bool onIdle, MStatus * status)
{
  FabricSpliceEditorWidget::getFirstOrOpen(onIdle);
  for(widgetIt it = gWidgets.begin(); it != gWidgets.end(); it++)
  {
    it->second->setNodeDropDown(nodeName, status);
    it->second->bringToFront();
  }
}

void FabricSpliceEditorWidget::setAllOperatorNames(std::string opName, bool onIdle, MStatus * status)
{
  FabricSpliceEditorWidget::getFirstOrOpen(onIdle);
  for(widgetIt it = gWidgets.begin(); it != gWidgets.end(); it++)
  {
    it->second->setOperatorName(opName, status);
    it->second->bringToFront();
  }
}

void FabricSpliceEditorWidget::reportAllCompilerError(unsigned int row, unsigned int col, const char * file, const char * level, const char * desc, MStatus * status)
{
  for(widgetIt it = gWidgets.begin(); it != gWidgets.end(); it++)
    it->second->reportCompilerError(row, col, file, level, desc, status);
}

QSize FabricSpliceEditorWidget::sizeHint () const
{
  return QSize(420, minimumHeight());
}

bool FabricSpliceEditorWidget::event ( QEvent * event )
{
  if(event->type() == QEvent::Show)
  {
    if(gWidgets.find(mName) == gWidgets.end())
      gWidgets.insert(widgetPair(mName, this));
  }
  else if(event->type() == QEvent::Close)
  {
    widgetIt it = gWidgets.find(mName);
    if(it != gWidgets.end())
      gWidgets.erase(it);
  }
  return QTabWidget::event(event);
}

void FabricSpliceEditorWidget::bringToFront()
{
  QWidget * parent = parentWidget();
  while(parent)
  {
    QDockWidget * dock = dynamic_cast<QDockWidget *>(parent);
    if(dock)
    {
      dock->raise();
      dock->show();
      break;
    }
    parent = parent->parentWidget();
  }
}

const char * FabricSpliceEditorWidget::getSourceCodeForOperator(const char * graphName, const char * opName)
{
  FabricSpliceEditorWidget * editor = getFirst();
  if(editor)
  {
    FabricSpliceBaseInterface * interf = editor->getCurrentBaseInterface();
    if(interf)
    {
      if(interf->getSpliceGraph().getName() == std::string(graphName) && editor->mOpName == opName)
      {
        editor->mLastSourceCode = editor->mSourceCode->getSourceCode();
        return editor->mLastSourceCode.c_str();
      }
    }
  }
  return NULL;
}
