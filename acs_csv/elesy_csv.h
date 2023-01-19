#pragma once
#include <QString>
#include <QStringList>
#include <QMap>
#include <QFile>
#include <QTextStream>
#include <QtXml>
#include <QVector>
#include <pugixml/pugixml.hpp>

// Модбас узел в карте уставок (только Сокол депо)
struct sokol_node {
    QString tag;
    QString segment;
    int adr;
    int bit;
};

// Заполнить массив nodes из документа
void getSokol_Nodes(pugi::xml_document &doc, QVector<sokol_node> &nodes);
// Сохранить карту
void exportSokol_Map(QVector<sokol_node> &nodes, QString path);


// Структура csv для парсинга шаблонов
struct template_modbus{
    uint type;                                  // RTU = 0/TCP= 1       (8814)
    QString station;                            // Адрес КП             (8888, 8816, 8827, 8833, 8911)
    QString modbus_module;                     // Модбас модуль         (8810, 8817, 8828, 8834, 8912)
    QString modbus_channel;                     // Модбас канал         (8811, 8818, 8829, 8835, 8913)
    QString mapping;                            // Маппинг              (8812, 8819, 8830, 8836, 8914)
    QString ip;                                 // IP адрес             (8813, 8820, 8831, 8837, 8915)
    QString dev_template;                       // Шаблон устройства    (8821, 8822, 8832, 8838, 8916)
    QString csv_name;                           // Имя CSV
    bool oven_dummy;                            //
    template_modbus();

    bool operator<(const template_modbus& other);

    bool isValid();
};
struct template_modbus_5ch{
    template_modbus m1;
    template_modbus m2;
    template_modbus m3;
    template_modbus m4;
    template_modbus m5;
};

void parseLineTemplates(QString s, QMap<QString,template_modbus_5ch> & template_map);


// Один тег модбас
struct mbs{
    QString tag;
    QString seg;
    QString seg_descr;
    unsigned int station;
    unsigned int addr;
    int bit;

    mbs(QString _tag, QString hex_val);
    QString to_string();                        // только для отладки
};

// один тег МЭК
struct iec104
{
    unsigned int    station;
    unsigned int    protocol_type_int;
    unsigned int    address;
    QString tag;

    iec104(QString _station, QString _protocol_type, QString _address, QString _tag);
};

// Это сразу все csv, не один файл
class elesy_csv
{
public:
    elesy_csv(QStringList csv_files);
    void save_modbus(QString dir);
    void save_iec(QString dir);
    bool mbs_empty();
    bool iec_empty();

private:
    // uint - номер станции, Qvector - массив адресов на этой станции
    QMap<uint, QVector<mbs>> modbus_map;
    QMap<uint,QVector<iec104>> iec_map;
    void parse_line(QString str);
};

