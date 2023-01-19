#include "elesy_csv.h"

mbs::mbs(QString _tag,QString hex_val) : tag(_tag), seg("unknown"), bit(-1)
{
    hex_val.replace("$","");
    QByteArray data = QByteArray::fromHex(hex_val.toUtf8());

    if(data.size() < 8 || data.at(7) != 6 || data.at(1) == 0)  //(6 - протокол Modbus TCP)
        return;

    int type    = data.at(6) & 0b00001111;                         //ТС = 0, ТУ = 1,  только у этих двух типов может быть указан номер бита
    addr        = ((unsigned char) data.at(2) << 8) | (unsigned char) data.at(3); // адрес
    station     = (unsigned char) data.at(1);                      // номер станции
    int seg_type= ((unsigned char) data.at(0) & 0b00110000) >> 4;  //тип сегмента (0 - output coil, 1 - holding coil, 2 - output register, 3 - holding register)

    QStringList segments = {"1x", "0x", "3x" ,"4x"};
    //QStringList descriptions_elesy = {"Output Coil", "Holding Coil", "Output Register" ,"Holding Register"};
    QStringList descriptions = {"Discretes Input", "Coils", "Input Registers" ,"Holding Registers"};
    seg = segments[seg_type];
    seg_descr = descriptions[seg_type];
    if((type == 0 || type == 1) && (seg_type == 2 || seg_type == 3))                      //для ТС и ТУ биты 0-3 - номер бита в байте
        bit = data.at(0) & 0b00001111;
}

QString mbs::to_string()
{
    if(bit == -1)
        return QString("%1 \t %2%3 \t\t %4").arg(station).arg(seg).arg(addr).arg(tag);
    else
        return QString("%1 \t %2%3.%4 \t\t %5").arg(station).arg(seg).arg(addr).arg(bit).arg(tag);
}

void elesy_csv::parse_line(QString str)
{
    QStringList attributes = str.split(",");
    if(attributes.length() < 2)
        return;

    QString prop = attributes.at(1);

    // modbus атрибуты
    if(prop == "7014")
    {

        mbs tmp_info(attributes.first(), attributes.last());
        if( tmp_info.seg == "unknown")
            return;
        modbus_map[tmp_info.station].push_back(tmp_info);
      //  qDebug().noquote() << tmp_info.to_string();
    }

    // IEC атрибуты
    if(prop == "7050")
    {
        QDomDocument doc;
        QDomElement element;
        doc.setContent(attributes.last());
        element = doc.documentElement();
        iec104 tmp(element.attribute("Station", "0"),
                      element.attribute("ProtocolType","0"),
                      element.attribute("Address", "0"),
                   attributes.first());
        iec_map[tmp.station].push_back(tmp);
    }
}

elesy_csv::elesy_csv(QStringList csv_files)
{
    for(auto f : csv_files)
    {
        QFile file(f);
        if(!file.open(QIODevice::ReadOnly))
            continue;
        QTextStream in(&file);
        while (!in.atEnd())
            parse_line(in.readLine());
        file.close();
    }
}

void elesy_csv::save_modbus(QString dir)
{
    for(auto &map : modbus_map)
    {
         QFile file(dir + QString("\\Modbus_TCP_USO%1.xml").arg(map.first().station));
         if(!file.open(QIODevice::WriteOnly))
             continue;
         QTextStream outfile(&file);

         outfile << "<root format-version=\"1\">" << "\n";
         for (auto & node : map)
         {
             outfile << "\t" << "<item Binding=\"Introduced\">" << "\n";
             outfile << "\t\t" << "<node-path>" << node.tag << "</node-path>" << "\n";
             outfile << "\t\t" << "<table>" << node.seg_descr << "</table>" << "\n";
             outfile << "\t\t" << "<address>" << node.addr << "</address>" << "\n";
             if (node.bit != -1)
                 outfile << "\t\t" << "<bitposition>" << node.bit << "</bitposition>" << "\n";
             outfile << "\t" << "</item>" << "\n";
         }
         outfile << "</root>" << "\n";
         file.close();
    }
}
void elesy_csv::save_iec(QString dir)
{
     for(auto &map : iec_map)
     {
         QFile file(dir + QString("\\IEC104_USO%1.xml").arg(map.first().station));
         if(!file.open(QIODevice::WriteOnly))
             continue;
         QTextStream outfile(&file);

         outfile << "<root format-version=\"0\">" << "\n";
         for (auto & node : map)
         {
             outfile << "\t" << "<item Binding=\"Introduced\">" << "\n";
             outfile << "\t\t" << "<node-path>" << node.tag << "</node-path>" << "\n";
             outfile << "\t\t" << "<address>" << node.address << "</address>" << "\n";
             outfile << "\t\t" << "<protocoltype>" << node.protocol_type_int << "</protocoltype>" << "\n";
             outfile << "\t" << "</item>" << "\n";
         }
         outfile << "</root>" << "\n";
         file.close();
     }
}


iec104::iec104(QString _station, QString _protocol_type, QString _address, QString _tag)
    :station(_station.toUInt()), protocol_type_int (_protocol_type.toUInt()), address(_address.toUInt()), tag(_tag)
{}

bool elesy_csv::mbs_empty()
{
    return this->modbus_map.empty();
}
bool elesy_csv::iec_empty()
{
    return this->iec_map.empty();
}

void getSokol_Nodes(pugi::xml_document &doc, QVector<sokol_node> &nodes)
{
    for (pugi::xml_node tool : doc.child("blocks").child("chart").children())
        for (pugi::xml_node tool2 : tool.child("table").child("parameters").children())
        {
            std::string data = tool2.attribute("description").value();	// Забираем текст из description, там содержится тег и адрес типа "ТЭГ [4x15.1]"
            sokol_node dev = { "","",-1,-1 };							// Инициализируем структуру типа {тег, сегмент, адрес, бит}

            size_t pos_br_start = data.find("[");
            size_t pos_br_end = data.find("]");
            if (pos_br_start == std::string::npos || pos_br_end == std::string::npos)
                continue;

            std::string adr_data = data.substr(pos_br_start + 1, pos_br_end - pos_br_start - 1);		// получаем содержимое в скобках []
            size_t pos_x = adr_data.find("x");													// позиция "x", что бы выделить сегмент
            size_t pos_dot = adr_data.find(".");												// позиция ".", что бы выделить бит

            dev.tag = data.substr(0, pos_br_start - 1).c_str();											// Получили тег
            int segm = stoi(adr_data.substr(0, pos_x));											// Получили сегмент
            switch (segm)
            {
                case 4:  dev.segment = "Holding Registers";
                    break;
                case 3:  dev.segment = "Input Registers";
                default:
                    dev.segment = "HZ KAKOI REGISTERS. DODELAT V BUDUSHEM";
                    break;
            }

            if (pos_dot != std::string::npos)													// Если есть символ точки, то получаем бит
                dev.bit = stoi(adr_data.substr(pos_dot + 1, adr_data.length() - pos_dot - 1));
            dev.adr = stoi(adr_data.substr(pos_x + 1, pos_dot - pos_x - 1)) - 1;				// Получаем адрес

            nodes.push_back(dev);																// Сохраняем структуру типа {тег, сегмент, адрес, бит} в массив
        }
}
void exportSokol_Map(QVector<sokol_node> &nodes, QString path)
{
    QFile file(path.replace("Теги","MAP"));
    if(!file.open(QIODevice::WriteOnly))
        return;
    QTextStream outfile(&file);

    outfile << "<root format-version=\"1\">" << "\n";
    for (auto & node : nodes)
    {
        outfile << "\t" << "<item Binding=\"Introduced\">" << "\n";
        outfile << "\t\t" << "<node-path>" << node.tag << "</node-path>" << "\n";
        outfile << "\t\t" << "<table>" << node.segment << "</table>" << "\n";
        outfile << "\t\t" << "<address>" << node.adr << "</address>" << "\n";
        if (node.bit != -1)
            outfile << "\t\t" << "<bitposition>" << node.bit << "</bitposition>" << "\n";
        outfile << "\t" << "</item>" << "\n";
     }
     outfile << "</root>" << "\n";
     file.close();
}


template_modbus::template_modbus() : type(0), oven_dummy(false) {}

bool template_modbus::operator<(const template_modbus& other)
{
    return this->mapping < other.mapping;
}

bool template_modbus::isValid()
{
    if(this->mapping.length()==0 || this->dev_template.length()==0 || this->station.length()==0)
        return false;

    if(this->type == 0) //rtu
    {
        return (this->modbus_module.length()>0 && this->modbus_channel.length()>0);
    }
    else    //tcp
    {
        return this->ip.length()>0;
    }

}

void parseLineTemplates(QString s, QMap<QString,template_modbus_5ch> & template_map)
{
    int prop;
    QString tag;

    QStringList attributes = s.split(",");
    if(attributes.length() < 2)
        return;
    tag = attributes.at(0);
    prop = attributes.at(1).toInt();


    switch(prop){
        case 8814:{
            template_map[tag].m1.type = template_map[tag].m2.type = template_map[tag].m3.type = template_map[tag].m4.type = template_map[tag].m5.type = attributes.last().toInt();
            template_map[tag].m1.csv_name = template_map[tag].m2.csv_name = template_map[tag].m3.csv_name = template_map[tag].m4.csv_name = template_map[tag].m5.csv_name = tag.split(".").first();
        break;}
        // КП
        case 8888:{ template_map[tag].m1.station = attributes.last().replace(" ","");         break;}
        case 8816:{ template_map[tag].m2.station = attributes.last().replace(" ","");         break;}
        case 8827:{ template_map[tag].m3.station = attributes.last().replace(" ","");         break;}
        case 8833:{ template_map[tag].m4.station = attributes.last().replace(" ","");         break;}
        case 8911:{ template_map[tag].m5.station = attributes.last().replace(" ","");         break;}
        // Модуль
        case 8810:{ template_map[tag].m1.modbus_module = attributes.last().replace(" ","");         break;}
        case 8817:{ template_map[tag].m2.modbus_module = attributes.last().replace(" ","");         break;}
        case 8828:{ template_map[tag].m3.modbus_module = attributes.last().replace(" ","");         break;}
        case 8834:{ template_map[tag].m4.modbus_module = attributes.last().replace(" ","");         break;}
        case 8912:{ template_map[tag].m5.modbus_module = attributes.last().replace(" ","");         break;}
        // Канал
        case 8811:{ template_map[tag].m1.modbus_channel = attributes.last().replace(" ","");         break;}
        case 8818:{ template_map[tag].m2.modbus_channel = attributes.last().replace(" ","");         break;}
        case 8829:{ template_map[tag].m3.modbus_channel = attributes.last().replace(" ","");         break;}
        case 8835:{ template_map[tag].m4.modbus_channel = attributes.last().replace(" ","");         break;}
        case 8913:{ template_map[tag].m5.modbus_channel = attributes.last().replace(" ","");         break;}
        // Маппинг
        case 8812:{ template_map[tag].m1.mapping = attributes.last().replace(" ","");         break;}
        case 8819:{ template_map[tag].m2.mapping = attributes.last().replace(" ","");         break;}
        case 8830:{ template_map[tag].m3.mapping = attributes.last().replace(" ","");         break;}
        case 8836:{ template_map[tag].m4.mapping = attributes.last().replace(" ","");         break;}
        case 8914:{ template_map[tag].m5.mapping = attributes.last().replace(" ","");         break;}
        // IP
        case 8813:{ template_map[tag].m1.ip = attributes.last().replace(" ","");         break;}
        case 8820:{ template_map[tag].m2.ip = attributes.last().replace(" ","");         break;}
        case 8831:{ template_map[tag].m3.ip = attributes.last().replace(" ","");         break;}
        case 8837:{ template_map[tag].m4.ip = attributes.last().replace(" ","");         break;}
        case 8915:{ template_map[tag].m5.ip = attributes.last().replace(" ","");         break;}
        // Шаблон
        case 8821:{ template_map[tag].m1.dev_template = attributes.last().replace(" ","");         break;}
        case 8822:{ template_map[tag].m2.dev_template = attributes.last().replace(" ","");         break;}
        case 8832:{ template_map[tag].m3.dev_template = attributes.last().replace(" ","");         break;}
        case 8838:{ template_map[tag].m4.dev_template = attributes.last().replace(" ","");         break;}
        case 8916:{ template_map[tag].m5.dev_template = attributes.last().replace(" ","");         break;}
        //Oven dummy
        case 89101: {
                    bool oven_dymmy = attributes.last().replace(" ","") == "1" ? true : false;
                    template_map[tag].m1.oven_dummy = oven_dymmy;
                    template_map[tag].m2.oven_dummy = oven_dymmy;
                    template_map[tag].m3.oven_dummy = oven_dymmy;
                    template_map[tag].m4.oven_dummy = oven_dymmy;
                    template_map[tag].m5.oven_dummy = oven_dymmy;
                    break;
                    }
    }
}
