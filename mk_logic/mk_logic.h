#pragma once

#include "excel/excel.h"
#include "acs_xml/elesy_xml.h"
#include <QTextStream>

class mk_logic
{
public:
    mk_logic();
};

// Получить первое число из строчки
int getNum(QString& l);
// Заменяем то, что в [скобках] на num
QString replaceBrackets (const QString &l, int num);


//void isagraf_xlsx_edit(xlnt::workbook & wb, QString filename);
void isagraf_csv_export(QString app_filename, QString csv_name);
void isagraf_ST_export(QString filename, QString folder, bool split, int max_lines);
void modifyIsagrafData(pugi::xml_node body_node, QStringList & dataList, bool split, int max_lines);
QString parse_indexIEC(QString & line, QTextStream & stream);
QString parse_FOR_WHILE(QString & line, QString sfb_name);
QString parse_FB(QString & line, QTextStream & stream, QString sfb_name);
void parseFbArguments(QString line, QVector<std::tuple<QString, QString, QString>> & args, QString index_name);
QString getArgumentsString(QVector<std::tuple<QString, QString, QString>> & args, QString index_name);
void splitMaxLines(const QString & input, QStringList & list, int max);
