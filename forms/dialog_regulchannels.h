#ifndef DIALOG_REGULCHANNELS_H
#define DIALOG_REGULCHANNELS_H

#include <QDialog>
class Elesy_XML;

namespace Ui {
class dialog_RegulChannels;
}

class dialog_RegulChannels : public QDialog
{
    Q_OBJECT

public:
    dialog_RegulChannels(QVector<Elesy_XML> & xml_objects, QWidget *parent = nullptr);
    ~dialog_RegulChannels();

private slots:
    void on_buttonBox_accepted();
    void OnTableWidgetCellChanged(int r,int c);

private:
    Ui::dialog_RegulChannels *ui;
    QVector<Elesy_XML> & _xml_objects;
    void initTable();
    bool updateUSOList(QString& module_name, QString& uso_name);

    // Настройки из реестра
    uint iec104_exec_time = 0;
    uint iec104_EDC_exec_time = 0;
    bool iec104_autotime = false;
    bool iec104_EDC_autotime = false;
    bool MBS_MemoryUnited = true;

    void initSettings();
};

#endif // DIALOG_REGULCHANNELS_H
