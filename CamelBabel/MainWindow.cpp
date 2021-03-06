#include <QMessageBox>
#include <QSettings>
#include <QInputDialog>
#include <QVariant>
#include <QTimer>
#include <QDebug>
#include "MainWindow.hpp"
#include "ui_MainWindow.h"
#include "SettingsDialog.hpp"
#include "ChatWidget.hpp"

MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent),
  _ui(new Ui::MainWindow),
  _availableImg(":/images/available.png"),
  _awayImg(":/images/away.png"),
  _doNotDisturbImg(":/images/do_not_disturb.png"),
  _offlineImg(":/images/offline.png"),
  _trayIcon(new QSystemTrayIcon(this)),
  _trayIconMenu(new QMenu(this)),
  _inCall(false),
  _rtpCallManager(new RTPCallManager(this)),
  _sipHandler(new SipHandler(this))
{
  _ui->setupUi(this);
  readSettings();
  createTrayIcon();
}

MainWindow::~MainWindow()
{
  QSettings settings;
  if (!settings.value("account/savePassword", true).toBool())
    settings.setValue("account/password", "");
  delete _sipHandler;
  delete _rtpCallManager;
  delete _ui;
}

void MainWindow::aboutToQuit()
{
  writeSettings();
  qApp->quit();
}

void MainWindow::iconActivated(QSystemTrayIcon::ActivationReason reason)
{
  if (reason == QSystemTrayIcon::Trigger
      || reason == QSystemTrayIcon::DoubleClick)
    changeMainWindowState();
}

void MainWindow::changeMainWindowState()
{
  setVisible(!isVisible());
}

void MainWindow::about()
{
  QMessageBox::about(this, "CamelBabel About",
                     "CamelBabel is a VOIP Client\n"        \
                     "CamelCorp is a camel company\n"                                 \
                     "All camel reserved to farcy_b, gossel_j, lingua_a, post_l, teisse_l, zanchi_r");
}

void MainWindow::settings()
{
  SettingsDialog settingsDlg(this);
  settingsDlg.exec();
}

void MainWindow::addContact()
{
  QString	contact = QInputDialog::getText(this, "Add Contact", "Username:");

  if (!contact.isEmpty() && !contactAlreadyAdded(contact))
    emit addContact(contact);
}

void MainWindow::deleteContact()
{
  QListWidgetItem	*item = _ui->contactList->currentItem();

  if (item)
    {
      int ret = QMessageBox::warning(this, "CamelBabel", "You are about to remove " + item->text()
                                     + " from your contact list.\n"	\
                                     "Do you really want to delete this contact?",
                                     QMessageBox::Cancel | QMessageBox::Ok);
      if (ret == QMessageBox::Ok)
        {
          QWidget		*widget = _ui->chatStack->currentWidget();
          emit deleteContact(item->data(Qt::UserRole).toUInt());
          _ui->contactList->takeItem(_ui->contactList->row(item));
          _ui->chatStack->removeWidget(widget);
          delete item;
          delete widget;
        }
    }
}

void MainWindow::contactSelected()
{
  _ui->chatStack->setCurrentIndex(_ui->contactList->currentRow());
}

void MainWindow::callStarted(bool startCall)
{
  QListWidgetItem *tmp = _ui->contactList->currentItem();

  _inCall = true;
  emit changeCallButton(false);
  if (startCall)
    emit call(tmp->data(Qt::UserRole).toUInt());
}

void MainWindow::callFinished()
{
  QListWidgetItem *tmp = _ui->contactList->currentItem();

  _inCall = false;
  emit changeCallButton(true);
  _sipHandler->sendEndCall(tmp->data(Qt::UserRole).toUInt());
}

void MainWindow::displayMessage(const QString &message)
{
  _trayIcon->showMessage("Error", message, QSystemTrayIcon::MessageIcon(2), 4000);
}

void MainWindow::clientConnected(const bool res)
{
  int   currentIndex = _ui->statusCombo->currentIndex();

  if (res && currentIndex >= 0 && currentIndex < 3)
    {
      QSettings settings;
      _me = settings.value("account/username", "").toString();
      _rtpCallManager->setRtpPort(settings.value("account/callPort", 4243).toInt());
      if (!currentIndex)
        {
          _trayIcon->setIcon(_availableImg);
          _sipHandler->setStatus(1);
        }
      else if (currentIndex == 1)
        {
          _trayIcon->setIcon(_awayImg);
          _sipHandler->setStatus(3);
        }
      else
        {
          _trayIcon->setIcon(_doNotDisturbImg);
          _sipHandler->setStatus(2);
        }
    }
  else
    _ui->statusCombo->setCurrentIndex(3);
}

void MainWindow::disconnected()
{
  if (_ui->statusCombo->currentIndex() != 3)
    {
      _ui->statusCombo->setCurrentIndex(3);
      _trayIcon->showMessage("Disconnected", "You've been disconnected!",
                            QSystemTrayIcon::MessageIcon(2), 4000);
      QTimer::singleShot(10000, _sipHandler, SLOT(connectMe()));
    }
}

void  MainWindow::registerError()
{
  qDebug() << "register error";
}

void MainWindow::contact(const unsigned int id, const QString &username,
			 const unsigned int status, const QString &mood)
{
  Q_UNUSED(mood);
  addChat(id, username, status);
}

void MainWindow::callRequest(const unsigned int id)
{
  QListWidgetItem       *item = getContactById(id);
  if (item != NULL && _inCall == false)
    {
      _ui->contactList->setCurrentItem(item);
      _ui->chatStack->setCurrentIndex(_ui->contactList->currentRow());
      int ret = QMessageBox::question(this, "Call request", "Accept call from " + item->text(),
                                     QMessageBox::Yes | QMessageBox::No);
      if (ret == QMessageBox::Yes)
        {
          (reinterpret_cast<ChatWidget*>(_ui->chatStack->currentWidget()))->callClicked(false);
          emit acceptCall(id);
        }
      else
        emit declineCall(id);
    }
  else
    emit declineCall(id);
}

void MainWindow::contactIp(const unsigned int id, const QString &ip, quint16 port)
{
  QListWidgetItem       *item = getContactById(id);

  if (item != NULL)
    {
      (reinterpret_cast<ChatWidget*>(_ui->chatStack->widget(_ui->contactList->row(item))))->startCall(ip, port);
    }
}

void MainWindow::declinedCall(const unsigned int id)
{
  QListWidgetItem       *item = getContactById(id);

  if (item != NULL)
    {
      reinterpret_cast<ChatWidget*>(_ui->chatStack->widget(_ui->contactList->row(item)))->callClicked();
    }
}

void MainWindow::endCall(const unsigned int id)
{
  QListWidgetItem     *item = getContactById(id);

  if (item != NULL)
    {
      reinterpret_cast<ChatWidget*>(_ui->chatStack->widget(_ui->contactList->row(item)))->callClicked(true);
    }
}

void MainWindow::addContactResult(bool res)
{
  if (res)
    emit listContacts();
}

void MainWindow::changeStatus(int index)
{
  if (!_sipHandler->isConnected() && index < 3)
    _sipHandler->connectMe();
  else if (!index)
    {
      _trayIcon->setIcon(_availableImg);
      _sipHandler->setStatus(1);
    }
  else if (index == 1)
    {
      _trayIcon->setIcon(_awayImg);
      _sipHandler->setStatus(3);
    }
  else if (index == 2)
    {
      _trayIcon->setIcon(_doNotDisturbImg);
      _sipHandler->setStatus(2);
    }
  else
    {
      _trayIcon->setIcon(_offlineImg);
      clearContactList();
      if (_sipHandler->isConnected())
        _sipHandler->disconnectMe();
    }
}

void MainWindow::sendMessageToCurrent(const QString &message)
{
  emit sendMessage(_ui->contactList->currentItem()->data(Qt::UserRole).toUInt(), message);
}

void MainWindow::message(const unsigned int id, const QString &message, const QString &date)
{
  QListWidgetItem       *item = getContactById(id);

  if (item != NULL)
    (reinterpret_cast<ChatWidget*>(_ui->chatStack->widget(_ui->contactList->row(item))))->appendMessage(item->text(), date, message);
}

void MainWindow::showContextMenuForContactList(const QPoint &pos)
{
  QListWidgetItem       *item = _ui->contactList->itemAt(pos);

  if (item && _ui->contactList->row(item) != 0)
  {
    QMenu contextMenu("Context menu", this);
    contextMenu.addAction(_ui->actionDeleteContact);
    contextMenu.exec(QCursor::pos());
  }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
  if (_trayIcon->isVisible())
    {
      event->ignore();
      hide();
    }
  else
    {
      event->accept();
      aboutToQuit();
    }
}

void MainWindow::addChat(const unsigned int id, const QString &contact, const unsigned int status)
{
  QListWidgetItem       *item = getContactById(id);

  if (item == NULL)
    {
      ChatWidget		*chat = new ChatWidget(_me, contact, _inCall, _rtpCallManager, this);

      item = new QListWidgetItem(_offlineImg, contact);
      connect(chat, SIGNAL(callStarted(bool)),
              this, SLOT(callStarted(bool)));
      connect(chat, SIGNAL(callFinished()),
              this, SLOT(callFinished()));
      connect(this, SIGNAL(changeCallButton(bool)),
              chat, SLOT(setCallButton(bool)));
      item->setData(Qt::UserRole, id);
      _ui->contactList->addItem(item);
      _ui->chatStack->addWidget(chat);
    }
  if (status == 0)
    item->setIcon(_offlineImg);
  else if (status == 1)
    item->setIcon(_availableImg);
  else if (status == 2)
    item->setIcon(_doNotDisturbImg);
  else
    item->setIcon(_awayImg);
}

QListWidgetItem *MainWindow::getContactById(const unsigned int id)
{
  for (int i = 0; i < _ui->contactList->count(); ++i)
    {
      QListWidgetItem *tmp = _ui->contactList->item(i);
      if (tmp->data(Qt::UserRole).toUInt() == id)
        {
          return (tmp);
        }
    }
  return (NULL);
}

bool MainWindow::contactAlreadyAdded(const QString &contact) const
{
  for (int i = 0; i < _ui->contactList->count(); ++i)
    if (_ui->contactList->item(i)->text() == contact)
      return (true);
  return (false);
}

void MainWindow::moveListItemToPos(const int fromPos, const int toPos)
{
  _ui->contactList->insertItem(toPos, _ui->contactList->takeItem(fromPos));
}

void MainWindow::moveChatWidgetToPos(const int fromPos, const int toPos)
{
  QWidget	*widget = _ui->chatStack->widget(fromPos);
  _ui->chatStack->removeWidget(widget);
  _ui->chatStack->insertWidget(toPos, widget);
}

void MainWindow::moveContactToPos(const QString &contact, const int pos)
{
 for (int i = 0; i < _ui->contactList->count(); ++i)
    if (_ui->contactList->item(i)->text() == contact)
      {
        if (i == pos) return;
        moveListItemToPos(i, pos);
        moveChatWidgetToPos(i, pos);
        _ui->contactList->setCurrentRow(pos);
        _ui->chatStack->setCurrentIndex(pos);
        return;
      }
}

void MainWindow::clearContactList()
{
  for (int i = 1; i < _ui->contactList->count(); ++i)
    {
      delete _ui->contactList->takeItem(i);
      _ui->chatStack->removeWidget(_ui->chatStack->widget(i));
    }
}

void MainWindow::createTrayIcon()
{
  _trayIconMenu->addAction(_ui->actionActiver);
  _trayIconMenu->addSeparator();
  _trayIconMenu->addAction(_ui->actionSettings);
  _trayIconMenu->addAction(_ui->actionAbout);
  _trayIconMenu->addSeparator();
  _trayIconMenu->addAction(_ui->actionQuit);

  _trayIcon->setContextMenu(_trayIconMenu);
  _trayIcon->setToolTip("CamelBabel");
  _trayIcon->setIcon(_offlineImg);
  connect(_trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
          this, SLOT(iconActivated(QSystemTrayIcon::ActivationReason)));
  _trayIcon->show();
}

void MainWindow::writeSettings()
{
  QSettings     settings;

  settings.beginGroup("MainWindow");
  settings.setValue("size", size());
  settings.setValue("pos", pos());
  settings.endGroup();
}

void MainWindow::readSettings()
{
  QSettings     settings;

  settings.beginGroup("MainWindow");
  resize(settings.value("size", QSize(400, 300)).toSize());
  move(settings.value("pos", QPoint(200, 200)).toPoint());
  settings.endGroup();
}
