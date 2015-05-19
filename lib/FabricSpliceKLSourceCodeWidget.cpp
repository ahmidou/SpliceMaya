#include <QtGui/QFont>
#include <QtGui/QVBoxLayout>
#include <QtGui/QPainter>
#include <QtGui/QApplication>
#include <QtGui/QScrollBar>
#include <QtGui/QToolTip>

#include "FabricSpliceKLSourceCodeWidget.h"
#include "FabricSpliceHelpers.h"

#include <maya/MString.h>
#include <maya/MGlobal.h>

FabricSpliceKLLineNumberWidget::FabricSpliceKLLineNumberWidget(QFont font, QWidget * parent)
: QWidget(parent)
{
  mFont = font;
  mFontMetrics = new QFontMetrics(mFont);
  int maxWidth = mFontMetrics->width("0000") + 4;
  setMinimumWidth(maxWidth);
  setMaximumWidth(maxWidth);
  mLineOffset = 0;
}

FabricSpliceKLLineNumberWidget::~FabricSpliceKLLineNumberWidget()
{
  delete(mFontMetrics);
}

void FabricSpliceKLLineNumberWidget::paintEvent ( QPaintEvent * event )
{
  QWidget::paintEvent(event);

  QPainter painter(this);
  QPalette palette(qApp->palette());
  painter.fillRect(event->rect(), palette.color(QPalette::Background));

  int lineHeight = mFontMetrics->lineSpacing();
  int width = event->rect().width();
  int height = event->rect().height();
  int offset = lineHeight;
  if(mLineOffset == 0)
    offset += 2;
  else
    offset -= 0;

  painter.setFont(mFont);
  painter.setPen(palette.color(QPalette::Highlight));

  int line = mLineOffset + 1;
  while(offset < height)
  {
    MString lineStr;
    lineStr.set(line);
    while(lineStr.length() < 4)
      lineStr = "0" + lineStr;
    int lineWidth = mFontMetrics->width(lineStr.asChar());
    painter.drawText(QPoint(width - 2 - lineWidth, offset), lineStr.asChar());
    offset += lineHeight;
    line++;
  }
}

FabricSpliceCodeCompletionMenu::FabricSpliceCodeCompletionMenu(QFont font, QWidget * parent)
: QMenu(parent)
{
  mFont = font;
  mFont.setPixelSize(11);
  mShown = false;
  setStyleSheet("QWidget { background-color: light-blue, foreground-color: white }");
}

void FabricSpliceCodeCompletionMenu::addItem(const std::string & name, const std::string & desc, const std::string & comments)
{
  Item item;
  item.name = name;
  item.desc = desc;
  item.comments = comments;
  mItems.push_back(item);
}

void FabricSpliceCodeCompletionMenu::clearItems()
{
  mItems.clear();
  mPressedKeySequence.clear();
  mPrefix.clear();
  clear();
}

void FabricSpliceCodeCompletionMenu::show(QPoint pos)
{
  mPos = pos;
  clear();
  QAction * currentAction = NULL;
  for(size_t  i=0;i<mItems.size();i++)
  {
    mItems[i].action = NULL;
    if(mPressedKeySequence.length() > 0)
    {
      if(mItems[i].name.substr(0, mPressedKeySequence.length()) != mPressedKeySequence)
        continue;
    }
    std::string label = mItems[i].desc.c_str();
    if(label.length() > 50)
      label = label.substr(0, 47) + "...";
    mItems[i].action = addAction(label.c_str());
    if(mItems[i].comments.length() > 0)
    {
      mItems[i].action->setStatusTip(mItems[i].comments.c_str());
      mItems[i].action->setToolTip(mItems[i].comments.c_str());
    }
    else
    {
      mItems[i].action->setStatusTip(mItems[i].desc.c_str());
      mItems[i].action->setToolTip(mItems[i].desc.c_str());
    }
    mItems[i].action->setFont(mFont);
    if(currentAction == NULL)
      currentAction = mItems[i].action;
  }
  if(mItems.size() == 0)
    return;
  if(!mShown)
  {
    mShown = true;
    setActiveAction(currentAction);
    QAction * action = exec(mPos);
    if(action)
    {
      std::string text;
      for(size_t i=0;i<mItems.size();i++)
      {
        if(mItems[i].action == action)
        {
          text = mItems[i].name;
          break;
        }
      }
      text = text.substr(mPrefix.length(), 10000);
      if(text.length() > 0)
        ((FabricSpliceKLPlainTextWidget*)parent())->insertPlainText(text.c_str());
    }
    ((FabricSpliceKLPlainTextWidget*)parent())->setFocus();
    mShown = false;
  }
  else
  {
    setActiveAction(currentAction);
  }
}

void FabricSpliceCodeCompletionMenu::keyPressEvent(QKeyEvent  *e)
{
  if(!mShown)
    return QMenu::keyPressEvent(e);
  if(e->key() == 16777219) // backspace
  {
    if(mPressedKeySequence.length() >  0)
    {
      mPressedKeySequence = mPressedKeySequence.substr(0, mPressedKeySequence.length()-1);
      update();
    }
    e->accept();
    return;
  }
  else if(e->key() == 16777220) // enter
  {
    return QMenu::keyPressEvent(e);
  }
  if(e->modifiers() != Qt::ShiftModifier && e->modifiers() != 0)
    return QMenu::keyPressEvent(e);

  QString text = e->text();
  if(text.length() != 1)
    return QMenu::keyPressEvent(e);
  if(isalnum(text.at(0).toAscii()))
  {
    mPressedKeySequence += text.at(0).toAscii();
    update();
    e->accept();
    // e->ignore();
    return;
  }

  QMenu::keyPressEvent(e);
}

void FabricSpliceCodeCompletionMenu::hide()
{
  mShown = false;
}

std::string FabricSpliceCodeCompletionMenu::completionSuffix()
{
  QAction * action = activeAction();
  if(!action)
    return "";
  QString qText = action->text();
  std::string text;
  #ifdef _WIN32
    text = qText.toLocal8Bit().constData();
  #else
    text = qText.toUtf8().constData();
  #endif
  if(text.substr(0, mPressedKeySequence.length()) == mPressedKeySequence)
    return text.substr(mPressedKeySequence.length(), 10000);
  return "";
}

FabricSpliceKLPlainTextWidget::FabricSpliceKLPlainTextWidget(QFont font, FabricSpliceKLLineNumberWidget * lineNumbers, QWidget * parent)
: QPlainTextEdit(parent)
{
  document()->setDefaultFont(font);
  setWordWrapMode(QTextOption::NoWrap);
  setTabStopWidth(2);

  mLineNumbers = lineNumbers;
  mLastScrollOffset = -1;

  mHighlighter = new FabricSpliceKLSyntaxHighlighter(this->document());

  mCodeCompletionMenu = new FabricSpliceCodeCompletionMenu(font, this);
  mCodeCompletionMenu->clearItems();
}

FabricSpliceKLPlainTextWidget::~FabricSpliceKLPlainTextWidget()
{
  delete(mHighlighter);
}

bool FabricSpliceKLPlainTextWidget::event(QEvent * e)
{
  if(e->type() == QEvent::ToolTip)
  {
    QHelpEvent *helpEvent = static_cast<QHelpEvent *>(e);
    QTextCursor cursor = cursorForPosition(helpEvent->pos());

    FabricSpliceKLSourceCodeWidget * sourceCodeWidget = (FabricSpliceKLSourceCodeWidget*)parent();
    sourceCodeWidget->updateKLParser();
    FabricSplice::KLParser parser = sourceCodeWidget->getKLParser();
    FabricSplice::KLParser::KLSymbol symbol = parser.getKLSymbolFromCharIndex(cursor.position());
    if(!symbol)
      return QPlainTextEdit::event(e);
    if(symbol.type() != FabricSplice::KLParser::KLSymbol::Type_name && symbol.type() != FabricSplice::KLParser::KLSymbol::Type_rt)
      return QPlainTextEdit::event(e);

    FabricSplice::KLParser::KLSymbol prevSymbol = symbol.prev();
    std::string toolTipText;
    if(prevSymbol.type() == FabricSplice::KLParser::KLSymbol::Type_period)
    {
      std::string name = symbol.str();
      std::string symbolType = parser.getKLTypeForSymbol(prevSymbol.prev());
      for(unsigned int i=0;i<FabricSplice::KLParser::getNbParsers();i++)
      {
        FabricSplice::KLParser p = FabricSplice::KLParser::getParser(i);
        for(unsigned int j=0;j<p.getNbKLFunctions();j++)
        {
          FabricSplice::KLParser::KLFunction f = p.getKLFunction(j);
          if(f.owner() == symbolType && f.name() == name)
          {
            if(toolTipText.length() > 0)
              toolTipText += "\n";
            toolTipText += f.comments();
            toolTipText += "\n";
            if(strlen(f.type()) > 0)
            {
              toolTipText += f.type();
              toolTipText += " ";
            }
            if(strlen(f.owner()) > 0)
            {
              toolTipText += f.owner();
              toolTipText += ".";
            }
            toolTipText += f.name();
            toolTipText += "(";
            FabricSplice::KLParser::KLArgumentList args = f.arguments();
            for(unsigned k=0;k<args.nbArgs();k++)
            {
              if(k>0)
                toolTipText += ", ";
              toolTipText += args.mode(k);
              toolTipText += " ";
              toolTipText += args.type(k);
              toolTipText += " ";
              toolTipText += args.name(k);
            }
            toolTipText += ")";
          }
        } 
      }
    }
    else
    {
      toolTipText = parser.getKLTypeForSymbol(symbol);
    }
    if(toolTipText.length() == 0)
      return QPlainTextEdit::event(e);
    QToolTip::showText(mapToGlobal(helpEvent->pos()), toolTipText.c_str());
    mayaLogFunc(symbol.str().c_str());
    return true;
  }
  return QPlainTextEdit::event(e);
}

std::string FabricSpliceKLPlainTextWidget::getStdText()
{
  QString code = toPlainText();
#ifdef _WIN32
  return code.toLocal8Bit().constData();
#else
  return code.toUtf8().constData();
#endif
}

void FabricSpliceKLPlainTextWidget::setStdText(const std::string & code)
{
  setPlainText(code.c_str());
}

void FabricSpliceKLPlainTextWidget::keyPressEvent (QKeyEvent * e)
{
  if(e->key() == 16777217) // tab
  {
    insertPlainText("  ");
    return;
  }
  else if(e->key() == 16777218) // shift + tab
  {
    // for now let's just ignore this
    return;
  }
  else if(e->key() == 16777220) // enter
  {
    std::string codeStr(getStdText());
    MString code;
    code += codeStr.c_str();
    if(code.length() > 0)
    {
      int pos = textCursor().position();
      MString posStr;

      // remove all previous lines
      int find = code.index('\n');
      while(find < pos && find >= 0)
      {
        code = code.substring(find + 1, 1000000);
        pos -= find + 1;
        find = code.index('\n');
      }

      // figure out how many spaces to insert
      int initialPos = pos;
      pos = 0;
      MString prefix;
      while(pos < code.length())
      {
        if(code.asChar()[pos] != ' ')
          break;
        prefix += " ";
        pos++;
      }

      // remove unrequired spaces
      while(prefix.length() > 0 && code.asChar()[initialPos] == ' ')
      {
        prefix = prefix.substring(1, 1000000);
        initialPos++;
        if(initialPos == code.length())
          break;
      }

      prefix = "\n" + prefix;
      insertPlainText(prefix.asChar());
      return;
    }
  }

  if(e->key() == 32) // space
  {
    if(e->modifiers().testFlag(Qt::ControlModifier))
    {
      if(mCodeCompletionMenu->shown())
      {
        std::string suffix = mCodeCompletionMenu->completionSuffix();
        if(suffix.length() > 0)
        {
          insertPlainText(suffix.c_str());
          return;
        }
      }
      else
      {
        std::string prefix;
        FabricSpliceKLSourceCodeWidget * sourceCodeWidget = (FabricSpliceKLSourceCodeWidget*)parent();
        sourceCodeWidget->updateKLParser();
        FabricSplice::KLParser parser = sourceCodeWidget->getKLParser();
        FabricSplice::KLParser::KLSymbol symbol = parser.getKLSymbolFromCharIndex(textCursor().position());
        if(!symbol)
          return QPlainTextEdit::keyPressEvent(e);
        FabricSplice::KLParser::KLSymbol prevSymbol = symbol.prev();
        if(symbol.type() == FabricSplice::KLParser::KLSymbol::Type_period || prevSymbol.type() == FabricSplice::KLParser::KLSymbol::Type_period)
        {
          if(symbol.type() == FabricSplice::KLParser::KLSymbol::Type_period)
            symbol = prevSymbol;
          else
          {
            prefix = symbol.str();
            symbol = prevSymbol.prev();
          }
          if(!symbol)
            return QPlainTextEdit::keyPressEvent(e);

          std::string symbolType = parser.getKLTypeForSymbol(symbol);
          if(symbolType.length() == 0)
            return;

          std::map<std::string, FabricSplice::KLParser::KLFunction> methods;
          std::map<std::string, std::string> members;

          if(symbolType.substr(symbolType.length()-1, 1) == "]")
          {
            FabricSplice::KLParser p;
            if(symbolType.substr(symbolType.length()-2, 1) == "[")
              p = FabricSplice::KLParser::getParser("array", "array");
            else
              p = FabricSplice::KLParser::getParser("dict", "dict");
            for(unsigned int j=0;j<p.getNbKLFunctions();j++)
            {
              FabricSplice::KLParser::KLFunction f = p.getKLFunction(j);
              std::string key = f.name();
              // if(key[0] == '_')
              //   continue;
              key += "(";
              FabricSplice::KLParser::KLArgumentList args = f.arguments();
              for(unsigned k=0;k<args.nbArgs();k++)
              {
                if(k>0)
                  key += ", ";
                key += args.mode(k);
                key += " ";
                key += args.type(k);
                key += " ";
                key += args.name(k);
              }
              key += ")";
              if(methods.find(key) != methods.end())
                continue;
              methods.insert(std::pair<std::string, FabricSplice::KLParser::KLFunction>(key, f));
            } 
          }
          if(methods.size() == 0)
          {
            for(unsigned int i=0;i<FabricSplice::KLParser::getNbParsers();i++)
            {
              FabricSplice::KLParser p = FabricSplice::KLParser::getParser(i);
              for(unsigned int j=0;j<p.getNbKLFunctions();j++)
              {
                FabricSplice::KLParser::KLFunction f = p.getKLFunction(j);
                if(f.owner() == symbolType)
                {
                  std::string key = f.name();
                  // if(key[0] == '_')
                  //   continue;
                  key += "(";
                  FabricSplice::KLParser::KLArgumentList args = f.arguments();
                  for(unsigned k=0;k<args.nbArgs();k++)
                  {
                    if(k>0)
                      key += ", ";
                    key += args.mode(k);
                    key += " ";
                    key += args.type(k);
                    key += " ";
                    key += args.name(k);
                  }
                  key += ")";
                  if(methods.find(key) != methods.end())
                    continue;
                  methods.insert(std::pair<std::string, FabricSplice::KLParser::KLFunction>(key, f));
                }
              } 
              for(unsigned int j=0;j<p.getNbKLStructs();j++)
              {
                FabricSplice::KLParser::KLStruct f = p.getKLStruct(j);
                if(f.name() == symbolType)
                {
                  for(unsigned k=0;k<f.nbMembers();k++)
                  {
                    std::string key = f.memberName(k);
                    // if(key[0] == '_')
                    //   continue;
                    key += " (";
                    key += f.memberType(k);
                    key += ")";

                    if(members.find(key) != members.end())
                      continue;
                    members.insert(std::pair<std::string, std::string>(key, f.memberName(k)));
                  }
                }
              } 
            }
          }

          mCodeCompletionMenu->clearItems();
          if(methods.size() == 0 && members.size() == 0)
            return;

          mCodeCompletionMenu->setPrefix(prefix);
          for(std::map<std::string, std::string>::iterator it = members.begin();it!=members.end();it++)
          {
            mCodeCompletionMenu->addItem(it->second, it->first, "");
          }
          for(std::map<std::string, FabricSplice::KLParser::KLFunction>::iterator it = methods.begin();it!=methods.end();it++)
          {
            mCodeCompletionMenu->addItem(it->second.name(), it->first, it->second.comments());
          }

          QPoint pos = mapToGlobal(cursorRect().topLeft() + QPoint(0, font().pixelSize()));
          mCodeCompletionMenu->show(pos);
          return;
        }

        if(symbol.type() != FabricSplice::KLParser::KLSymbol::Type_name && symbol.type() != FabricSplice::KLParser::KLSymbol::Type_rt)
          return QPlainTextEdit::keyPressEvent(e);
        std::string name = symbol.str();

        std::map<std::string, std::string> matches;
        for(unsigned int i=0;i<FabricSplice::KLParser::getNbParsers();i++)
        {
          FabricSplice::KLParser p = FabricSplice::KLParser::getParser(i);
          for(unsigned int j=0;j<p.getNbKLStructs();j++)
          {
            FabricSplice::KLParser::KLStruct f = p.getKLStruct(j);
            std::string key = f.name();
            if(strlen(f.type()) > 0)
            {
              key += " (";
              key += f.type();
              key += ")";
            }
            if(matches.find(key) != matches.end())
              continue;
            matches.insert(std::pair<std::string, std::string>(key, f.name()));
          } 
        }
        for(unsigned int j=0;j<parser.getNbKLVariables();j++)
        {
          FabricSplice::KLParser::KLVariable f = parser.getKLVariable(j);
          if(f.symbol().pos() >= symbol.pos())
            continue;
          std::string match(f.name());
          if(match.substr(0,name.length()) == name)
          {
            std::string key = f.name();
            key += " (";
            key += f.type();
            key += ")";
            if(matches.find(key) != matches.end())
              continue;
            matches.insert(std::pair<std::string, std::string>(key, f.name()));
          }
        }

        for(unsigned int i=0;i<FabricSplice::KLParser::getNbParsers();i++)
        {
          FabricSplice::KLParser p = FabricSplice::KLParser::getParser(i);
          for(unsigned int j=0;j<p.getNbKLConstants();j++)
          {
            FabricSplice::KLParser::KLConstant f = p.getKLConstant(j);
            std::string match(f.name());
            if(match.substr(0,name.length()) == name)
            {
              std::string key = f.name();
              // if(key[0] == '_')
              //   continue;
              key += " = ";
              key += f.value();
              if(matches.find(key) != matches.end())
                continue;
              matches.insert(std::pair<std::string, std::string>(key, f.name()));
            }
          }
          for(unsigned int j=0;j<p.getNbKLStructs();j++)
          {
            FabricSplice::KLParser::KLStruct f = p.getKLStruct(j);
            std::string match(f.name());
            if(match.substr(0,name.length()) == name)
            {
              std::string key = f.name();
              // if(key[0] == '_')
              //   continue;
              key += " (";
              key += f.type();
              key += ")";
              if(matches.find(key) != matches.end())
                continue;
              matches.insert(std::pair<std::string, std::string>(key, f.name()));
            }
          }           
          for(unsigned int j=0;j<p.getNbKLFunctions();j++)
          {
            FabricSplice::KLParser::KLFunction f = p.getKLFunction(j);
            std::string match(f.name());
            if(strlen(f.owner()) == 0 && match.substr(0,name.length()) == name)
            {
              std::string key = f.name();
              // if(key[0] == '_')
              //   continue;
              key += "(";
              FabricSplice::KLParser::KLArgumentList args = f.arguments();
              for(unsigned k=0;k<args.nbArgs();k++)
              {
                if(k>0)
                  key += ", ";
                key += args.mode(k);
                key += " ";
                key += args.type(k);
                key += " ";
                key += args.name(k);
              }
              key += ")";
              if(matches.find(key) != matches.end())
                continue;
              matches.insert(std::pair<std::string, std::string>(key, f.name()));
            }
          } 
        }

        mCodeCompletionMenu->clearItems();
        mCodeCompletionMenu->setPrefix(name);

        if(matches.size() == 0)
          return QPlainTextEdit::keyPressEvent(e);
        else if(matches.size() == 0)
        {
          insertPlainText(matches.begin()->first.c_str());
          return;
        }
        
        std::map<std::string, std::string>::iterator it;
        for(it = matches.begin();it!=matches.end();it++)
          mCodeCompletionMenu->addItem(it->second, it->first, "");

        QPoint pos = mapToGlobal(cursorRect().topLeft() + QPoint(0, font().pixelSize()));
        mCodeCompletionMenu->setPressedKeySequence(name);
        mCodeCompletionMenu->show(pos);

        return;

      }
    }
  }

  QPlainTextEdit::keyPressEvent(e);

}

void FabricSpliceKLPlainTextWidget::paintEvent ( QPaintEvent * event )
{
  QPlainTextEdit::paintEvent(event);
  if(mLastScrollOffset == verticalScrollBar()->value())
    return;
  mLastScrollOffset = verticalScrollBar()->value();
  mLineNumbers->setLineOffset(mLastScrollOffset);
}

FabricSpliceKLSourceCodeWidget::FabricSpliceKLSourceCodeWidget(QWidget * parent, int fontSize)
: QWidget(parent)
{
  QFont font("courier", fontSize);
  setLayout(new QHBoxLayout());
  layout()->setContentsMargins(0, 0, 0, 0);
  mLineNumbers = new FabricSpliceKLLineNumberWidget(font, this);
  layout()->addWidget(mLineNumbers);
  mTextEdit = new FabricSpliceKLPlainTextWidget(font, mLineNumbers, this);
  layout()->addWidget(mTextEdit);
}

std::string FabricSpliceKLSourceCodeWidget::getSourceCode()
{
  return mTextEdit->getStdText();
}

void FabricSpliceKLSourceCodeWidget::setSourceCode(const std::string & opName, const std::string & code)
{
  mOperator = opName;
  mTextEdit->setStdText(code);
  updateKLParser();
}

void FabricSpliceKLSourceCodeWidget::setEnabled(bool enabled)
{
  mTextEdit->setEnabled(enabled);
}

void FabricSpliceKLSourceCodeWidget::updateKLParser()
{
  mParser = FabricSplice::KLParser::getParser(mOperator.c_str(), mOperator.c_str(), getSourceCode().c_str());
}
