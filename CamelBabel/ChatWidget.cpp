#include <QTime>
#include "ChatWidget.hpp"
#include "ui_ChatWidget.h"

ChatWidget::ChatWidget(const QString &me, const QString &contact, bool incall,
                       RTPCallManager *rtpCallManager, QWidget *parent) :
  QWidget(parent),
  _ui(new Ui::ChatWidget),
  _me(me),
  _contact(contact),
  _inCall(false),
  _rtpCallManager(rtpCallManager)
{
  _ui->setupUi(this);
  if (incall)
    _ui->callButton->setEnabled(false);
  connect(_ui->userInput, SIGNAL(returnPressed()), this, SLOT(sendText()));
  connect(this, SIGNAL(sendMessageToCurrent(const QString&)),
          parent, SLOT(sendMessageToCurrent(const QString&)));
}

ChatWidget::~ChatWidget()
{
  delete _ui;
}

void ChatWidget::appendMessage(const QString &name, const QString &date, const QString &message)
{
  QDateTime timestamp;
  timestamp.setTime_t(date.toInt());
  _ui->chatText->append("<b><font color=\"#D00000\">(" + timestamp.toString(Qt::SystemLocaleShortDate) + ") " + name + ": </font></b>" + message);
}

void ChatWidget::startCall(const QString &ip, quint16 port)
{
  _rtpCallManager->call(ip, port);
}

void ChatWidget::callClicked(const bool startCall)
{
  if (!_inCall)
    {
      _ui->callButton->setIcon(QIcon::fromTheme("call-stop"));
      _inCall = true;
      emit callStarted(startCall);
    }
  else
    {
      _ui->callButton->setIcon(QIcon::fromTheme("call-start"));
      emit callFinished();
      _inCall = false;
      _rtpCallManager->finishCall();
    }
}

void ChatWidget::setCallButton(bool status)
{
  if (!_inCall)
    _ui->callButton->setEnabled(status);
  else if (_inCall && status)
    {
      _ui->callButton->setIcon(QIcon::fromTheme("call-start"));
      _inCall = false;
      _rtpCallManager->finishCall();
    }
}

void ChatWidget::sendText()
{
  if(!_ui->userInput->text().isEmpty())
    {
      QDateTime time = QDateTime::currentDateTime();
      emit sendMessageToCurrent(_ui->userInput->text().replace('\t', "    "));
      _ui->chatText->append("<b><font color=\"#3333CC\">(" + time.toString(Qt::SystemLocaleShortDate) + ") " +_me + ": </font></b>" + _ui->userInput->text());
      _ui->userInput->clear();
    }
}
