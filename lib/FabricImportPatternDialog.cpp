//
// Copyright (c) 2010-2016, Fabric Software Inc. All rights reserved.
//

#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QCheckBox>
#include <QLineEdit>
#include <QDoubleValidator>
#include <QDialogButtonBox>
#include <QPushButton>

#include <maya/MGlobal.h>

#include "FabricImportPatternDialog.h"
#include "FabricImportPatternCommand.h"
#include "FabricSpliceHelpers.h"

#include <FTL/StrRef.h>
#include <FTL/JSONDec.h>
#include <FTL/JSONValue.h>
#include <FTL/OwnedPtr.h>

bool FabricImportPatternDialog::s_previewRenderingEnabled = false;

FabricImportPatternDialog::FabricImportPatternDialog(QWidget * parent, FabricCore::Client client, FabricCore::DFGBinding binding, FabricImportPatternSettings settings)
: QDialog(parent)
, m_settings(settings)
, m_client(client)
, m_binding(binding)
, m_wasAccepted(false)
, m_stripNameSpacesCheckbox(NULL)
{
  setWindowTitle("Fabric Import Pattern");
  setAttribute(Qt::WA_DeleteOnClose, true);

  if(m_settings.useLastArgValues)
    restoreSettings(client, m_settings.filePath, m_binding);

  m_stack = new QUndoStack();
  m_handler = new FabricUI::DFG::DFGUICmdHandler_QUndo(m_stack);
  m_bindingItem = new FabricUI::ModelItems::BindingModelItem(m_handler, binding, true, false, false);
  m_owner = new FabricUI::ValueEditor::VEEditorOwner();
  m_owner->initConnections();
  m_owner->emitReplaceModelRoot( m_bindingItem );

  setSizePolicy(QSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding ));
  setMinimumWidth(500);
  setMaximumWidth(1200);
  setMinimumHeight(800);

  QWidget * valueEditor = m_owner->getWidget();
  valueEditor->setSizePolicy(QSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding ));
  valueEditor->setParent(this);

  QVBoxLayout * layout = new QVBoxLayout();
  setLayout(layout);
  layout->addWidget(valueEditor);

  QWidget * optionsWidget = new QWidget(this);
  optionsWidget->setContentsMargins(0, 0, 0, 0);
  QGridLayout * optionsLayout = new QGridLayout();
  optionsWidget->setLayout(optionsLayout);
  layout->addWidget(optionsWidget);

  int row = 0;

  QLabel * attachToExistingLabel = new QLabel("Attach to existing", optionsWidget);
  optionsLayout->addWidget(attachToExistingLabel, row, 0, Qt::AlignLeft | Qt::AlignVCenter);
  QCheckBox * attachToExistingCheckbox = new QCheckBox(optionsWidget);
  attachToExistingCheckbox->setCheckState(m_settings.attachToExisting ? Qt::Checked : Qt::Unchecked);
  optionsLayout->addWidget(attachToExistingCheckbox, row, 1, Qt::AlignLeft | Qt::AlignVCenter);
  QObject::connect(attachToExistingCheckbox, SIGNAL(stateChanged(int)), this, SLOT(onAttachToExistingChanged(int)));

  row++;
  QLabel * userAttributesLabel = new QLabel("User Attributes", optionsWidget);
  optionsLayout->addWidget(userAttributesLabel, row, 0, Qt::AlignLeft | Qt::AlignVCenter);
  QCheckBox * userAttributesCheckbox = new QCheckBox(optionsWidget);
  userAttributesCheckbox->setCheckState(m_settings.userAttributes ? Qt::Checked : Qt::Unchecked);
  optionsLayout->addWidget(userAttributesCheckbox, row, 1, Qt::AlignLeft | Qt::AlignVCenter);
  QObject::connect(userAttributesCheckbox, SIGNAL(stateChanged(int)), this, SLOT(onUserAttributesChanged(int)));

  row++;
  QLabel * enableMaterialsLabel = new QLabel("Enable Materials", optionsWidget);
  optionsLayout->addWidget(enableMaterialsLabel, row, 0, Qt::AlignLeft | Qt::AlignVCenter);
  QCheckBox * enableMaterialsCheckbox = new QCheckBox(optionsWidget);
  enableMaterialsCheckbox->setCheckState(m_settings.enableMaterials ? Qt::Checked : Qt::Unchecked);
  optionsLayout->addWidget(enableMaterialsCheckbox, row, 1, Qt::AlignLeft | Qt::AlignVCenter);
  QObject::connect(enableMaterialsCheckbox, SIGNAL(stateChanged(int)), this, SLOT(onEnableMaterialsChanged(int)));

  row++;
  QLabel * scaleLabel = new QLabel("Scale", optionsWidget);
  optionsLayout->addWidget(scaleLabel, row, 0, Qt::AlignLeft | Qt::AlignVCenter);
  QLineEdit * scaleLineEdit = new QLineEdit(optionsWidget);
  scaleLineEdit->setValidator(new QDoubleValidator());
  scaleLineEdit->setText(QString::number(m_settings.scale));
  optionsLayout->addWidget(scaleLineEdit, row, 1, Qt::AlignLeft | Qt::AlignVCenter);
  QObject::connect(scaleLineEdit, SIGNAL(textChanged(const QString &)), this, SLOT(onScaleChanged(const QString &)));

  row++;
  QLabel * nameSpaceLabel = new QLabel("NameSpace", optionsWidget);
  optionsLayout->addWidget(nameSpaceLabel, row, 0, Qt::AlignLeft | Qt::AlignVCenter);
  QLineEdit * nameSpaceLineEdit = new QLineEdit(optionsWidget);
  nameSpaceLineEdit->setText(m_settings.nameSpace.asChar());
  optionsLayout->addWidget(nameSpaceLineEdit, row, 1, Qt::AlignLeft | Qt::AlignVCenter);
  QObject::connect(nameSpaceLineEdit, SIGNAL(textChanged(const QString &)), this, SLOT(onNameSpaceChanged(const QString &)));

  row++;
  QLabel * stripNameSpacesLabel = new QLabel("Strip namespaces from file", optionsWidget);
  optionsLayout->addWidget(stripNameSpacesLabel, row, 0, Qt::AlignLeft | Qt::AlignVCenter);
  m_stripNameSpacesCheckbox = new QCheckBox(optionsWidget);
  m_stripNameSpacesCheckbox->setCheckState(m_settings.stripNameSpaces ? Qt::Checked : Qt::Unchecked);
  optionsLayout->addWidget(m_stripNameSpacesCheckbox, row, 1, Qt::AlignLeft | Qt::AlignVCenter);
  QObject::connect(m_stripNameSpacesCheckbox, SIGNAL(stateChanged(int)), this, SLOT(onStripNameSpacesChanged(int)));

  row++;
  QLabel * attachToSceneTimeLabel = new QLabel("Connect scene time", optionsWidget);
  optionsLayout->addWidget(attachToSceneTimeLabel, row, 0, Qt::AlignLeft | Qt::AlignVCenter);
  QCheckBox * attachToSceneTimeCheckbox = new QCheckBox(optionsWidget);
  attachToSceneTimeCheckbox->setCheckState(m_settings.attachToSceneTime ? Qt::Checked : Qt::Unchecked);
  optionsLayout->addWidget(attachToSceneTimeCheckbox, row, 1, Qt::AlignLeft | Qt::AlignVCenter);
  QObject::connect(attachToSceneTimeCheckbox, SIGNAL(stateChanged(int)), this, SLOT(onAttachToSceneTimeChanged(int)));

  QDialogButtonBox * buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
  layout->addWidget(buttons);

  QObject::connect(buttons, SIGNAL(accepted()), this, SLOT(onAccepted()));
  QObject::connect(buttons, SIGNAL(rejected()), this, SLOT(onRejected()));

  updatePreviewRendering(true);
}

FabricImportPatternDialog::~FabricImportPatternDialog()
{
  delete m_owner;
  delete m_bindingItem;
  delete m_handler;
  delete m_stack;
  m_binding = FabricCore::DFGBinding();
}

void FabricImportPatternDialog::onAccepted()
{
  FabricCore::DFGBinding binding = m_binding;
  FabricCore::Client client = m_client;
  FabricImportPatternSettings settings = m_settings;
  storeSettings(client, settings.filePath, binding);
  disablePreviewRendering();
  close();
  FabricImportPatternCommand().invoke(client, binding, settings);
}

void FabricImportPatternDialog::onRejected()
{
  m_wasAccepted = false;
  m_binding.deallocValues();
  disablePreviewRendering();
  close();
}

void FabricImportPatternDialog::onAttachToExistingChanged(int state)
{
  m_settings.attachToExisting = state == Qt::Checked;
}

void FabricImportPatternDialog::onAttachToSceneTimeChanged(int state)
{
  m_settings.attachToSceneTime = state == Qt::Checked;
}

void FabricImportPatternDialog::onUserAttributesChanged(int state)
{
  m_settings.userAttributes = state == Qt::Checked;
}

void FabricImportPatternDialog::onEnableMaterialsChanged(int state)
{
  m_settings.enableMaterials = state == Qt::Checked;
}

void FabricImportPatternDialog::onScaleChanged(const QString & text)
{
  m_settings.scale = text.toDouble();
}

void FabricImportPatternDialog::onNameSpaceChanged(const QString & text)
{
  m_settings.nameSpace = text.toUtf8().constData();
  if(m_settings.nameSpace.length() > 0)
  {
    while(m_settings.nameSpace.substring(m_settings.nameSpace.length()-1, m_settings.nameSpace.length()-1) == ":")
      m_settings.nameSpace = m_settings.nameSpace.substring(0, m_settings.nameSpace.length()-2);
    m_stripNameSpacesCheckbox->setEnabled(false);
    m_stripNameSpacesCheckbox->setCheckState(Qt::Checked);
    m_settings.stripNameSpaces = true;
  }
  else
  {
    m_stripNameSpacesCheckbox->setEnabled(true);
    m_stripNameSpacesCheckbox->setCheckState(Qt::Unchecked);
    m_settings.stripNameSpaces = false;
  }
}

void FabricImportPatternDialog::onStripNameSpacesChanged(int state)
{
  m_settings.stripNameSpaces = state == Qt::Checked;
}

void FabricImportPatternDialog::closeEvent(QCloseEvent * event)
{
  disablePreviewRendering();
  QDialog::closeEvent(event);
}

void FabricImportPatternDialog::storeSettings(FabricCore::Client client, MString patternPath, FabricCore::DFGBinding binding)
{
  MString app = "Fabric Engine";
  QSettings settings(app.asChar(), patternPath.asChar());

  try
  {
    FabricCore::DFGExec exec = binding.getExec();
    for(unsigned int i=0;i<exec.getExecPortCount();i++)
    {
      if(exec.getExecPortType(i) != FabricCore::DFGPortType_In)
        continue;

      FTL::CStrRef type = exec.getExecPortResolvedType(i);
      if(type != "String" && type != "FilePath" && !FabricCore::GetRegisteredTypeIsShallow(client.getContext(), type.c_str()))
        continue;

      FTL::CStrRef name = exec.getExecPortName(i);

      FabricCore::RTVal value = binding.getArgValue(i);
      if(type == "FilePath")
        value = value.callMethod("String", "string", 0, 0);
      QString jsonValue = value.getJSON().getStringCString();
      settings.setValue(name.c_str(), jsonValue);
    }
  }
  catch(FabricCore::Exception e)
  {
    mayaLogErrorFunc(e.getDesc_cstr());
  }
}

void FabricImportPatternDialog::restoreSettings(FabricCore::Client client, MString patternPath, FabricCore::DFGBinding binding)
{
  MString app = "Fabric Engine";
  QSettings settings(app.asChar(), patternPath.asChar());

  try
  {
    FabricCore::DFGExec exec = binding.getExec();
    for(unsigned int i=0;i<exec.getExecPortCount();i++)
    {
      if(exec.getExecPortType(i) != FabricCore::DFGPortType_In)
        continue;

      FTL::CStrRef type = exec.getExecPortResolvedType(i);
      if(type != "String" && type != "FilePath" && !FabricCore::GetRegisteredTypeIsShallow(client.getContext(), type.c_str()))
        continue;

      FTL::CStrRef name = exec.getExecPortName(i);

      QVariant jsonVariant = settings.value(name.c_str());
      if(!jsonVariant.isValid())
        continue;

      MString jsonString = jsonVariant.toString().toUtf8().constData();

      FabricCore::RTVal value;
      if(type == "FilePath")
      {
        FabricCore::RTVal valueStr = FabricCore::ConstructRTValFromJSON(client, "String", jsonString.asChar());
        value = FabricCore::RTVal::Create(client, "FilePath", 1, &valueStr);
      }
      else
      {
        value = FabricCore::ConstructRTValFromJSON(client, type.c_str(), jsonString.asChar());
      }
      binding.setArgValue(i, value);
    }
  }
  catch(FabricCore::Exception e)
  {
    mayaLogErrorFunc(e.getDesc_cstr());
  }
}

void FabricImportPatternDialog::BindingNotificationCallback(void * userData, char const *jsonCString, uint32_t jsonLength)
{
  FabricImportPatternDialog * dialog = (FabricImportPatternDialog *)userData;
  if(!dialog)
    return;

  FTL::CStrRef jsonStrRef( jsonCString, jsonLength );
  FTL::JSONStrWithLoc jsonStrWithLoc( jsonStrRef );
  FTL::OwnedPtr<FTL::JSONObject const> jsonObject(
    FTL::JSONValue::Decode( jsonStrWithLoc )->cast<FTL::JSONObject>()
    );
  FTL::CStrRef descStr = jsonObject->getString( FTL_STR("desc") );
  if ( descStr != FTL_STR("dirty") )
    return;

  dialog->updatePreviewRendering(false);
}

void FabricImportPatternDialog::updatePreviewRendering(bool enableIfDisabled)
{
  if((!s_previewRenderingEnabled) && (!enableIfDisabled))
    return;

  if(!s_previewRenderingEnabled)
  {
    m_binding.setNotificationCallback( FabricImportPatternDialog::BindingNotificationCallback, this );
    s_previewRenderingEnabled = true;
  }

  try
  {
    m_binding.execute();
  }
  catch(FabricCore::Exception e)
  {
    mayaLogErrorFunc(e.getDesc_cstr());

    if(m_binding.isValid())
    {
      MString errors = m_binding.getErrors(true).getCString();
      mayaLogErrorFunc(errors);
    }
  }
  MGlobal::executeCommand("refresh");
}

void FabricImportPatternDialog::disablePreviewRendering()
{
  if(!s_previewRenderingEnabled)
    return;
  m_binding.setNotificationCallback(NULL, NULL);
  s_previewRenderingEnabled = false;
  MGlobal::executeCommand("refresh");
}
