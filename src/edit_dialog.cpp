#include "edit_dialog.hpp"
#include <vector>
#include <string>
#include <sstream>

EditDialog::EditDialog(Gtk::Window& parent, const std::string& title, bool is_group)
    : m_NameLabel("Name:"), m_DescriptionLabel("Description:"), m_IconLabel("Icon:"),
      m_CommandLabel("Command:"), m_KeywordsLabel("Keywords:"), m_TypeLabel("Type:") {
    set_title(title);
    set_transient_for(parent);
    set_modal(true);
    add_button("_Cancel", Gtk::RESPONSE_CANCEL);
    add_button("_OK", Gtk::RESPONSE_OK);

    get_content_area()->add(m_Grid);
    m_Grid.set_border_width(10);
    m_Grid.set_row_spacing(10);
    m_Grid.set_column_spacing(10);

    m_Grid.attach(m_NameLabel, 0, 0, 1, 1);
    m_Grid.attach(m_NameEntry, 1, 0, 1, 1);
    m_Grid.attach(m_DescriptionLabel, 0, 1, 1, 1);
    m_Grid.attach(m_DescriptionEntry, 1, 1, 1, 1);
    m_Grid.attach(m_IconLabel, 0, 2, 1, 1);
    m_Grid.attach(m_IconEntry, 1, 2, 1, 1);

    if (!is_group) {
        m_Grid.attach(m_CommandLabel, 0, 3, 1, 1);
        m_Grid.attach(m_CommandEntry, 1, 3, 1, 1);
        m_Grid.attach(m_KeywordsLabel, 0, 4, 1, 1);
        m_Grid.attach(m_KeywordsEntry, 1, 4, 1, 1);
        m_Grid.attach(m_TypeLabel, 0, 5, 1, 1);
        m_Grid.attach(m_TypeComboBox, 1, 5, 1, 1);

        m_TypeComboBox.append("command", "Command");
        m_TypeComboBox.append("terminal_command", "Terminal Command");
        m_TypeComboBox.append("url", "URL");
        m_TypeComboBox.append("application", "Application");
    }

    show_all();
}

void EditDialog::set_group_data(const PrimeCuts::Group& group) {
    m_NameEntry.set_text(group.name);
    m_DescriptionEntry.set_text(group.description);
    m_IconEntry.set_text(group.icon);
}

void EditDialog::get_group_data(PrimeCuts::Group& group) {
    group.name = m_NameEntry.get_text();
    group.description = m_DescriptionEntry.get_text();
    group.icon = m_IconEntry.get_text();
}

void EditDialog::set_action_data(const PrimeCuts::Action& action) {
    m_NameEntry.set_text(action.name);
    m_DescriptionEntry.set_text(action.description);
    m_IconEntry.set_text(action.icon);
    m_CommandEntry.set_text(action.command);

    std::string keywords;
    for (size_t i = 0; i < action.keywords.size(); ++i) {
        keywords += action.keywords[i];
        if (i < action.keywords.size() - 1) {
            keywords += ",";
        }
    }
    m_KeywordsEntry.set_text(keywords);

    switch (action.type) {
        case PrimeCuts::ActionType::COMMAND: m_TypeComboBox.set_active_id("command"); break;
        case PrimeCuts::ActionType::TERMINAL_COMMAND: m_TypeComboBox.set_active_id("terminal_command"); break;
        case PrimeCuts::ActionType::URL: m_TypeComboBox.set_active_id("url"); break;
        case PrimeCuts::ActionType::APPLICATION: m_TypeComboBox.set_active_id("application"); break;
    }
}

void EditDialog::get_action_data(PrimeCuts::Action& action) {
    action.name = m_NameEntry.get_text();
    action.description = m_DescriptionEntry.get_text();
    action.icon = m_IconEntry.get_text();
    action.command = m_CommandEntry.get_text();

    action.keywords.clear();
    std::stringstream ss(m_KeywordsEntry.get_text());
    std::string keyword;
    while (std::getline(ss, keyword, ',')) {
        action.keywords.push_back(keyword);
    }

    std::string type = m_TypeComboBox.get_active_id();
    if (type == "command") action.type = PrimeCuts::ActionType::COMMAND;
    else if (type == "terminal_command") action.type = PrimeCuts::ActionType::TERMINAL_COMMAND;
    else if (type == "url") action.type = PrimeCuts::ActionType::URL;
    else if (type == "application") action.type = PrimeCuts::ActionType::APPLICATION;
}
