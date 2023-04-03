#pragma once
#include <QString>
#include <QVector>
#include <pugixml/pugixml.hpp>
#include "xlnt/xlnt.hpp"

class hmi_actions
{
public:
    hmi_actions();
};

// Каждый экран имеет имя, список тегов. А так же общее кол-во элементов и кол-во всех тегов
struct hmi_screen_info {
    QVector<QString> tags;
    QString name;
    QString code;
    hmi_screen_info(QString name, QString code);
    int elem_count = 0;
    int tags_count = 0;
};

// Каждый тип содержит свое имя и массив тегов
struct hmi_type {
    QString name;
    QVector<QString> tags;
    hmi_type(QString name);
};

// Забираем информацию по каждому агрегату у которого есть поле FUNCTION
struct hmi_obj_type {
    QString display_name;
    QString base_type;
    QString tag;
    QString function;
    QString screen_name;
};

// Настройки содержат массив типов и массив экранов
struct hmi_settings {
    QVector<hmi_type> types;
    QVector<hmi_screen_info> screens;
    QVector< hmi_obj_type> objs;                    // только для таблицы типов

    bool isValid();                                 // Валидные ли настройки
    QString to_string();

    void to_xlsx_fun(xlnt::workbook & wb);          // Выгрузить QVector< hmi_obj_type> objs в wb
    QStringList getUniqFun();                       // Получить из QVector< hmi_obj_type> objs список QStringList уникальных типов
};


//Структура для обхода всего документа от начала до конца
struct simple_walker : pugi::xml_tree_walker
{
    hmi_settings& setup;                                // Ссылка объект с параметрами
    int screen_index;                                   // Индекс текущего экрана

    simple_walker(hmi_settings& setup, int index);
    virtual bool for_each(pugi::xml_node& node);
    QString getAlias(pugi::xml_node& node);             // Вытащить поле in_alias из узла node
    bool addIfNotDuplicate(QVector<QString>& tags, QString val);    // Добавить val в tags если его там нет
};

// Структура для генерации HMI объектов
struct hmi_objects{
    QString type;
    QString name;
    QString tag;
    QString location;
    QString room;

    bool valid = false;

    hmi_objects(QString s);
};

//Генерация объектов функции
void load_obj_map(QString & path, QMap<QString,QVector<hmi_objects>> & obj_map);
void load_obj_vector(QString & path, QVector<hmi_objects> & obj_vec);
void gen_hmi_form(QMap<QString,QVector<hmi_objects>> & obj_map,  pugi::xml_document & types_doc, QString save_name);
void gen_hmi_nolink_form(QVector<hmi_objects> & obj_vec,  pugi::xml_document & types_doc, QString save_name);
void openFileXmlFromTemplate(pugi::xml_document& doc, const QString& filename);
void change_mnemo(pugi::xml_node mnemo_obj, hmi_objects & obj, int & x_cor);
void change_room(pugi::xml_node room_obj, hmi_objects obj, int & y_cor);

// Чтение файла SETTINGS.xml в set
void parseHMISettins(QString filename, hmi_settings & set);
// Парсинг каждого файла экранов и обновление set
bool parseHMIScreen(QString screen_path, hmi_settings & set, int screen_index);
void openFileXml(pugi::xml_document& doc, const QString& filename);     // из elesy_xml.h
pugi::xml_node findNodeByName(pugi::xml_node root, QString node_name);  // из elesy_xml.h
pugi::xml_node findNodeByAttribute(pugi::xml_node root, QString attr_name, QString attr_val);   // из elesy_xml.h
