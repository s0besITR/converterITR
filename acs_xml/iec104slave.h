#pragma once

#include <QString>
#include <QVector>
#include <pugixml/pugixml.hpp>

// Функции определены в elesy_xml.cpp
pugi::xml_node findNodeByAttribute(pugi::xml_node root, QString attr_name, QString attr_val);
pugi::xml_node findNodeByName(pugi::xml_node root, QString node_name);

// Один канал МЭК-104
class iec104_channel{
protected:
    QString name;                           // Имя канала
    QString description;                    // Описание
    uint type_id;                           // Идентификатор типа
    uint addr;                              // Адрес
    QString variable;                       // Имя переменной, куда идет маппинг
 public:
    iec104_channel(QString _name, QString _description, uint _type_id, uint _addr, QString _variable);
};

// Один канал данных МЭК-104
class iec104_channel_data : iec104_channel{
 public:
    bool autotime;                          // Автоприсвоение метки времени (по умолчанию false, дергается в ui)
    iec104_channel_data(QString _name, QString _description, uint _type_id, uint _addr, QString _variable, bool _autotime);
    QString toString();
};
// Один канал команд МЭК-104
class iec104_channel_cmd : iec104_channel{
public:
    uint select_exec_time;                  // Время выборки
    iec104_channel_cmd(QString _name, QString _description, uint _type_id, uint _addr, QString _variable, uint _select_exec_time);
    QString toString();
};


// Один модуль МЭК-104 Slave
class iec104Slave
{
public:
    iec104Slave(const pugi::xml_node& node); //Инициализируем класс, передавая xml узел
    void save(QString & path);              // Сохранение данных

    QString module_name;                    // Название модуля
    bool    checked;                        // Флаг того, что модуль выбран для работы, иначе пропускается  
    QVector<iec104_channel_data> data_channels;     // Список каналов данных
    QVector<iec104_channel_cmd> cmd_channels;       // Список каналов команд    
};


