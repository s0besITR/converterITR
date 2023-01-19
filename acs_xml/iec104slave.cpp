#include "acs_xml/iec104slave.h"
#include <QMap>

iec104_channel::iec104_channel(QString _name, QString _description, uint _type_id, uint _addr, QString _variable)
    : name(_name), description(_description), type_id(_type_id), addr(_addr), variable(_variable) {}

iec104_channel_data::iec104_channel_data(QString _name, QString _description, uint _type_id, uint _addr, QString _variable, bool _autotime)
    : iec104_channel(_name, _description, _type_id, _addr, _variable), autotime(_autotime) {}

iec104_channel_cmd::iec104_channel_cmd(QString _name, QString _description, uint _type_id, uint _addr, QString _variable, uint _select_exec_time)
    : iec104_channel(_name, _description, _type_id, _addr, _variable), select_exec_time(_select_exec_time) {}

QString iec104_channel_data::toString()
{
    return QString("<Item"
                        " Name=\"%1\""
                        " Descr=\"%2\""
                        " TypeId=\"%3\""
                        " CustomTypeId=\"0\""
                        " AutoTime=\"%4\""
                        " HighBound=\"32000\""
                        " LowBound=\"0\""
                        " Scale=\"0\""
                        " IoAdr=\"%5\""
                        " MapVarName=\"%6\""
                        " Cycle=\"0\""
                        " DeadBand=\"0\""
                    " />").arg(name).arg(description).arg(type_id).arg(autotime ? "true" : "false").arg(addr).arg(variable);
}

QString iec104_channel_cmd::toString()
{
    return QString("<Item"
                       " Name=\"%1\""
                       " Descr=\"%2\""
                       " TypeId=\"%3\""
                       " CustomTypeId=\"0\""
                       " AutoTime=\"False\""
                       " HighBound=\"0\""
                       " LowBound=\"0\""
                       " Scale=\"0\""
                       " IoAdr=\"%4\""
                       " MapVarName=\"%5\""
                       " MirrorAdr=\"0\""
                       " SelectPeriod=\"%6\""
                       " ExecTimeout=\"%6\""
                   " />").arg(name).arg(description).arg(type_id).arg(addr).arg(variable).arg(select_exec_time);
}


iec104Slave::iec104Slave(const pugi::xml_node& node)
{
    module_name = node.attribute("name").value();
    checked = true;                                     // Потом может меняется

   // Находим первый узел с каналом (ch_node)
   pugi::xml_node ch_node = node.find_node(
        [](pugi::xml_node& node) {
            QString type = QString(node.attribute("type").value());
            return (type.startsWith("localTypes:C_") || type.startsWith("localTypes:M_"));
        }
   );

   pugi::xml_node root_node(0);

   if(ch_node)
       root_node = ch_node.parent();            // Берем его родительский узел, что бы пройтись по всем каналам через root_node

   // Из  <ParameterSection ...> мы заберем три элемента, остальные два (descr, mapping_node) мы заберем из <Parameter ....>
   struct tmp_ch{
       QString name;
       QString addr;
       QString type_id;
   };
    QMap<QString, tmp_ch> channels_map;                 // Карта временных каналов с ключем - именем канала

    // Сначала проходимся по узлам <ParameterSection> и добываем по три поля на каждый канал
   for(pugi::xml_node & ps : root_node.children("ParameterSection"))
   {
       QString name = ps.child("Name").first_child().value();
       pugi::xml_node addr_node = findNodeByAttribute(ps,"name","IecAddr");
       pugi::xml_node type_id_node = findNodeByAttribute(ps,"name","IecType");

       if(!addr_node || !type_id_node || name.endsWith("_Confirmation", Qt::CaseInsensitive))
           continue;

       channels_map[name].name = name;
       channels_map[name].addr = addr_node.first_child().value();
       channels_map[name].type_id = type_id_node.first_child().value();
   }

   // Потом по  <Parameter ...> и сохраняем каждый валидный канал в списки cmd_channels или data_channels
  for(pugi::xml_node & ps : root_node.children("Parameter"))
  {
      QString name = ps.child("Name").first_child().value();
      pugi::xml_node descr = findNodeByName(ps,"Description");
      pugi::xml_node mapping_node = findNodeByName(ps,"Mapping");

      if(!descr || !mapping_node || name.endsWith("_Confirmation", Qt::CaseInsensitive))
          continue;


      if(name.startsWith("C_"))                      // Команды
         cmd_channels.push_back(iec104_channel_cmd(
                                    name,
                                    descr.first_child().value(),
                                    channels_map[name].type_id.toUInt(),
                                    channels_map[name].addr.toUInt(),
                                    mapping_node.first_child().value(),
                                    0)                                          // Время блокировки (потом поменять)
                                );

      if(name.startsWith("M_"))                      // Данные
          data_channels.push_back(iec104_channel_data(
                                     name,
                                     descr.first_child().value(),
                                     channels_map[name].type_id.toUInt(),
                                     channels_map[name].addr.toUInt(),
                                     mapping_node.first_child().value(),
                                     false)                                     //Autitime - потом поменять
                                 );
  }

}
