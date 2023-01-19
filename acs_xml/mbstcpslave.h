#pragma once

#include <QString>
#include <QVector>
#include <pugixml/pugixml.hpp>

// Функции определены в elesy_xml.cpp
pugi::xml_node findNodeByAttribute(pugi::xml_node root, QString attr_name, QString attr_val);
pugi::xml_node findNodeByName(pugi::xml_node root, QString node_name);

// Один канал Modbus TCP
class modbus_tcp_channel{
public:
    QString VarName;							//Slave_mbs1_1_3_BOOL_0
    QString Descr;
    QString ChannelName;
    QString GroupName;
    QString Type;							// Holding Registers, ...
    int Offset;
    int Length;
    int index;                              // Просто внутренний индекс в нашем массиве каналов

    modbus_tcp_channel(QString _var, QString _descr, QString _ch_name, QString _group_name, QString _type, int _offset, int _len, int _ind);

    // по имени переменной получить ее тип
    QString getType();
    // Канал в строковом представлении
    QString toString();
};



// Один модуль Modbus TCP Slave
class mbsTCPSlave
{
public:
    mbsTCPSlave(const pugi::xml_node& node); //Инициализируем класс, передавая xml узел
    void save(QString & path);                          // Сохранение данных

    QString module_name;                       // Название модуля
    bool    checked;                           // Флаг того, что модуль выбран для работы, иначе пропускается
    bool united_memory;                        // Флаг использования объединенной памяти
    QVector<modbus_tcp_channel> channels;       // Список каналов

    // Среди всех каналов вычсляет последний адрес, необходимый для размещения их в памяти в массиве [0 .. getLastByteAddress()]
    int getLastByteAddress();
};

