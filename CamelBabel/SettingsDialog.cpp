#include <QDebug>
#include <QSettings>
#include "SettingsDialog.hpp"
#include "ui_SettingsDialog.h"

SettingsDialog::SettingsDialog(QWidget *parent) :
  QDialog(parent),
  _ui(new Ui::SettingsDialog)
{
  QSettings settings;

  _ui->setupUi(this);
  settings.beginGroup("account");
  _ui->serverLineEdit->setText(settings.value("server", "127.0.0.1").toString());
  _ui->portLineEdit->setText(settings.value("port", "4242").toString());
  _ui->statusLineEdit->setText(settings.value("status", "school").toString());
  settings.endGroup();
}

SettingsDialog::~SettingsDialog()
{
  delete _ui;
}

void SettingsDialog::cancelButton()
{
  close();
}

void SettingsDialog::registerButton()
{
  qDebug() << "register";
}
