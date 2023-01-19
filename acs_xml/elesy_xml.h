#pragma once

#include <QString>
#include <QVector>
#include <QFile>
#include "iec104slave.h"
#include "mbstcpslave.h"
#include <utility>
#include <tuple>

//#include <pugixml/pugixml.hpp>
class Elesy_XML;
namespace enums{
    enum plc {r500, r200};
};


// Открыть xml документ filename и загрузить его в doc
void openFileXml(pugi::xml_document& doc, const QString& filename);

// Загрузить шаблон глобальных переменных в doc
void load_gvl_from_template(pugi::xml_document & doc,QString name, QVector<std::pair<QString,QString>> attributes);

// Из списка выбранных файлов получаем список объектов Elesy_XML
void parseACSFiles(QStringList files, uint plc, std::tuple<bool, bool> os_flags, QVector<Elesy_XML> & result_vector);

// Найти узел по атрибуту
pugi::xml_node findNodeByAttribute(pugi::xml_node root, QString attr_name, QString attr_val);
// Найти узел по имени
pugi::xml_node findNodeByName(pugi::xml_node root, QString node_name);

// Модифицировать элемент application_doc в векторе объектов Elesy_XML
void modifyApplications(QVector<Elesy_XML>& xmls);

//Сохранить все изменения на диск в папку folder
void saveModified(QVector<Elesy_XML>& xmls, QString folder);


// Класс представляет собой одну пропарсенную пару Crate-Application
class Elesy_XML
{
public:
    /*  Конструктор класса Elesy_XML
        cfg_path[0]         - путь до файла USOx.xml
        cfg_path[1]         - путь до файла USOx_Application.xml
        ide                 - индекс выбранной ide (Astra, Regul) в комбобоксе программы
        plc                 - индекс выбранного plc (R500, R200) в комбобоксе программы
    */
    Elesy_XML(std::pair<QString, QString> cfg_path, uint plc, std::tuple<bool, bool> os_flags);
    ~Elesy_XML() = default;
    Elesy_XML(const Elesy_XML & copy);      // Переопределил конструктор копирования, потому что app_doc не скопируешь автоматически

    // Поля из UI
    uint _plc;                              // Выбранный ПЛК (enum plc)
    bool _regul_bus_os;                     // Шина ОС
    bool _iec104s_os;                       // МЭК104 ОС

    // Внутренние поля
    QString _usoName;                       // Имя УСО
    QVector<QString> triggers;              // Переменные triggers
    pugi::xml_document app_doc;             // Храним в ОЗУ считанный документ _Application.xml для дальнейшего преобразования
    QVector<iec104Slave> slaves_104;        // Список модулей МЭК-104 Slave
    QVector<mbsTCPSlave> slaves_mbs;        // Список модулей Modbus TCP Slave

public:
    // Модифицируем свой Application в зависимости от _ide и _plc
    void modifyApplication();
    // Сохраняем application, каналы
    void save(QString& folder);
private:
    // Парсинг крейта, загрузка каналов в списки slaves_104 и slaves_mbs
     void load_crate(QString filename);
     // Просматриваем Application, заполняем масив QVector<QString> triggers. Делаем это в конструкторе, сразу после загрузки документа
     void parseTriggers();

     // Стандартные текстовые замены
     void standartTextReplaces();
     // dInitFlag меняем атрибут retain обычно на false
     void set_dInitFlag(bool val);
     // В глобальные переменные добавляем тригера
     void addTrigers();
     // В глобальных переменных меняем retain на persistent
     void changeRatainToPersistent();
     // Вырезаем из секции fb все переменные variable settings, и вставляем их в конец field секции
     void cutVarsFromFB_toField();
     // Вырезаем из секции fb все переменные variable settings, что бы потом вставить их в конец field секции
     QByteArray getFBVars(pugi::xml_node * fb);

     // Выносим BROADCAST переменные в Shared секцию глобальных переменных (Павелецкая, Терехово)
     void add_Redundancy();
      // Выносим Crossed и Shared переменные в отдельные секции глобальных переменных (Пока отказались от ОС резервирования)
     void add_Redundancy_OS();
     // Новое резервирование Печатники, Текстильщики
      void add_Redundancy_RT();
     // В shared_gvl добавляем переменные StartAddress и End Address
     void add_StartEndVariable(pugi::xml_document & doc, bool start);
     // Добавляем атрибут ps.add_redundancy узлу
     void add_redundancy_pou(pugi::xml_node * n);

     // Узнать тип резервирования (0 - старый shared (терехово, павелецкая); 1 - OS (нет станций) shared, crossed; 2 - новый shared (текстиль, печат)
     // есть глобальная переменная M_REGUL - 0, M_REGUL_OS - 1, M_REGUL_RT - 2
     int getRedundancyType();
     void addRedundancyFB_toSharedGVL(pugi::xml_node var);
     void addRedundancyFB_toCrossedGVL(pugi::xml_node cr_node, QVector<QString> v);

     // Добавить в глобальные переменные modbus данные (Старый вариант, без объединенной памяти)
     void addMBS_GVL();
     void addMBS_GVL_United();
     void replaceModbusVarsToPointers();

};


