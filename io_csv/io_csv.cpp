#include "io_csv.h"

QMap<unsigned int,QVector<io_csv>> IO_info_map;                     // Объект будет храниться здесь глобальный

QMap<unsigned int, QString> iec_types{
    {1, QString("M_SP_NA_1")},
    {2, QString("M_SP_TA_1")},
    {3, QString("M_DP_NA_1")},
    {4, QString("M_DP_TA_1")},
    {5, QString("M_ST_NA_1")},
    {6, QString("M_ST_TA_1")},
    {7, QString("M_BO_NA_1")},
    {8, QString("M_BO_TA_1")},
    {9, QString("M_ME_NA_1")},
    {10, QString("M_ME_TA_1")},
    {11, QString("M_ME_NB_1")},
    {12, QString("M_ME_TB_1")},
    {13, QString("M_ME_NC_1")},
    {14, QString("M_ME_TC_1")},
    {15, QString("M_IT_NA_1")},
    {16, QString("M_IT_TA_1")},
    {17, QString("M_EP_TA_1")},
    {18, QString("M_EP_TB_1")},
    {19, QString("M_EP_TC_1")},
    {21, QString("M_ME_ND_1")},
    {30, QString("M_SP_TB_1")},
    {31, QString("M_DP_TB_1")},
    {32, QString("M_ST_TB_1")},
    {33, QString("M_BO_TB_1")},
    {34, QString("M_ME_TD_1")},
    {35, QString("M_ME_TE_1")},
    {36, QString("M_ME_TF_1")},
    {37, QString("M_IT_TB_1")},
    {38, QString("M_EP_TD_1")},
    {39, QString("M_EP_TE_1")},
    {40, QString("M_EP_TF_1")},
    {45, QString("C_SC_NA_1")},
    {46, QString("C_DC_NA_1")},
    {47, QString("C_RC_NA_1")},
    {48, QString("C_SE_NA_1")},
    {49, QString("C_SE_NB_1")},
    {50, QString("C_SE_NC_1")},
    {51, QString("C_BO_NA_1")},
    {58, QString("C_SC_TA_1")},
    {59, QString("C_DC_TA_1")},
    {60, QString("C_RC_TA_1")},
    {61, QString("C_SE_TA_1")},
    {62, QString("C_SE_TB_1")},
    {63, QString("C_SE_TC_1")},
    {64, QString("C_BO_TA_1")},
};

io_csv::io_csv(QString _station, QString _protocol_type, QString _address, QString _description, QString _tag)
{
    station = _station.toUInt();
    protocol_type_int = _protocol_type.toUInt();
    address = _address.toUInt();
    description = _description;
    protocol_type_str = "unknown";
    dubl_count = 0;
    tag = _tag;

    if(iec_types.contains(protocol_type_int))
        protocol_type_str = iec_types[protocol_type_int];
}


bool io_csv::operator<(const io_csv& other) const
{
    QStringList myList = splitNumbers(this->tag);
    QStringList otherList =  splitNumbers(other.tag);

    for(int i = 0; i < qMin(myList.length(), otherList.length()); ++i)
    {
       QString my_str = myList.at(i);
       QString other_str = otherList.at(i);

       if(my_str == other_str)
          continue;

       if(my_str.at(0).isDigit() && other_str.at(0).isDigit())
          return (my_str.toInt() < other_str.toInt());
       else
           return (my_str < other_str);
    }
    return myList.length() < otherList.length();
}

QStringList io_csv::splitNumbers(const QString& str) const
   {
       QStringList lst;
       QString tmp = "";
       bool dgt = false;

       for(QString::const_iterator it = str.cbegin(); it != str.cend(); ++it)
       {
           if(dgt != (*it).isDigit())
           {
               if(!tmp.isEmpty())
               {
                   lst.push_back(tmp);
                   tmp.clear();
               }
               dgt = (*it).isDigit();
           }
           tmp += *it;
       }
       if(!tmp.isEmpty())
           lst.push_back(tmp);
       return lst;
   };

bool io_csv::is_Rezerv()
{
    return (description.endsWith(" Резерв") || description == "Резерв");
}

bool io_csv::is_Empty()
{
    return description.isEmpty();
}

int getRezervSize(QVector<io_csv> & v)
{
    int size = 0;
    for(auto & el : v)
        size += el.is_Rezerv();
    return size;
}

int getEmptySize(QVector<io_csv> & v)
{
    int size = 0;
    for(auto & el : v)
        size += el.is_Empty();
    return size;
}

int getNumberofDublicates(QVector<io_csv> & v)
{
    int size = 0;
    for(auto & el : v)
        size += (el.dubl_count > 1);
    return size;
}

void apply_style(xlnt::cell c, bool border_flag, bool bold_flag, xlnt::horizontal_alignment h_al, xlnt::vertical_alignment w_al)
{
    xlnt::alignment align;
    xlnt::border border;
    xlnt::font font;

    align.horizontal(h_al);
    align.vertical(w_al);

    if(border_flag)
        for (xlnt::border_side side : border.all_sides())
            border.side(side, xlnt::border::border_property().style(xlnt::border_style::thin));
    font.bold(bold_flag);

    c.alignment(align);
    c.border(border);
    c.font(font);
}

void parse_line_complex(QString & str)
{
    QString prop;
    static QString description;
    static QString IEC_address = "";
    static QString IEC_address_type = "";
    static QString tag;

    QStringList attributes = str.split(",");
    if(attributes.length() < 2)
        return;
    prop = attributes.at(1);

    if (tag != attributes.at(0))
        description = "";
    tag = attributes.at(0);

    if(prop == "101")
    {
        for(int i = 0, size =attributes.size() ; i < size && i !=3; ++i)
            attributes.removeFirst();
        description = attributes.join(",");
    }
    else if(prop == "5564")
        IEC_address = QString::number(attributes.last().replace("0x","").toUInt(nullptr, 16));
    else if(prop == "5566")
        IEC_address_type = attributes.last().replace("(","").replace(")","");
    else if(prop == "5567")
    {
        io_csv tmp_info(attributes.last(),
                      IEC_address_type.split(" ").first(),
                      IEC_address,
                      description,
                      attributes.first());

        IEC_address = "";
        IEC_address_type = "";
        description = "";

        if(tmp_info.protocol_type_int == 0 || tmp_info.address == 0)         // Если не заполнен протокол или адрес, не включаем его в рассчет
            return;
        if(tmp_info.protocol_type_int == 37 && tmp_info.description.toLower().endsWith(" отсутствие связи")) // ПГЕшные отсутствия свзи M_IT_TB_1
            return;

        IO_info_map[tmp_info.station].push_back(tmp_info);
    }
}

void parse_line_native(QString & str)
{
    QString prop;
    static QString description;
    static QString tag;

    QStringList attributes = str.split(",");
    if(attributes.length() < 2)
        return;
    prop = attributes.at(1);

    if (tag != attributes.at(0))
        description = "";
    tag = attributes.at(0);

    if(prop == "101")
    {
        for(int i = 0, size =attributes.size() ; i < size && i !=3; ++i)
            attributes.removeFirst();
        description = attributes.join(",");
    }
    else if(prop == "7050")
    {
        QDomDocument doc;
        QDomElement element;
        doc.setContent(attributes.last());
        element = doc.documentElement();
        io_csv tmp_info(element.attribute("Station", "0"),
                      element.attribute("ProtocolType","0"),
                      element.attribute("Address", "0"),
                      description,
                      attributes.first());

        if(tmp_info.protocol_type_int == 0 || tmp_info.address == 0)         // Если не заполнен протокол или адрес, не включаем его в рассчет
            return;
        if(tmp_info.protocol_type_int == 37 && tmp_info.description.toLower().endsWith(" отсутствие связи")) // ПГЕшные отсутствия свзи M_IT_TB_1
            return;

        IO_info_map[tmp_info.station].push_back(tmp_info);
        description = "";
    }
}

