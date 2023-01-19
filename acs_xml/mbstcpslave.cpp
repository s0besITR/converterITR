#include "acs_xml/mbstcpslave.h"

mbsTCPSlave::mbsTCPSlave(const pugi::xml_node& node)
{
    module_name = node.attribute("name").value();
    checked = true;                                     // Потом может меняется

    // Ищем первый узел, где есть type="localTypes:MODBUS_SLAVE_CHANNEL".
    pugi::xml_node tmp = findNodeByAttribute(node,"type", "localTypes:MODBUS_SLAVE_CHANNEL");
    if (!tmp)
        return;

    pugi::xml_node ch_root_node = tmp.parent();                                 // Нашли родительский узел всех каналов в модуле MBSTCP
    for(pugi::xml_node & ch_node : ch_root_node.children("Parameter"))					// Проходимся по всем узлам Parameter
    {
        QString type_name = ch_node.attribute("type").value();
        if(type_name != "localTypes:MODBUS_SLAVE_CHANNEL")
            continue;

        // Запоминаем нужную информацию типа начального адреса, кол-ва каналов
        QString name = ch_node.child("Name").first_child().value();
        int address = QString(ch_node.child("Value").find_child_by_attribute("name", "ModBusAddress").first_child().value()).toInt();
        int count = QString(ch_node.child("Value").find_child_by_attribute("name", "Count").first_child().value()).toInt();
        long int start_id = QString(ch_node.child("Value").find_child_by_attribute("name", "SignalsStartID").first_child().value()).toLong();
        int mbs_seg = QString(ch_node.child("Value").find_child_by_attribute("name", "ModBusSegment").first_child().value()).toInt();
        int length = 1;
        QString dType = ch_node.child("Value").find_child_by_attribute("name", "DType").first_child().value();
        dType.replace("'", "");
        if (dType == "DINT" || dType == "DWORD" || dType == "UDINT" || dType == "REAL")
            length = 2;

        // Проходимся по всем каналам, которые начинаются с start_id и меньше start_id + count
        for (pugi::xml_node ch : ch_root_node.children("Parameter"))
        {
            int id = QString(ch.attribute("ParameterId").value()).toInt();
            if (id >= start_id  && id < (start_id + count))
            {
                QString tmp_type;
                switch (mbs_seg)
                {
                    case 3 : tmp_type = "HoldingRegisters";	// Регистры хранения
                        break;
                    case 2: tmp_type = "InputRegisters";	// регистры ввода
                        break;
                    case 1: tmp_type = "DiscreteInputs";	// дискретные входы
                        break;
                    case 0: tmp_type = "Coils";				// регистры флагов
                        break;
                    default:
                        tmp_type = QString("Undefined segment %1").arg(mbs_seg);
                        break;
                }

                modbus_tcp_channel ch_data(
                            ch.child("Mapping").first_child().value(),
                            QString(ch.child("Description").first_child().value()).replace("©",""),
                            ch.child("Name").first_child().value(),
                            name,
                            tmp_type,
                            address + (id - start_id)*length,
                            length,
                            channels.length() + 1
                            );
                channels.push_back(ch_data);
            }
        }
    }
}



modbus_tcp_channel::modbus_tcp_channel(QString _var, QString _descr, QString _ch_name, QString _group_name, QString _type, int _offset, int _len, int _ind)
    : VarName(_var), Descr(_descr), ChannelName(_ch_name), GroupName(_group_name), Type(_type), Offset(_offset), Length(_len), index(_ind)
{}

 QString modbus_tcp_channel::getType()
 {
    if (VarName.contains("_BOOL_"))
        return "WORD";
    if (VarName.contains("_WORD_"))
        return "WORD";
    if (VarName.contains("_DWORD_"))
        return "DWORD";
     if (VarName.contains("_INT_"))
        return "INT";
    if (VarName.contains("_DINT_"))
        return "DINT";
    if (VarName.contains("_UINT_"))
        return "UINT";
    if (VarName.contains("_UDINT_"))
        return "UDINT";
    if (VarName.contains("_REAL_"))
        return "REAL";

    return "WORD";
 }

 QString modbus_tcp_channel::toString()
 {
     return QString("<DirectSlaveCh"
                    " Name=\"%1\""
                    " Descr=\"%2\""
                    " Type=\"%3\""
                    " Offset=\"%4\""
                    " Length=\"%5\""
                    " VarName=\"%6\""
                    "/>").arg(QString("Channel_%1").arg(index)).arg(Descr).arg(Type).arg(Offset).arg(Length).arg(VarName);
 }

 // Среди всех каналов вычисляет последний адрес, необходимый для размещения их в памяти в массиве [0 .. getLastByteAddress()]
 int mbsTCPSlave::getLastByteAddress()
 {
     int max = 0;
     for (size_t i = 0; i < channels.size(); ++i)
         if (channels.at(i).Offset + channels.at(i).Length - 1 > max)
             max = channels.at(i).Offset + channels.at(i).Length - 1;
     return max;
 }
