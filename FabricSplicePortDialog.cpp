#include <QtGui/QLabel>
#include <QtCore/QRegExp>
#include <QtGui/QRegExpValidator>

#include "FabricSplicePortDialog.h"

FabricSplicePortDialog::FabricSplicePortDialog(QWidget * parent)
: FabricSpliceBaseDialog(parent)
{
  setWindowTitle("New Port");

  QLabel * label = new QLabel("Name", this);
  layout()->addWidget(label);

  mName = new QLineEdit(this);
  layout()->addWidget(mName);

  QRegExp rx("[_a-zA-Z].[_a-zA-Z0-9]*");
  QValidator *validator = new QRegExpValidator(rx, mName);
  mName->setValidator(validator);

  label = new QLabel("DataType", this);
  layout()->addWidget(label);

  mDataType = new FabricSpliceComboWidget(this);
  mDataType->addItem("Custom");
  mDataType->addItem("CompoundParam");
  mDataType->addItem("Boolean");
  mDataType->addItem("Integer");
  mDataType->addItem("Scalar");
  mDataType->addItem("String");
  mDataType->addItem("Color");
  mDataType->addItem("Vec3");
  mDataType->addItem("Euler");
  mDataType->addItem("Mat44");
  mDataType->addItem("KeyframeTrack");
  mDataType->addItem("Points");
  mDataType->addItem("Lines");
  mDataType->addItem("PolygonMesh");
  mDataType->addItem("GeometryLocation");
  layout()->addWidget(mDataType);
  mDataType->setCurrentIndex(2);
  mDataType->setCallback(onDataTypeChanged, this);

  mCustomRTLabel = new QLabel("Custom", this);
  layout()->addWidget(mCustomRTLabel);

  mCustomRT = new QLineEdit(this);
  layout()->addWidget(mCustomRT);

  mExtensionLabel = new QLabel("Extension", this);
  layout()->addWidget(mExtensionLabel);

  mExtension = new QLineEdit(this);
  layout()->addWidget(mExtension);

  mCustomRTLabel->hide();
  mCustomRT->hide();
  mExtensionLabel->hide();
  mExtension->hide();

  label = new QLabel("Structure", this);
  layout()->addWidget(label);

  mArrayType = new FabricSpliceComboWidget(this);
  layout()->addWidget(mArrayType);

  label = new QLabel("Mode", this);
  layout()->addWidget(label);

  mMode = new FabricSpliceComboWidget(this);
  mMode->addItem("Input");
  mMode->addItem("Output");
  mMode->addItem("I / O");
  mMode->setCurrentIndex(0);
  layout()->addWidget(mMode);

  mAttrCheckBox = new FabricSpliceCheckBoxWidget("Add Maya Attribute", this);
  layout()->addWidget(mAttrCheckBox);
  mAttrCheckBoxChecked = false;

  mAttrSpliceMayaDataCheckBox = new FabricSpliceCheckBoxWidget("Use SpliceMayaData Attribute", this);
  layout()->addWidget(mAttrSpliceMayaDataCheckBox);

  mPersistenceType = new FabricSpliceComboWidget(this);
  mPersistenceType->addItem("Default");
  mPersistenceType->addItem("Always persist");
  mPersistenceType->addItem("Never persist");
  mPersistenceType->setCurrentIndex(0);
  layout()->addWidget(mPersistenceType);

  onDataTypeChanged(this);
  setupButtons();
}

FabricCore::Variant FabricSplicePortDialog::getValue(const std::string & name)
{
  FabricCore::Variant result;
  if(name == "name")
  {
    std::string value = getStdStringFromQString(mName->text());
    if(value.length() > 0)
      result = FabricCore::Variant::CreateString(value.c_str());
  }
  else if(name == "mode")
  {
    int value = mMode->currentIndex();
    result = FabricCore::Variant::CreateSInt32(value);
  }
  else if(name == "dataType")
  {
    std::string value = mDataType->getStdText();
    if(value == "Custom")
      value = getStdStringFromQString(mCustomRT->text());
    if(value.length() > 0)
      result = FabricCore::Variant::CreateString(value.c_str());
  }
  else if(name == "extension")
  {
    std::string value = getStdStringFromQString(mExtension->text());
    if(value.length() > 0)
      result = FabricCore::Variant::CreateString(value.c_str());
  }
  else if(name == "arrayType")
  {
    std::string value = mArrayType->getStdText();
    if(value.length() > 0)
      result = FabricCore::Variant::CreateString(value.c_str());
  }
  else if(name == "addAttribute")
  {
    bool value = mAttrCheckBox->checkState() == Qt::Checked;
    result = FabricCore::Variant::CreateBoolean(value);
  }
  else if(name == "useSpliceMayaData"){
    bool value = mAttrSpliceMayaDataCheckBox->checkState() == Qt::Checked;
    result = FabricCore::Variant::CreateBoolean(value);
  }
  else if(name == "persistenceType")
  {
    int value = mPersistenceType->currentIndex();
    result = FabricCore::Variant::CreateSInt32(value);
  }

  return result;
}

void FabricSplicePortDialog::onDataTypeChanged(void * userData)
{
  FabricSplicePortDialog * dialog = (FabricSplicePortDialog*)userData;

  dialog->mArrayType->clear();
  dialog->mArrayType->addItem("Single Value");

  std::string dataType = dialog->mDataType->getStdText();
  if(dataType != "String" &&
    dataType != "Euler" &&
    dataType != "CompoundParam" &&
    dataType != "CompoundArrayParam")
    dialog->mArrayType->addItem("Array (Multi)");
  if(dataType == "Scalar" || dataType == "Integer" || dataType == "Vec3")
    dialog->mArrayType->addItem("Array (Native)");

  // standard types which can be communicated with maya
  if(dataType == "Boolean" ||
    dataType == "Integer" ||
    dataType == "Scalar" ||
    dataType == "String" ||
    dataType == "Color" ||
    dataType == "Vec3" ||
    dataType == "Euler" ||
    dataType == "Mat44" ||
    dataType == "KeyframeTrack" || 
    dataType == "PolygonMesh" || 
    dataType == "Lines")
  {
    if(!dialog->mAttrCheckBoxChecked)
    {
      dialog->mAttrCheckBox->setCheckState(Qt::Checked);
      dialog->mAttrCheckBox->setEnabled(true);
      dialog->mAttrCheckBoxChecked = true;
    }
  }
  else
  {
    if(dialog->mAttrCheckBoxChecked)
    {
      dialog->mAttrCheckBox->setCheckState(Qt::Unchecked);
      dialog->mAttrCheckBox->setDisabled(true);
      dialog->mAttrCheckBoxChecked = false;
    }
  }

  if(dataType == "Custom")
  {
    dialog->mCustomRTLabel->show();
    dialog->mCustomRT->show();
    dialog->mExtensionLabel->show();
    dialog->mExtension->show();
  }
  else
  {
    dialog->mCustomRTLabel->hide();
    dialog->mCustomRT->hide();
    dialog->mExtensionLabel->hide();
    dialog->mExtension->hide();
  }
}
