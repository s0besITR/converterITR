#include "settings_dialog.h"
#include "ui_settings_dialog.h"
#include <QSettings>
#include <QIntValidator>
#include<QPushButton>
#include <QDebug>

SettingsDialog::SettingsDialog(int index,QWidget *parent) :
    QDialog(parent, Qt::WindowTitleHint | Qt::WindowCloseButtonHint),
    ui(new Ui::SettingsDialog)
{
     ui->setupUi(this);

     //Открываем настройки той вкладки, которая была в главной программе
      ui->tabWidget->setCurrentIndex(index);

     //Настройка Нижних кнопок
     QPushButton* ok_btn = new QPushButton("ОК");
     QPushButton* cancel_btn = new QPushButton("Отмена");
     ok_btn->setAutoDefault(false);
     cancel_btn->setDefault(true);
     ui->buttonBox->clear();
     ui->buttonBox->addButton(ok_btn, QDialogButtonBox::AcceptRole);
     ui->buttonBox->addButton(cancel_btn, QDialogButtonBox::RejectRole);

     // Валидация целочисленных значений для времени блокировки
     QIntValidator * int_val = new QIntValidator;
     int_val->setBottom(0);
     ui->edit_iec104_time->setValidator( int_val);
     ui->edit_iec104_timeEDC->setValidator(int_val);
     ui->lineEdit_isagraf_maxlines->setValidator(int_val);

    ui->cbox_activeTab->addItems({"Regul", "MKLogic", "Alpha", "Инф. обесп.", "Другое"});
    ui->cbox_regulPLC->addItems({"Regul R500", "Regul R200"});   
    ui->cbox_IO_parse->addItems({"7050", "5564-5567"});
    readSettings();
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::readSettings()
{
    QSettings settings ( "ITR", "ITR_CONFIGURATOR") ;
    settings.beginGroup("/Settings");

       settings.beginGroup("/Common");
           ui->cbox_activeTab->setCurrentIndex(settings.value("ActiveTab","0").toInt());
           //ui->tabWidget->setCurrentIndex(settings.value("ActiveTab","0").toInt());
       settings.endGroup();

       settings.beginGroup("/Regul");
           ui->cbox_regulPLC->setCurrentIndex(settings.value("PLC","0").toInt());          
           ui->cbx_RegulBus_OS->setChecked(settings.value("RegulBus_OS","1").toBool());
           ui->cbx_Iec104s_OS->setChecked(settings.value("Iec104S_OS","1").toBool());          
           ui->edit_iec104_time->setText(settings.value("IEC104S_exec_time","0").toString());
           ui->edit_iec104_timeEDC->setText(settings.value("IEC104S_EDC_exec_time","0").toString());
           ui->edit_ModbusVersion->setText(settings.value("MBS_Device_Versions","1.6.5.1").toString());
           ui->cbx_uniteMemory->setChecked(settings.value("MBS_MemoryUnited","1").toBool());
           ui->cbx_OSTemplated->setChecked(settings.value("MBS_OS_Templates","0").toBool());
           ui->cbx_AutoTime->setChecked(settings.value("IEC104S_autotime","0").toBool());
           ui->cbx_AutoTimeEDC->setChecked(settings.value("IEC104S_EDC_autotime","0").toBool());
       settings.endGroup();

       settings.beginGroup("/DevStudio");
           this->ui->chb_IEC870_csv->setChecked(settings.value("IEC_map","1").toBool());
           this->ui->chb_Modbus_csv->setChecked(settings.value("Modbus_map","0").toBool());
       settings.endGroup();

       settings.beginGroup("/MK_Logik");
           this->ui->chb_isagrafSplitLargeFiles->setChecked(settings.value("split_max_files","1").toBool());
           this->ui->lineEdit_isagraf_maxlines->setText(settings.value("max_lines","3000").toString());
       settings.endGroup();

       settings.beginGroup("/IO");
           this->ui->cbx_IO_rezerv->setCheckState(settings.value("No_rezerv","1").toBool() ? Qt::Checked : Qt::Unchecked);
           this->ui->cbx_IO_empty->setCheckState(settings.value("No_empty","1").toBool() ? Qt::Checked : Qt::Unchecked);
           this->ui->cbx_IO_highlight->setCheckState(settings.value("Highlight","1").toBool() ? Qt::Checked : Qt::Unchecked);
           this->ui->cbox_IO_parse->setCurrentIndex(settings.value("Parse_mode","1").toInt());
       settings.endGroup();

    settings.endGroup();
}

// Запись значений в реестр
void SettingsDialog::writeSettings()
{
    QSettings settings ( "ITR", "ITR_CONFIGURATOR") ;
    settings.beginGroup("/Settings");
       settings.setValue("/Common/ActiveTab", ui->cbox_activeTab->currentIndex());

       settings.beginGroup("/Regul");       
            settings.setValue("PLC", ui->cbox_regulPLC->currentIndex());
            settings.setValue("RegulBus_OS", ui->cbx_RegulBus_OS->isChecked());
            settings.setValue("Iec104S_OS",  ui->cbx_Iec104s_OS->isChecked());            
            settings.setValue("IEC104S_exec_time", ui->edit_iec104_time->text().length() ==0 ? "0" : ui->edit_iec104_time->text());
            settings.setValue("IEC104S_EDC_exec_time", ui->edit_iec104_timeEDC->text().length() ==0 ? "0" : ui->edit_iec104_timeEDC->text());
            settings.setValue("MBS_Device_Versions", ui->edit_ModbusVersion->text().length() == 0 ? "0.0.0.0" : ui->edit_ModbusVersion->text());
            settings.setValue("MBS_MemoryUnited", ui->cbx_uniteMemory->isChecked());
            settings.setValue("MBS_OS_Templates", ui->cbx_OSTemplated->isChecked());
            settings.setValue("IEC104S_autotime", ui->cbx_AutoTime->isChecked());
            settings.setValue("IEC104S_EDC_autotime", ui->cbx_AutoTimeEDC->isChecked());
       settings.endGroup();

       settings.beginGroup("/DevStudio");
           settings.setValue("IEC_map", this->ui->chb_IEC870_csv->isChecked());
           settings.setValue("Modbus_map", this->ui->chb_Modbus_csv->isChecked());
       settings.endGroup();

       settings.beginGroup("/MK_Logik");
            settings.setValue("split_max_files",this->ui->chb_isagrafSplitLargeFiles->isChecked());
            settings.setValue("max_lines",ui->lineEdit_isagraf_maxlines->text().length() == 0 ? "3000" : ui->lineEdit_isagraf_maxlines->text());
       settings.endGroup();

       settings.beginGroup("/IO");
            settings.setValue("No_rezerv", this->ui->cbx_IO_rezerv->isChecked());
            settings.setValue("No_empty", this->ui->cbx_IO_empty->isChecked());
            settings.setValue("Highlight", this->ui->cbx_IO_highlight->isChecked());
            settings.setValue("Parse_mode", this->ui->cbox_IO_parse->currentIndex());
       settings.endGroup();
    settings.endGroup();
}

void SettingsDialog::on_buttonBox_accepted()
{
    writeSettings();
    QDialog::accept();

}

void SettingsDialog::on_buttonBox_rejected()
{
     QDialog::reject();
}

