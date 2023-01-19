#include "deny_req.h"


deny_req::deny_req()
{

}

void find_tn713_modules(xml_document &doc, QVector<QStringList> & changed_items)
{
    // Предикат для поиска узла у которого Имя модуля = N713
    auto TN713_predicate = [](xml_node& node) {
        if (QString(node.attribute("visiblename").value()) == "ModName")
        {
            QString module_name = QString(node.first_child().value());
                if (module_name == "'N713'" || module_name == "'N723'")
                    return true;
        }
        return false;
    };

    QString module_name = "";
    // Предикат для поиска узла с именем Connector, который является модулем tn713
    auto Connector_Predicate = [&TN713_predicate, &module_name, &changed_items](xml_node& node) {
        if (QString(node.name()) == "configuration")
            module_name = node.attribute("name").value();

        if (QString(node.name()) == "Connector")
            if (node.find_node(TN713_predicate))
            {
                QStringList tmp_list;
                tmp_list << module_name;
                modify_params(node,tmp_list);
                changed_items.push_back(tmp_list);
            }

        return false;
    };

    doc.find_node(Connector_Predicate);
}

void modify_params(xml_node& connector_713, QStringList & modified_list)
{
    xml_node host_params = connector_713.first_child();
    for (xml_node tool : host_params.children("ParameterSection"))
    {
        if (QString(tool.child("Name").first_child().value()) == "Extended Module Configuration")
        {
            for (xml_node tool_ext : tool.children("Parameter"))
            {
                QString param_name = QString(tool_ext.child_value("Name"));
                QString value = QString(tool_ext.child_value("Value"));
                size_t mode_pos = param_name.indexOf("_ItemMode", 0);
                if (mode_pos != -1 && value == "0")
                {
                    tool_ext.child("Value").first_child().set_value("1");
                    modified_list << param_name.replace(mode_pos, 9, "");
                }
            }           
        }

        if (QString(tool.child("Name").first_child().value()) == "Channels")
        {
            for (xml_node tool_ps : tool.children("ParameterSection"))
                for (xml_node tool_p : tool_ps.children("Parameter"))
                {
                    QString param_name, ChannelType, ReqMode = "";
                    xml_node req_node;
                    for (xml_node info = tool_p.first_child(); info; info = info.next_sibling())
                    {
                        if (QString(info.name()) == "Name")	param_name = QString(info.first_child().value());
                        if (QString(info.name()) == "Value")
                            for (xml_node elem : info.children("Element"))
                            {
                                if (QString(elem.attribute("name").value()) == QString("ChannelType"))
                                    ChannelType = elem.first_child().value();
                                if (QString(elem.attribute("name").value()) == QString("ReqMode"))
                                {
                                    ReqMode = elem.first_child().value();
                                    req_node = elem;
                                }
                            }
                    }

                    if (ChannelType == "1" && ReqMode == "0")                       
                        req_node.first_child().set_value("1");

                }
        }
    }
}
