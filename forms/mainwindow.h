#pragma once

#include <QMainWindow>
#include "dialog_regulchannels.h"
#include <QSettings>
#include <xlnt/xlnt.hpp>
#include "acs_csv/elesy_csv.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QProgressBar;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    Ui::MainWindow *ui;
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void readSettings(bool boot = true);
    void wirteDefaultSettings();

    //ИО методы
    void IO_update_data();
    void IO_reset_data();
    void IO_initProgressBar();
    void IO_initWorkbook(xlnt::workbook & wb);
    void IO_DataToWorkbook(xlnt::workbook & wb);


private slots:
    void on_btn_regulLoadXML_clicked();
    void on_ledit_TemplatesPath_textChanged(const QString &arg1);
    void on_btn_TemplatesPath_clicked();
    void on_btn_TemplateGen_clicked();
    void on_action_settings_triggered();
    void on_pb_loadCsv_dev_clicked();
    void on_pb_loadXml_dev_Sokol_clicked();
    void on_pb_openExellTable_withTemplates_clicked();
    void on_btn_isagrafLoadXML_clicked();
    void on_chb_isagrafSplitLargeFiles_stateChanged(int arg1);
    void on_pb_HMI_RemoveWaterMark_clicked();
    void on_pb_HMI_genUpdateAlarmFun_clicked();
    void on_pb_HMI_genFunctionTable_clicked();

    //ИО слоты
    void on_rb_IO_Complex_clicked();
    void on_rb_IO_Native_clicked();
    void on_pb_IO_load_clicked();
    void on_pb_IO_gen_clicked();    
    void on_tabWidget_currentChanged(int index);
    void on_pb_IO_gen_txt_clicked();

    void on_pb_Other_DenyRequest_clicked();

    void on_action_about_triggered();

private:
    QString recentPath;
    QSettings settings;
    QProgressBar * pProgressBar;
    int pBar_range_count;
    // методы для инициализации вкладок
    void tab_0_init();
    void tab_1_init();
    void tab_2_init();

    // вспомогательные методы  
    void updateRecentPath(QString path);
    void updateRecentPath(QStringList files);
    QVector<template_modbus> parseModbusTemplates(QStringList file_list);
    void saveModbusTemplates(QVector<template_modbus> & result_templates, QString folder);   

    //Водяной знак
    int removeWaterMark(QByteArray & content, QByteArray & mark);
    QByteArray utf8str_to_utf16QByteArray(QString utf8_s);
};
