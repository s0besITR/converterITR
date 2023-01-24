#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QStandardPaths>
#include <QDebug>
#include <utility>
#include <QMessageBox>
#include "acs_xml/elesy_xml.h"
#include "forms/settings_dialog.h"
#include "excel/excel.h"
#include "io_csv/io_csv.h"
#include "mk_logic/mk_logic.h"
#include <QProgressBar>
#include <iostream>
#include <fstream>
#include "other/hmi_actions.h"
#include "other/deny_req.h"
#include "forms/aboutwindow.h"

extern QMap<unsigned int,QVector<io_csv>> IO_info_map;               // ИОшная карта

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow), recentPath(""), settings("ITR", "ITR_CONFIGURATOR")
{

    ui->setupUi(this);

    // Инициализация элементов по вкладкам
    tab_0_init();
    tab_1_init();
    tab_2_init();


    // Прогресс бар
    pProgressBar = new QProgressBar(this->centralWidget());
    pProgressBar->setAlignment(Qt::AlignCenter);
    pProgressBar->hide();
    statusBar()->addPermanentWidget(pProgressBar);
    pBar_range_count = 0;

    // В самом конце обновляем внешний вид согласно настройкам в реестре
    wirteDefaultSettings();
    readSettings();

    if(this->recentPath.length() == 0 || !QDir(this->recentPath).exists())
        updateRecentPath(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));

    ui->lineEdit_isagraf_maxlines->setDisabled(!ui->chb_isagrafSplitLargeFiles->isChecked());

    //this->setWindowTitle(QString("ИТР Конфигуратор (дата сборки: %1)").arg(BUILDV));
}

MainWindow::~MainWindow()
{
    delete ui;
}

// Обновить недавний путь новым путем
 void MainWindow::updateRecentPath(QString path)
 {    
    QDir dir(path);
    if(dir.exists())
        this->recentPath = path;
    else
       this->recentPath = QFileInfo(path).path();

    this->settings.setValue("/Settings/Common/RecentPath", this->recentPath);
 }

 // Обновить недавний путь, если выбирали файлы
 void MainWindow::updateRecentPath(QStringList files)
 {
    if(files.size() == 0)
        return;
    updateRecentPath(QFileInfo(files.at(0)).filePath());
 }

// Инициализация вкладки 0
 void MainWindow::tab_0_init()
 {
     ui->cbox_regulPLC->addItems({"Regul R500", "Regul R200"});
     ui->btn_TemplateGen->setDisabled(true);
 }

 // Инициализация вкладки 1
 void MainWindow::tab_1_init()
 {
     QIntValidator * int_val = new QIntValidator;
     ui->lineEdit_isagraf_maxlines->setValidator(int_val);
 }

 // Инициализация вкладки 2
  void MainWindow::tab_2_init()
 {
    ui->tableWidget_IO->setHorizontalHeaderLabels({"Номер КП", "Сигналы", "В резерве", "Пустые", "Наложений адресов"});
    ui->tableWidget_IO->setColumnWidth(0, 80);
    ui->tableWidget_IO->setColumnWidth(1, 80);
    ui->tableWidget_IO->setColumnWidth(2, 80);
    ui->tableWidget_IO->setColumnWidth(3, 80);
    ui->tableWidget_IO->setColumnWidth(4, 120);
    ui->pb_IO_gen->setEnabled(false);
    ui->pb_IO_gen_txt->setEnabled(false);
}



 // Чтение сохраненных настроек из реестра и обновление интерфейса программы
 void MainWindow::readSettings(bool boot)
 {
     settings.beginGroup("/Settings");
        settings.beginGroup("/Common");
            this->recentPath = settings.value("RecentPath",QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)).toString();
            if(boot)
                this->ui->tabWidget->setCurrentIndex(settings.value("ActiveTab","0").toInt());
        settings.endGroup();

        settings.beginGroup("/Regul");
            this->ui->cbox_regulPLC->setCurrentIndex(settings.value("PLC","0").toInt());           
            this->ui->chb_RegulBus_OS->setCheckState(settings.value("RegulBus_OS","1").toBool() ? Qt::Checked : Qt::Unchecked);
            this->ui->chb_iec104s_OS->setCheckState(settings.value("Iec104S_OS","1").toBool() ? Qt::Checked : Qt::Unchecked);            
            this->ui->checkb_TemlatesOS->setCheckState(settings.value("MBS_OS_Templates","1").toBool() ? Qt::Checked : Qt::Unchecked);
            this->ui->ledit_TemplatesVersion->setText(settings.value("MBS_Device_Versions","1.6.5.1").toString());

            QString p = settings.value("TemplatePath","").toString();
            if(p.length()==0 || !QDir(p).exists())
            {
                this->ui->ledit_TemplatesPath->setText("");
                settings.setValue("TemplatePath","");
            }
            else
                this->ui->ledit_TemplatesPath->setText(p);
        settings.endGroup();

        settings.beginGroup("/DevStudio");
            this->ui->chb_IEC870_csv->setCheckState(settings.value("IEC_map","1").toBool() ? Qt::Checked : Qt::Unchecked);
            this->ui->chb_Modbus_csv->setCheckState(settings.value("Modbus_map","0").toBool() ? Qt::Checked : Qt::Unchecked);
        settings.endGroup();

        settings.beginGroup("/MK_Logik");
            this->ui->chb_isagrafSplitLargeFiles->setChecked(settings.value("split_max_files","1").toBool());
            this->ui->lineEdit_isagraf_maxlines->setText(settings.value("max_lines","3000").toString());
        settings.endGroup();

        settings.beginGroup("/IO");
            this->ui->chb_IO_Rezerv->setCheckState(settings.value("No_rezerv","1").toBool() ? Qt::Checked : Qt::Unchecked);
            this->ui->chb_IO_Empty->setCheckState(settings.value("No_empty","1").toBool() ? Qt::Checked : Qt::Unchecked);
            this->ui->chb_IO_Highlight->setCheckState(settings.value("Highlight","1").toBool() ? Qt::Checked : Qt::Unchecked);
            this->ui->rb_IO_Complex->setChecked(settings.value("Parse_mode","1").toBool());
            this->ui->rb_IO_Native->setChecked(!settings.value("Parse_mode","1").toBool());
        settings.endGroup();
     settings.endGroup();
 }

 // Запись значений в реестр
 void MainWindow::wirteDefaultSettings()
 {
     settings.beginGroup("/Settings");
        settings.beginGroup("/Common");            
            if(settings.value("RecentPath","").toString().length() == 0)        // Запоминаем недавний путь
                settings.setValue("RecentPath", QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));            
            if(settings.value("ActiveTab","").toString().length() == 0)         // Активная вкладка
                settings.setValue("ActiveTab", 0);
        settings.endGroup();

        settings.beginGroup("/Regul");            
            if(settings.value("PLC","").toString().length() == 0)                       // ПЛК
                settings.setValue("PLC", 0);
            if(settings.value("RegulBus_OS","").toString().length() == 0)               // Использовать RegulBus OS
                settings.setValue("RegulBus_OS", true);
            if(settings.value("Iec104S_OS","").toString().length() == 0)               // Использовать Iec104S OS
                settings.setValue("Iec104S_OS", true);           
            if(settings.value("IEC104S_exec_time","").toString().length() == 0)         // IEC104s время блокировки в сек
                settings.setValue("IEC104S_exec_time", 5);            
            if(settings.value("IEC104S_EDC_exec_time","").toString().length() == 0)     // IEC104s EDC время блокировки в сек
                settings.setValue("IEC104S_EDC_exec_time", 5);            
            if(settings.value("MBS_MemoryUnited","").toString().length() == 0)          // Modbus использовать указатели на общую память
                settings.setValue("MBS_MemoryUnited", true);           
            if(settings.value("MBS_OS_Templates","").toString().length() == 0)          // Modbus шаблоны преобразовывать к OS
                settings.setValue("MBS_OS_Templates", false);            
            if(settings.value("IEC104S_autotime","").toString().length() == 0)          // IEC104s Автоматическое присвоение метки времени
                settings.setValue("IEC104S_autotime", false);                       
            if(settings.value("IEC104S_EDC_autotime","").toString().length() == 0)      // IEC104s EDC Автоматическое присвоение метки времени
                settings.setValue("IEC104S_EDC_autotime", false);           
            if(settings.value("MBS_Device_Versions","").toString().length() == 0)       // Версия модбас устройств
                settings.setValue("MBS_Device_Versions", "1.6.5.1");
            if(settings.value("TemplatePath","").toString().length() == 0)              // Запоминаем путь шаблонов
                settings.setValue("TemplatePath", "");
        settings.endGroup();

        settings.beginGroup("/DevStudio");
            if(settings.value("Modbus_map","").toString().length() == 0)                // Чекбокс модбас адресов
                settings.setValue("Modbus_map", false);
            if(settings.value("IEC_map","").toString().length() == 0)                   // Чекбокс МЭК адресов
                settings.setValue("IEC_map", true);
        settings.endGroup();

        settings.beginGroup("/MK_Logik");
            if(settings.value("split_max_files","").toString().length() == 0)            // Разделять большие файлы
                settings.setValue("split_max_files", true);
            if(settings.value("max_lines","").toString().length() == 0)                 // Максимальное кол-во строк
                settings.setValue("max_lines", "3000");
        settings.endGroup();

        settings.beginGroup("/IO");
            if(settings.value("No_rezerv","").toString().length() == 0)                // Чекбокс Не включать Резерв
                settings.setValue("No_rezerv", true);
            if(settings.value("No_empty","").toString().length() == 0)                 // Чекбокс Не включать пустые
                settings.setValue("No_empty", true);
            if(settings.value("Highlight","").toString().length() == 0)                 // Чекбокс Подсвечивать наложения
                settings.setValue("Highlight", true);
            if(settings.value("Parse_mode","").toString().length() == 0)                // Парсинг свойств. 0 - 7050, 1 - 5564
                settings.setValue("Parse_mode", 1);
        settings.endGroup();

     settings.endGroup();
 }

 // Открыть диалоговое окно Настроек программы
 void MainWindow::on_action_settings_triggered()
 {
     SettingsDialog dlg(this->ui->tabWidget->currentIndex(),this);
     if(dlg.exec() == QDialog::Accepted)
         readSettings(false);
 }

 void MainWindow::on_action_about_triggered()
 {
     aboutwindow dlg(this);
     dlg.exec();
 }

/*************REGUL**************/

// Событие на вкладке Regul при нажатии кнопки "Загрузить xml"
void MainWindow::on_btn_regulLoadXML_clicked()
{
    QStringList file_list = QFileDialog::getOpenFileNames(0,"Выберите все файл(ы) ЭЛСИ-ТМК.xml и ЭЛСИ-ТМК_Application.xml",this->recentPath,"*.xml");
    if(file_list.length() == 0)
        return;
    updateRecentPath(file_list);
    QVector<Elesy_XML> xml_objects;                         // Реальные объекты, которые были получены после парсинга Крейта и Application

    //Парсим xml
    try{
        parseACSFiles(file_list, ui->cbox_regulPLC->currentIndex(), std::make_tuple(ui->chb_RegulBus_OS->isChecked(), ui->chb_iec104s_OS->isChecked()), xml_objects);
    }
    catch(const std::runtime_error& e){
        QMessageBox::critical(nullptr,"Ошибка чтения файла", e.what(), QMessageBox::Ok);
        return;
    }

    // Если есть в любом крейте каналы
    bool crate_exists = false;
    for(auto & xml_obj : xml_objects)
        if(xml_obj.slaves_104.size() + xml_obj.slaves_mbs.size() > 0)
        {
            crate_exists = true;
            break;
        }
    // Выводим окно настроек для найденных модулей
    if(crate_exists)
    {
        dialog_RegulChannels dlg(xml_objects, this);
        if(dlg.exec() == QDialog::Rejected)
            return;
    }

    // Вносим изменения в Application
    modifyApplications(xml_objects);

    // Сохраняем
    QString folder = QFileDialog::getExistingDirectory (0,"Выберите место для сохранения", this->recentPath,QFileDialog::ShowDirsOnly);   
    if(folder.length() == 0)
        return;
    updateRecentPath(folder);
    saveModified(xml_objects, folder);

    QMessageBox::information(nullptr,"Информация","Готово!                ", QMessageBox::Ok);
}

// Событие на вкладке Regul при нажатии кнопки "Открыть таблицу xlsx"
void MainWindow::on_pb_openExellTable_withTemplates_clicked()
{
    QString template_folder;
    QString file = QFileDialog::getOpenFileName(0,"Выберите xlsx файл таблицы",this->recentPath,"*.xlsx");
    template_folder = this->recentPath + "\\templates";    
    if(file.length() == 0)
        return;
    updateRecentPath(file);

    QVector<templateSokol_data> v;

    readTemplateSokol_data(file, v);

    if(v.empty())
    {
        QMessageBox::critical(nullptr,"Ошибка", "Таблица не заполнена", QMessageBox::Ok);
        return;
    }

    QString folder = QFileDialog::getExistingDirectory (0,"Выберите место для сохранения", this->recentPath,QFileDialog::ShowDirsOnly);    
    if(folder.length() == 0)
        return;
    updateRecentPath(folder);

    wsriteNewSokol_Templates(folder,template_folder,v);
    QMessageBox::information(nullptr,"Информация","Готово!                ", QMessageBox::Ok);
}

// Событие на вкладке Regul при нажатии на выбор папки с шаблонами
void MainWindow::on_btn_TemplatesPath_clicked()
{
    QString p = this->ui->ledit_TemplatesPath->text();
    QString folder = QFileDialog::getExistingDirectory (0,"Выберите папку с шаблонами", p.length()>0 ? (QDir(p).exists() ? p : "." ) : this->recentPath ,QFileDialog::ShowDirsOnly);
    if(folder.length() == 0)
        return;
    updateRecentPath(folder);
    this->ui->ledit_TemplatesPath->setText(folder);    
    settings.setValue("/Settings/Regul/TemplatePath", folder);
}

// По изменению пути шаблонов показываем кол-во доступных шаблонов
void MainWindow::on_ledit_TemplatesPath_textChanged(const QString &arg1)
{
    QString p = this->ui->ledit_TemplatesPath->text();
    QDir directory(p);
    if(p.length()!=0 && directory.exists())
    {
        QStringList templates = directory.entryList(QStringList() << "*.xml" << "*.XML",QDir::Files);
        this->ui->label_TemplatesTotal->setText(QString("Доступно шаблонов: %1").arg(templates.size()));
        this->ui->btn_TemplateGen->setDisabled(templates.size() == 0);
    }
}

// Событие на вкладке Regul при нажатии на кнопку Загрузить csv (для генерации шаблонов модбас)
void MainWindow::on_btn_TemplateGen_clicked()
{
    QStringList file_list = QFileDialog::getOpenFileNames(0,"Выберите все файл(ы) Infinity Server.csv",this->recentPath,"*.csv");    
    if(file_list.length() == 0)
        return;
    updateRecentPath(file_list);

    QVector<template_modbus> result_templates;
    result_templates = parseModbusTemplates(file_list);

    QString folder = QFileDialog::getExistingDirectory (0,"Выберите место для сохранения", this->recentPath,QFileDialog::ShowDirsOnly);   
    if(folder.length() == 0)
        return;
     updateRecentPath(folder);

    saveModbusTemplates(result_templates, folder);

    QMessageBox::information(nullptr,"Информация","Готово!                ", QMessageBox::Ok);
}

// Парсим все csv файлы, возвращаем массив структур template_modbus
QVector<template_modbus> MainWindow::parseModbusTemplates(QStringList file_list)
{
    QMap<QString,template_modbus_5ch> templates;                // парсим все свойства и распихиваем их сюда
    QVector<template_modbus> result_templates;                  // Затем оставляем только валидный массив

    // Парсим .csv
    for(auto f : file_list)
    {
        QFile file(f);
        if(!file.open(QIODevice::ReadOnly))
            continue;

        QTextStream in(&file);
        while (!in.atEnd())
        {
            QString t = in.readLine();
            parseLineTemplates(t, templates);
        }
        file.close();
    }

    // Оставлем только валидное
    for(auto key : templates.keys())
    {
        if(templates[key].m1.isValid())   result_templates.push_back(templates[key].m1);
        if(templates[key].m2.isValid())   result_templates.push_back(templates[key].m2);
        if(templates[key].m3.isValid())   result_templates.push_back(templates[key].m3);
        if(templates[key].m4.isValid())   result_templates.push_back(templates[key].m4);
        if(templates[key].m5.isValid())   result_templates.push_back(templates[key].m5);
    }


    // Проверяем, есть ли шаблоны на диске
    QDir directory(this->ui->ledit_TemplatesPath->text());
    QStringList xml_files = directory.entryList(QStringList() << "*.xml" << "*.XML",QDir::Files);
    QVector<template_modbus> tmp_vec;           // Сюда попадут только те элементы, под которые на диске есть шаблон

    for(auto & t : result_templates)
        for(auto & s : xml_files)
            if(t.dev_template + ".xml" == s)
                tmp_vec.push_back(t);

    // Если каких-то шаблонов нет, показываем сообщение с их списком
    std::sort(result_templates.begin(), result_templates.end());
    std::sort(tmp_vec.begin(), tmp_vec.end());
    std::vector<template_modbus> difference;
    std::set_difference(result_templates.begin(), result_templates.end(),tmp_vec.begin(), tmp_vec.end(), std::back_inserter( difference ));
    QStringList xml_not_found;
    for(auto t : difference)
        xml_not_found << t.dev_template;
    xml_not_found = xml_not_found.toSet().toList();

    if(xml_not_found.size() > 0)
         QMessageBox::warning(nullptr,"Предупреждение",QString("Не были найдены следующие шаблоны:\n").append(xml_not_found.join('\n')), QMessageBox::Ok);

   // Удаляем дубликаты
    QVector<template_modbus> tmp_vec_result;
    for(auto & el : tmp_vec)
    {
        bool found = false;
        for(auto & new_el : tmp_vec_result)
            if(el.mapping == new_el.mapping && el.csv_name == new_el.csv_name)
            {
                found = true;
                break;
            }
        if(!found)
            tmp_vec_result.push_back(el);
    }

    // Теперь tmp_vec - список 100% проверенный
    return tmp_vec_result;
}

// Делаем преобразования в шаблонах модбас и сохраняем их
void MainWindow::saveModbusTemplates(QVector<template_modbus> & result_templates, QString folder)
{
    // сначала удаляем все папки modules предыдущие
    for(auto & templ : result_templates)
    {
        QDir dir_rtu(folder + "/" + templ.csv_name + QString("/modules"));
        QDir dir_tcp(folder + "/TCP_DEVICES");
        if(dir_rtu.exists())        dir_rtu.removeRecursively();
        if(dir_tcp.exists())        dir_tcp.removeRecursively();
    }

    // Для каждого шаблона
    for(auto & templ : result_templates)
    {
        // Считываем его содержимое в content
        QString template_path = this->ui->ledit_TemplatesPath->text() + "/" + templ.dev_template + ".xml";
        QFile file(template_path);
        file.open(QIODevice::ReadOnly | QIODevice::Text);
        QString content = file.readAll();
        file.close();
        // Заменяем название шаблона
        content.replace(templ.dev_template, templ.mapping);
        // Далее работаем с шаблоном, как с xml документом. Для этого загружаем content в doc
        pugi::xml_document doc;
        doc.load_string(content.toStdString().c_str(),pugi::parse_default);
        //Заменяем КП, IP адрес, версию шаблона
        pugi::xml_node kp_node = findNodeByAttribute(doc, "visiblename", "Адрес ведомого устройства");
        pugi::xml_node ip_node = findNodeByAttribute(doc, "visiblename", "IP адрес");
        pugi::xml_node ver_node = findNodeByName(doc, "Version");
        pugi::xml_node type_node = findNodeByName(doc, "Type");

        if(kp_node)                         kp_node.first_child().set_value(templ.station.toStdString().c_str());
        if(ip_node && templ.type == 1)      ip_node.first_child().set_value(QString("[%1]").arg(templ.ip.replace(".",", ")).toStdString().c_str());
        if(ver_node)                        ver_node.first_child().set_value(this->ui->ledit_TemplatesVersion->text().toStdString().c_str());
        if(type_node)
        {
            if(templ.type == 1)             type_node.first_child().set_value(ui->checkb_TemlatesOS->isChecked() ? "50089" : "40089");      // tcp
            else                            type_node.first_child().set_value(ui->checkb_TemlatesOS->isChecked() ? "50096" : "40096");      // rtu
        }

        // Сохраняем
        QString dir_path;
        if(templ.type == 1)
            dir_path = folder + "/TCP_DEVICES";                  // сохраняем шаблон tcp в отдельную папку
        else
            dir_path = folder + "/" + templ.csv_name + QString("/modules/Модуль %1/Канал %2").arg(templ.modbus_module).arg(templ.modbus_channel);
        QDir dir(dir_path);
        if (!dir.exists())
            dir.mkpath(".");
        doc.save_file(dir.path().append("/").append(templ.mapping).append(".xml").toStdWString().data(),"\t", pugi::format_indent | pugi::format_indent_attributes, pugi::encoding_utf8);

       //Копируем пустой файл DEV_ALL из ресурсов на диск, если его там нет
       if(!QFile::exists(dir_path + "/DEV_ALL.XML"))
       {
           QFile::copy(":/mbs_template.xml", dir_path + "/DEV_ALL.XML");
           QFile::setPermissions(dir_path + "/DEV_ALL.XML", QFileDevice::WriteUser | QFileDevice::ReadUser);
       }

       // Открываем DEV_ALL.xml как xml документ. И копируем узел configuration из шаблона в узел configurations DEV_ALL.xml
       pugi::xml_document doc_dev;
       openFileXml(doc_dev, dir_path + "/DEV_ALL.XML");
       pugi::xml_node cfgs_node = findNodeByName(doc_dev, "configurations");
       pugi::xml_node config_node =  findNodeByName(doc, "configuration");
       pugi::xml_node struct_node = findNodeByName(doc_dev, "ProjectStructure");
       if(!cfgs_node || !config_node || !struct_node)
           continue;

       cfgs_node.append_copy(config_node);
       pugi::xml_node obj_node = struct_node.append_child("Object");
       obj_node.append_attribute("Name").set_value(templ.mapping.toStdString().c_str());
       obj_node.append_attribute("ObjectId").set_value("aee2fe06-f677-434c-8c3d-69670ehui001");
       obj_node.append_attribute("xmlns").set_value("");

       // Добавляем пустышку для овнов
       if(templ.oven_dummy)
       {
           pugi::xml_document doc_dummy;
           QFile file(":/oven_dummy.xml");
           if(file.open(QIODevice::ReadOnly))
           {
               doc_dummy.load_string(QString(file.readAll()).toStdString().data(), pugi::parse_default);
               file.close();
           }
           pugi::xml_node cfg_dummy = doc_dummy.child("configuration");
           findNodeByName(doc_dummy, "Version").first_child().set_value(this->ui->ledit_TemplatesVersion->text().toStdString().c_str());
           findNodeByName(doc_dummy, "Type").first_child().set_value(ui->checkb_TemlatesOS->isChecked() ? "50096" : "40096");      // rtu

           cfgs_node.append_copy(cfg_dummy);
           obj_node = struct_node.append_child("Object");
           obj_node.append_attribute("Name").set_value("oven_dummy");
           obj_node.append_attribute("ObjectId").set_value("aee2fe06-f677-434c-8c3d-69670ehui001");
           obj_node.append_attribute("xmlns").set_value("");
       }

       doc_dev.save_file(dir_path.append("/DEV_ALL.XML").toStdWString().data(),"\t", pugi::format_indent | pugi::format_indent_attributes, pugi::encoding_utf8);
    }
}


/*************DEV STUDIO**************/
// Событие на вкладке DevStudio при нажатии кнопки "Загрузить csv"
void MainWindow::on_pb_loadCsv_dev_clicked()
{
    if(!this->ui->chb_Modbus_csv->isChecked() && !this->ui->chb_IEC870_csv->isChecked())
    {
        QMessageBox::critical(nullptr,"Ошибка", "Не выбран ни один протокол", QMessageBox::Ok);
        return;
    }
    QStringList file_list = QFileDialog::getOpenFileNames(0,"Выберите все файл(ы) Infinity Server.csv",this->recentPath,"*.csv");    
    if(file_list.length() == 0)
        return;
    updateRecentPath(file_list);

    elesy_csv data(file_list);

    if(this->ui->chb_IEC870_csv->isChecked() && data.iec_empty())
        QMessageBox::critical(nullptr,"Ошибка", "Не найдены адреса IEC 870-50", QMessageBox::Ok);
    if(this->ui->chb_Modbus_csv->isChecked() && data.mbs_empty())
         QMessageBox::critical(nullptr,"Ошибка", "Не найдены адреса Modbus", QMessageBox::Ok);

    if((this->ui->chb_IEC870_csv->isChecked() && !data.iec_empty()) || (this->ui->chb_Modbus_csv->isChecked() && !data.mbs_empty()))
    {
        QString folder = QFileDialog::getExistingDirectory (0,"Выберите место для сохранения", this->recentPath,QFileDialog::ShowDirsOnly);       
        if(folder.length() == 0)
            return;
         updateRecentPath(folder);

        if(this->ui->chb_IEC870_csv->isChecked())
            data.save_iec(folder);
        if(this->ui->chb_Modbus_csv->isChecked())
            data.save_modbus(folder);
        QMessageBox::information(nullptr,"Информация","Готово!                ", QMessageBox::Ok);
    }
}

// Событие на вкладке DevStudio при нажатии кнопки "Загрузить xml"
void MainWindow::on_pb_loadXml_dev_Sokol_clicked()
{
    QStringList file_list = QFileDialog::getOpenFileNames(0,"Выберите все карты уставок .xml",this->recentPath,"*.xml");   
    if(file_list.length() == 0)
        return;
     updateRecentPath(file_list);

    QMap<QString, QVector<sokol_node>> data_map;                        // QString - имя карты, QVector - адреса в этой карте

    for(QString & file : file_list)
    {
        pugi::xml_document doc;
        QVector<sokol_node> nodes;

        try{
           openFileXml(doc, file);
           getSokol_Nodes(doc, nodes);
           if(nodes.size() !=0)
                data_map[file.section("/",-1,-1)] = nodes;
        }
        catch(const std::runtime_error& e){
            QMessageBox::critical(nullptr,"Ошибка чтения файла", e.what(), QMessageBox::Ok);
            continue;
        }
    }

    if(data_map.empty())
        return;

    QString folder = QFileDialog::getExistingDirectory (0,"Выберите место для сохранения", this->recentPath,QFileDialog::ShowDirsOnly);    
    if(folder.length() == 0)
        return;
    updateRecentPath(folder);

    for(auto & key : data_map.keys())
        exportSokol_Map(data_map[key], folder + "/" + key);
    QMessageBox::information(nullptr,"Информация","Готово!                ", QMessageBox::Ok);
}

// Кнопка Пропатчить на водяной знак
void MainWindow::on_pb_HMI_RemoveWaterMark_clicked()
{
    QString file_name = QFileDialog::getOpenFileName(0,"Выберите alpha.hmi.ui.dll (win) или libalpha.hmi.ui.so (linux)",this->recentPath,"DLL or SO (*.dll *.so)");
    if(file_name.length() == 0)
        return;
    updateRecentPath(file_name);

    QByteArrayList marks = {"Лицензия не обнаружена или ограничена.",
                           "Использование %1 без лицензии допустимо только для разработки и испытания проектов автоматизации инжиниринговыми компаниями.",
                           "Не допускается использование нелицензированных компонентов конечными пользователями на объектах.",
                           "Все права защищены © %1"};
    QByteArray content;
    int count = 0;
    QFile file(file_name);
    if(!file.open(QIODevice::ReadWrite))
    {
        QMessageBox::critical(nullptr,"Ошибка чтения файла", QString("Невозможно открыть %1. Закройте Alpha.HMI и попробуйте снова").arg(file_name), QMessageBox::Ok);
        return;
    }
    content = file.readAll();
    for(QByteArray& mark : marks)
        for(int i = 0, buf_pos = 0; i != content.length(); ++i)
        {
            buf_pos = (mark.at(buf_pos) == content.at(i)) ? buf_pos + 1 : 0;
            if(buf_pos == mark.length())
            {
               for(int j = 0; j <= buf_pos; ++j)
                   content[i-j] = ' ';
               buf_pos = 0;
               count++;
            }
        }
    if(count > 0)
    {
        file.resize(0);
        file.write(content);
        file.close();
        QMessageBox::information(nullptr,"Информация",QString("Готово! Стерто %1 водяных знака                ").arg(count), QMessageBox::Ok);
    }
    else
    {
        file.close();
        QMessageBox::information(nullptr,"Информация","Не было найдено водяных знаков!                ", QMessageBox::Ok);
    }
}

// Кнопка генерации updateAlarm по Settings.xml
void MainWindow::on_pb_HMI_genUpdateAlarmFun_clicked()
{
    QString file_name = QFileDialog::getOpenFileName(0,"Выберите файл SETTINGS.xml (из папки support проекта HMI)",this->recentPath,"*.xml");
    if(file_name.length() == 0)
        return;
    updateRecentPath(file_name);

    hmi_settings set;
    parseHMISettins(file_name,set);

    QDir obj_dir(QFileInfo(file_name).dir());
    obj_dir.cdUp();
    if(!obj_dir.cd("objects"))
    {
        QMessageBox::critical(nullptr,"Ошибка", QString("Не найдена папка objects проекта HMI").arg(file_name), QMessageBox::Ok);
        return;
    }

    for(int i = 0; i != set.screens.size(); ++i)
    {
        QString screen_path = obj_dir.path() + "/" + set.screens.at(i).name + ".omobj";
        parseHMIScreen(screen_path, set, i);
    }

    // Сохраняем
    QString save_name = QFileDialog::getSaveFileName(0, "Выберите место для сохранения файла ",recentPath + "\\func.txt","*.txt");
    if(save_name.length() == 0)
        return;
    updateRecentPath(save_name);

    QFile file(save_name);
    if(!file.open(QFile::WriteOnly))
        return;
    file.write(set.to_string().toStdString().c_str());
    file.close();

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Готово!", "Файл был сохранен. Обновить автоматически unit.Global.updateAlarm() в проекте HMI?", QMessageBox::Yes|QMessageBox::No);
    if(reply == QMessageBox::Yes)
    {
        pugi::xml_document doc;
        try{
            openFileXml(doc, obj_dir.path() + "/Global.omobj");
            pugi::xml_node node = findNodeByAttribute(doc.root(),"name","getButtonAlarm");
            node = node.find_child_by_attribute("kind", "javascript");
            node.first_child().set_value(set.to_string().toStdString().c_str());
            doc.save_file(QString(obj_dir.path() + "/Global.omobj").toStdWString().data(), "\t", pugi::format_indent, pugi::encoding_auto);
        }
        catch(const std::runtime_error& e){return;}
    }
}

// Событие на кнопке получения таблицы функций и экранов HMI
void MainWindow::on_pb_HMI_genFunctionTable_clicked()
{
    QString file_name = QFileDialog::getOpenFileName(0,"Выберите файл SETTINGS.xml (из папки support проекта HMI)",this->recentPath,"*.xml");
    if(file_name.length() == 0)
        return;
    updateRecentPath(file_name);

    hmi_settings set;
    parseHMISettins(file_name,set);

    QDir obj_dir(QFileInfo(file_name).dir());
    obj_dir.cdUp();
    if(!obj_dir.cd("objects"))
    {
        QMessageBox::critical(nullptr,"Ошибка", QString("Не найдена папка objects проекта HMI").arg(file_name), QMessageBox::Ok);
        return;
    }
    for(int i = 0; i != set.screens.size(); ++i)
    {
        QString screen_path = obj_dir.path() + "/" + set.screens.at(i).name + ".omobj";
        parseHMIScreen(screen_path, set, i);
    }

    // Создаем xls файл
    xlnt::workbook wb;
    set.to_xlsx_fun(wb);

    QString save_name = QFileDialog::getSaveFileName(0, "Выберите место для сохранения файла ",recentPath + "\\func_table.xlsx","*.xlsx");
    if(save_name.length() == 0)
        return;
    updateRecentPath(save_name);

   // Сохраняем xlsx файл
   try{
       wb.save(save_name.toLocal8Bit().toStdString());
       QMessageBox::information(nullptr,"Информация","Готово!                ", QMessageBox::Ok);
   }
   catch(...){
       QMessageBox::critical(this, "Ошибка сохранения xlsx файла", "Возможно сохраняемый файл открыт");
   }
}



/*************ИНФ. ОБЕСПЕЧЕНИЕ**************/

void MainWindow::IO_update_data()
{
    // общие счетчики сигналов по всем усо
    int count_all   = 0,
        reserv_all  = 0,
        empty_all   = 0,
        dubl_all    = 0;
    // Проверяем, есть ли некорректные адреса (совпадающие по адресу и типу сигнала)
    for(auto & v : IO_info_map)
        for(auto & el_1 : v)
            for(auto & el_2 : v)
                if(el_1.address == el_2.address && el_1.protocol_type_int == el_2.protocol_type_int)
                    el_1.dubl_count++;

    QTableWidgetItem * protoitem = new QTableWidgetItem();
    protoitem->setTextAlignment(Qt::AlignCenter);

    for(auto key : IO_info_map.keys())
    {
        ui->tableWidget_IO->insertRow(ui->tableWidget_IO->rowCount());
        QStringList list;
        list << QString::number(key)
             << QString::number(IO_info_map[key].count())
             << QString::number(getRezervSize(IO_info_map[key]))
             << QString::number(getEmptySize(IO_info_map[key]))
             << QString::number(getNumberofDublicates(IO_info_map[key]));
        count_all   += IO_info_map[key].count(),
        reserv_all  += getRezervSize(IO_info_map[key]),
        empty_all   += getEmptySize(IO_info_map[key]),
        dubl_all    += getNumberofDublicates(IO_info_map[key]);

        for(int i = 0; i != list.size(); ++i)
        {
            QTableWidgetItem * newitem = protoitem->clone();
            newitem->setText(list.at(i));
            if(i == 0)
                newitem->setCheckState(Qt::Checked);

            ui->tableWidget_IO->setItem(ui->tableWidget_IO->rowCount() - 1, i, newitem);
        }
    }

    ui->pb_IO_gen->setEnabled(IO_info_map.keys().count() > 0);
    ui->pb_IO_gen_txt->setEnabled(IO_info_map.keys().count() > 0);
    if(IO_info_map.keys().count() <= 1)
        return;

    QStringList list;
    list << QString("Всего")
         << QString::number(count_all)
         << QString::number(reserv_all)
         << QString::number(empty_all)
         << QString::number(dubl_all);

    ui->tableWidget_IO->insertRow(ui->tableWidget_IO->rowCount());
    for(int i = 0; i != list.size(); ++i)
    {
        QTableWidgetItem * newitem = protoitem->clone();
        newitem->setText(list.at(i));
        ui->tableWidget_IO->setItem(ui->tableWidget_IO->rowCount() - 1, i, newitem);
    }
}
void MainWindow::IO_reset_data()
{
    IO_info_map.clear();
    ui->tableWidget_IO->setRowCount(0);
    ui->pb_IO_gen->setEnabled(false);
    ui->pb_IO_gen_txt->setEnabled(false);
}
void MainWindow::IO_initProgressBar()
{
    pBar_range_count = 0;
    for(auto & v : IO_info_map)
        pBar_range_count += v.size();
    this->pProgressBar->setRange(0, pBar_range_count);
    pProgressBar->setValue(1);
    pProgressBar->show();
    pBar_range_count = 0;
}
void MainWindow::IO_initWorkbook(xlnt::workbook & wb)
{
    wb.remove_sheet(wb.active_sheet());
    QStringList headers;
    QStringList headers_width;
    headers << "Описание" << "Станция (КП)" << "Тип (код)" << "Тип (название)" << "Адрес (dec)" << "Адрес (hex)";
    headers_width << "90" << "15" << "15" << "15" << "15" << "15";

    int i = 0;
    for(auto & key : IO_info_map.keys())
    {
        QTableWidgetItem* item  = 0;
        QApplication::processEvents();
        for(int row_index = 0; row_index != ui->tableWidget_IO->rowCount(); ++row_index)
        {
           item = ui->tableWidget_IO->item(row_index, 0);
           if(item->text() == QString::number(key))
               break;
        }
        if(item->checkState() != Qt::Checked)
            continue;

        QString title = QString("УСО-%1").arg(key);
        wb.create_sheet(i++).title(title.toStdString());
        xlnt::worksheet sheet = wb.sheet_by_title(title.toStdString());

        int j = 1;
        for(QString& header : headers)
        {
            xlnt::cell cell = sheet.cell(j++,1);
            cell.value(header.toStdString());
            apply_style(cell,true, true,xlnt::horizontal_alignment::center, xlnt::vertical_alignment::center);
            sheet.column_properties(cell.column()).custom_width = true;
            sheet.column_properties(cell.column()).width = headers_width[j-2].toUInt();
        }
    }
}
void MainWindow::IO_DataToWorkbook(xlnt::workbook & wb)
{
    for(auto & key : IO_info_map.keys())
    {
        QApplication::processEvents();
        xlnt::worksheet sheet;
        try{
            sheet = wb.sheet_by_title(QString("УСО-%1").arg(key).toStdString());
        }
        catch(xlnt::key_not_found &e)
        {
            pBar_range_count += IO_info_map[key].size();
            pProgressBar->setValue(pBar_range_count);
            continue;
        }

        for(int k = 0, pos = 1; k != IO_info_map[key].count(); ++k)
        {
            pProgressBar->setValue(++pBar_range_count);
            QApplication::processEvents();
            io_csv tmp_info = IO_info_map[key].at(k);

            if(ui->chb_IO_Rezerv->isChecked() && tmp_info.is_Rezerv())
                continue;
            if(ui->chb_IO_Empty->isChecked() && tmp_info.is_Empty())
                continue;

            ++pos;
            sheet.cell(1, pos).value(tmp_info.description.toStdString());
            sheet.cell(2, pos).value(tmp_info.station);
            sheet.cell(3, pos).value(tmp_info.protocol_type_int);
            sheet.cell(4, pos).value(tmp_info.protocol_type_str.toStdString());
            sheet.cell(5, pos).value(tmp_info.address);
            QString hex_addr = QString("0x") + QString::number(tmp_info.address, 16);
            sheet.cell(6, pos).value(hex_addr.toStdString());

            for(int l = 1; l != 7; ++l)
            {
                apply_style(sheet.cell(l, pos),true, false);
                if(l != 1)
                    apply_style(sheet.cell(l, pos),true, false, xlnt::horizontal_alignment::center);
                if(tmp_info.dubl_count > 1 && ui->chb_IO_Highlight->isChecked())
                     sheet.cell(l, pos).fill(xlnt::fill::solid(xlnt::color::yellow()));
            }
        }
    }
}

void MainWindow::on_rb_IO_Complex_clicked()
{
    IO_reset_data();
}
void MainWindow::on_rb_IO_Native_clicked()
{
    IO_reset_data();
}
void MainWindow::on_pb_IO_load_clicked()
{
    IO_reset_data();

    QStringList f_list = QFileDialog::getOpenFileNames(0,"Выберите csv файл(ы)",recentPath,"*.csv");
    if(f_list.size() == 0)
        return;
    updateRecentPath(f_list);

    for(auto f : f_list)
    {
        QFile file(f);
        if(!file.open(QIODevice::ReadOnly))
            continue;

        QTextStream in(&file);
        while (!in.atEnd())
        {
            QString t = in.readLine();
            if(ui->rb_IO_Native->isChecked())
                parse_line_native(t);
            if(ui->rb_IO_Complex->isChecked())
                parse_line_complex(t);
        }
        file.close();
    }
    IO_update_data();

    // сортировка по тегам
    for(auto key : IO_info_map.keys())
        qSort(IO_info_map[key]);

}
void MainWindow::on_pb_IO_gen_clicked()
{
    setCursor(Qt::CursorShape::WaitCursor);
    QApplication::processEvents();
    xlnt::workbook wb;
    IO_initProgressBar();
    IO_initWorkbook(wb);
    IO_DataToWorkbook(wb);

    pProgressBar->hide();
    QString file = QFileDialog::getSaveFileName(0, "Выберите место для сохранения файла ИО",recentPath + "\\МЭК-104 Инф. обеспечение.xlsx","*.xlsx");

    if(file.length() == 0)
    {
        setCursor(Qt::CursorShape::ArrowCursor);
        return;
    }
     updateRecentPath(file);

    try{
        wb.save(file.toLocal8Bit().toStdString());
    }
    catch(...){
        QMessageBox::critical(this, "Ошибка сохранения", "Возможно сохраняемый файл открыт");
    }
    setCursor(Qt::CursorShape::ArrowCursor);
    QMessageBox::information(nullptr,"Информация","Готово!                ", QMessageBox::Ok);
}
void MainWindow::on_tabWidget_currentChanged(int index)
{
    this->IO_reset_data();
}
void MainWindow::on_pb_IO_gen_txt_clicked()
{
    QString filename = QFileDialog::getSaveFileName(0, "Выберите место для сохранения файла",recentPath + "\\fileDescription.txt","*.txt");   
    if(filename.length() == 0)
        return;
     updateRecentPath(filename);

    QFile file(filename);
    if (file.open(QIODevice::ReadWrite)) {
        QTextStream stream(&file);
        stream.setCodec("UTF-8");
        for(auto & key : IO_info_map.keys())
            for(int k = 0; k != IO_info_map[key].count(); ++k)
            {
                io_csv tmp_info = IO_info_map[key].at(k);

                if(ui->chb_IO_Rezerv->isChecked() && tmp_info.is_Rezerv())
                    continue;
                if(ui->chb_IO_Empty->isChecked() && tmp_info.is_Empty())
                    continue;
                stream << tmp_info.station << ";" << tmp_info.address << ";" << tmp_info.description << Qt::endl;
            }

    }
    QMessageBox::information(nullptr,"Информация","Готово!                ", QMessageBox::Ok);
}

/************* MK LOGIC **************/
void MainWindow::on_btn_isagrafLoadXML_clicked()
{
    QStringList file_list = QFileDialog::getOpenFileNames(0,"Выберите все файл(ы) ЭЛСИ-ТМК_Application.xml",this->recentPath,"*.xml");
    if(file_list.length() == 0)
        return;
    updateRecentPath(file_list);

    QString folder = QFileDialog::getExistingDirectory (0,"Выберите место для сохранения", this->recentPath,QFileDialog::ShowDirsOnly);    
    if(folder.length() == 0)
        return;
    updateRecentPath(folder);

    for(QString & filename : file_list)
    {
        // Создаем папку для сохранения файлов
        QDir dir(folder + "\\" + QFileInfo(filename).fileName().replace(".xml",""));
        if (!dir.exists())
            dir.mkpath(".");

         // Создаем xls файл
       /* xlnt::workbook wb;

        isagraf_xlsx_edit(wb, filename);
        // Сохраняем xlsx файл
        try{
            QString path = dir.absolutePath() + "\\" + QFileInfo(file_list.at(0)).fileName().replace("_application.xml",".xlsx");
            wb.save(path.toLocal8Bit().toStdString());
        }
        catch(...){
            QMessageBox::critical(this, "Ошибка сохранения", "Возможно сохраняемый файл открыт");
        }
        */
        QString path = dir.absolutePath() + "\\" + QFileInfo(file_list.at(0)).fileName().replace("_application.xml",".csv");
        isagraf_csv_export(filename,path);

        // Экспорт ST текста
        isagraf_ST_export(filename, dir.absolutePath(), ui->chb_isagrafSplitLargeFiles->isChecked(), ui->lineEdit_isagraf_maxlines->text().toInt());
    }

    QMessageBox::information(nullptr,"Информация","Готово!                ", QMessageBox::Ok);
}

void MainWindow::on_chb_isagrafSplitLargeFiles_stateChanged(int arg1)
{
    ui->lineEdit_isagraf_maxlines->setDisabled(!ui->chb_isagrafSplitLargeFiles->isChecked());
}


/************* ДРУГОЕ **************/
void MainWindow::on_pb_Other_DenyRequest_clicked()
{
    QStringList file_list = QFileDialog::getOpenFileNames(0,"Выберите все файл(ы) ЭЛСИ-ТМК.xml",this->recentPath,"*.xml");
    if(file_list.length() == 0)
        return;
    updateRecentPath(file_list);

    QVector<QStringList> result;
    // Для каждого файла
    for(QString & file_name : file_list)
    {
        pugi::xml_document doc;
        openFileXml(doc,file_name);
        find_tn713_modules(doc, result);
        doc.save_file(file_name.replace(".xml", "_req_mod.xml").toStdWString().data(), "\t", pugi::format_indent, pugi::encoding_auto);
    }
    QMessageBox::information(nullptr,"Информация","Готово! Модифицированные файлы были сохранены рядом с оригинальными.", QMessageBox::Ok);
    /*for(auto & list : result)
        for(auto s : list)
            qDebug() << s;
            */
}




