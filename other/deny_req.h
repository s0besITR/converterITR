#pragma once
#include "pugixml/pugixml.hpp"
#include <QString>
#include <QStringList>
#include <QVector>

using namespace pugi;

class deny_req
{
public:
    deny_req();
};

void find_tn713_modules(pugi::xml_document &doc, QVector<QStringList> & changed_items);
void modify_params(pugi::xml_node& connector_713, QStringList & modified_list);
