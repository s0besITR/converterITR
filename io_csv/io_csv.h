#pragma once

#include <QMap>
#include <QVector>
#include <QString>
#include <QtXml>
#include <xlnt/xlnt.hpp>

class io_csv
{
public:
    unsigned int    station;
    unsigned int    protocol_type_int;
    unsigned int    address;
    QString         protocol_type_str;
    QString         description;
    int             dubl_count;
    QString tag;

    io_csv(QString _station, QString _protocol_type, QString _address, QString _description, QString _tag);
    bool operator<(const io_csv& other) const;
    bool is_Rezerv();
    bool is_Empty();
    QStringList splitNumbers(const QString& str) const;
};


int getRezervSize(QVector<io_csv> & v);
int getEmptySize(QVector<io_csv> & v);
int getNumberofDublicates(QVector<io_csv> & v);
void apply_style(xlnt::cell c,
                 bool border_flag = false,
                 bool bold_flag = false,
                 xlnt::horizontal_alignment h_al = xlnt::horizontal_alignment::left,
                 xlnt::vertical_alignment w_al = xlnt::vertical_alignment::center);
void parse_line_complex(QString & str);
void parse_line_native(QString & str);


