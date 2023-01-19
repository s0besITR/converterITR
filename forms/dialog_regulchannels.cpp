#include "dialog_regulchannels.h"
#include "ui_dialog_regulchannels.h"
#include "acs_xml/elesy_xml.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QSettings>


dialog_RegulChannels::dialog_RegulChannels(QVector<Elesy_XML> & xml_objects, QWidget *parent) :
    QDialog(parent, Qt::WindowTitleHint | Qt::WindowCloseButtonHint), ui(new Ui::dialog_RegulChannels),
    _xml_objects(xml_objects)
{
    ui->setupUi(this);

    //Настройка Нижних кнопок
    QPushButton* ok_btn = new QPushButton("ОК");
    QPushButton* cancel_btn = new QPushButton("Отмена");
    ok_btn->setAutoDefault(false);
    cancel_btn->setDefault(true);
    ui->buttonBox->clear();
    ui->buttonBox->addButton(ok_btn, QDialogButtonBox::AcceptRole);
    ui->buttonBox->addButton(cancel_btn, QDialogButtonBox::RejectRole);

    initSettings();
    initTable();
}

// Если модуль с таким названием уже есть в таблице, добавляем табличному модулю название нашего УСО
bool dialog_RegulChannels::updateUSOList(QString & module_name, QString & uso_name)
{
    for(uint i = 0; i < ui->tableWidget_channels->rowCount(); ++i)
    {
        if(ui->tableWidget_channels->item(i, 2)->text() == module_name)
        {
            QTableWidgetItem * it = ui->tableWidget_channels->item(i, 1);
            it->setText(it->text() + "\n" + uso_name);
            ui->tableWidget_channels->resizeRowsToContents();
            return true;
        }
    }
    return false;
}


void dialog_RegulChannels::initSettings()
{
    QSettings settings ( "ITR", "ITR_CONFIGURATOR") ;
    settings.beginGroup("/Settings/Regul");
        this->iec104_exec_time =settings.value("IEC104S_exec_time","5").toUInt();
        this->iec104_EDC_exec_time =settings.value("IEC104S_EDC_exec_time","5").toUInt();
        this->iec104_autotime =settings.value("IEC104S_autotime","0").toBool();
        this->iec104_EDC_autotime =settings.value("IEC104S_EDC_autotime","0").toBool();
        this->MBS_MemoryUnited = settings.value("MBS_MemoryUnited","1").toBool();
    settings.endGroup();
}

void dialog_RegulChannels::initTable()
{    

    // рисуем таблицу по _xml_objects


   QString exec_time_title = "Время блокировки (с)";
   MainWindow* parent = qobject_cast<MainWindow*>(this->parent());
   if(parent->ui->chb_iec104s_OS->isChecked())
   {
       exec_time_title = "Время блокировки (мс)";
       this->iec104_EDC_exec_time *=1000;
       this->iec104_exec_time *=1000;
   }

    ui->tableWidget_channels->setColumnCount(6);
    ui->tableWidget_channels->setHorizontalHeaderLabels({"","УСО","Модуль", "Тип", exec_time_title , "Автоприсвоение метки времени"});

    ui->tableWidget_channels->setColumnWidth(0, 1);
    ui->tableWidget_channels->setColumnWidth(1, 80);
    ui->tableWidget_channels->setColumnWidth(2, 80);
    ui->tableWidget_channels->setColumnWidth(3, 100);
    ui->tableWidget_channels->setColumnWidth(4, 160);
    ui->tableWidget_channels->setColumnWidth(5, 190);

    QObject::connect(ui->tableWidget_channels, SIGNAL(cellChanged(int,int)),this, SLOT(OnTableWidgetCellChanged(int,int)));     // Для проверки корректности введенных значений

    QTableWidgetItem * protoitem = new QTableWidgetItem();
    protoitem->setTextAlignment(Qt::AlignCenter);

    this->ui->chbx_modbusUniteArea->setVisible(false);
    for(Elesy_XML & obj : this->_xml_objects)
    {
        for(iec104Slave & iec_slave : obj.slaves_104)
        {
            // Если модуля с таким названием нет то добавляем новую строчку
            if(!updateUSOList(iec_slave.module_name,obj._usoName ))
            {
                uint exec_time = iec_slave.module_name.contains("EDC",Qt::CaseInsensitive) ? this->iec104_EDC_exec_time : this->iec104_exec_time;
                bool autotime = iec_slave.module_name.contains("EDC",Qt::CaseInsensitive) ? this->iec104_EDC_autotime : this->iec104_autotime;

                ui->tableWidget_channels->setRowCount(ui->tableWidget_channels->rowCount() + 1);
                QStringList row_vals;
                row_vals << "" << obj._usoName << iec_slave.module_name << "IEC104Slave" << QString::number(exec_time) << "Autotime";
                for(int i = 0; i != row_vals.size(); ++i)
                {
                     QTableWidgetItem * newitem = protoitem->clone();
                     if(i == 0)     newitem->setCheckState(Qt::Checked);
                     if(i == 5)     newitem->setCheckState(autotime ? Qt::Checked : Qt::Unchecked);
                     if(i != 4)     newitem->setFlags(newitem->flags() &  ~ Qt::ItemIsEditable);
                     newitem->setText(row_vals.at(i));
                     ui->tableWidget_channels->setItem(ui->tableWidget_channels->rowCount() - 1,i, newitem);
                }
            }
        }
        for(mbsTCPSlave & mbs_slave : obj.slaves_mbs)
        {
            this->ui->chbx_modbusUniteArea->setVisible(true);
            // Если модуля с таким названием нет то добавляем новую строчку
            if(!updateUSOList(mbs_slave.module_name,obj._usoName ))
            {
                ui->tableWidget_channels->setRowCount(ui->tableWidget_channels->rowCount() + 1);
                QStringList row_vals;
                row_vals << "" << obj._usoName << mbs_slave.module_name << "MbsTCPSlave" << "-" << "-";
                for(int i = 0; i != row_vals.size(); ++i)
                {
                     QTableWidgetItem * newitem = protoitem->clone();
                     if(i == 0)     newitem->setCheckState(Qt::Checked);
                     newitem->setFlags(newitem->flags() &  ~ Qt::ItemIsEditable);
                     newitem->setText(row_vals.at(i));
                     ui->tableWidget_channels->setItem(ui->tableWidget_channels->rowCount() - 1,i, newitem);
                }
            }
        }
    }

    this->ui->chbx_modbusUniteArea->setCheckState(this->MBS_MemoryUnited ? Qt::Checked : Qt::Unchecked);
}

dialog_RegulChannels::~dialog_RegulChannels()
{
    delete ui;
}

void dialog_RegulChannels::on_buttonBox_accepted()
{
    // сохранить данные из таблицы в xml_objects

    for(int i = 0; i < ui->tableWidget_channels->rowCount(); ++i)
    {
        QString module_name = ui->tableWidget_channels->item(i, 2)->text();                         // Имя модуля

        for(Elesy_XML & obj : this->_xml_objects)
        {
            for(iec104Slave & iec_slave : obj.slaves_104)
                if(iec_slave.module_name == module_name)
                {
                    iec_slave.checked = ui->tableWidget_channels->item(i, 0)->checkState() == Qt::Checked;      // Генерировать каналы для модуля или нет
                    for(auto & data: iec_slave.data_channels)
                        data.autotime = ui->tableWidget_channels->item(i, 5)->checkState() == Qt::Checked;      // Автоприсвоение метки времени (данные)
                    for(auto & cmd: iec_slave.cmd_channels)
                        cmd.select_exec_time = ui->tableWidget_channels->item(i, 4)->text().toUInt();           // Время блокировки (команды)
                }

            for(mbsTCPSlave & mbs_slave : obj.slaves_mbs)
               if(mbs_slave.module_name == module_name)
               {
                   mbs_slave.checked = ui->tableWidget_channels->item(i, 0)->checkState() == Qt::Checked;      // Генерировать каналы для модуля или нет
                   mbs_slave.united_memory = ui->chbx_modbusUniteArea->isChecked();
               }
        }
    }

}

// Проверка на правильность введенных значений для времени блокировки
 void dialog_RegulChannels::OnTableWidgetCellChanged(int r,int c)
 {
     if((ui->tableWidget_channels->item(r, c)->flags() & Qt::ItemIsEditable) == 0)
         return;

     bool bRetValue = false;
     QString strValue = ui->tableWidget_channels->item(r, c)->text();
     int nValue = strValue.toInt(&bRetValue);
     if(bRetValue == false || nValue < 0)
          ui->tableWidget_channels->item(r, c)->setText("0");
 }

