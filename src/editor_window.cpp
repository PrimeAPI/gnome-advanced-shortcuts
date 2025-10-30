#include "editor_window.hpp"
#include <iostream>
#include <signal.h>
#include <stdlib.h>
#include <algorithm>
#include <sys/stat.h>

// Function to check if a file exists
inline bool file_exists(const std::string& name) {
    struct stat buffer;
    return (stat(name.c_str(), &buffer) == 0);
}

EditorWindow::EditorWindow()
    : m_VBox(Gtk::ORIENTATION_VERTICAL),
      m_ButtonBox(Gtk::ORIENTATION_HORIZONTAL),
      m_AddGroupButton("Add Group"),
      m_AddActionButton("Add Action"),
      m_EditButton("Edit"),
      m_DeleteButton("Delete"),
      m_MoveUpButton("Up"),
      m_MoveDownButton("Down"),
      m_SaveButton("Save and Apply"),
      m_QuitButton("Quit") {
    set_title("PrimeCuts Editor");
    set_default_size(600, 400);

    // TreeView setup
    m_refTreeModel = Gtk::ListStore::create(m_Columns);
    m_TreeView.set_model(m_refTreeModel);
    m_TreeView.append_column("Configuration", m_Columns.m_col_text);

    m_ScrolledWindow.add(m_TreeView);
    m_ScrolledWindow.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    m_VBox.pack_start(m_ScrolledWindow);

    // Button box setup
    m_ButtonBox.set_spacing(5);
    m_ButtonBox.pack_start(m_AddGroupButton, Gtk::PACK_SHRINK);
    m_ButtonBox.pack_start(m_AddActionButton, Gtk::PACK_SHRINK);
    m_ButtonBox.pack_start(m_EditButton, Gtk::PACK_SHRINK);
    m_ButtonBox.pack_start(m_DeleteButton, Gtk::PACK_SHRINK);
    m_ButtonBox.pack_start(m_MoveUpButton, Gtk::PACK_SHRINK);
    m_ButtonBox.pack_start(m_MoveDownButton, Gtk::PACK_SHRINK);
    m_VBox.pack_start(m_ButtonBox, Gtk::PACK_SHRINK);

    // Main action buttons
    m_VBox.pack_start(m_SaveButton, Gtk::PACK_SHRINK);
    m_VBox.pack_start(m_QuitButton, Gtk::PACK_SHRINK);

    add(m_VBox);

    // Connect signals
    m_QuitButton.signal_clicked().connect(sigc::mem_fun(*this, &EditorWindow::on_button_quit));
    m_SaveButton.signal_clicked().connect(sigc::mem_fun(*this, &EditorWindow::on_button_save));
    m_AddGroupButton.signal_clicked().connect(sigc::mem_fun(*this, &EditorWindow::on_button_add_group));
    m_AddActionButton.signal_clicked().connect(sigc::mem_fun(*this, &EditorWindow::on_button_add_action));
    m_EditButton.signal_clicked().connect(sigc::mem_fun(*this, &EditorWindow::on_button_edit));
    m_DeleteButton.signal_clicked().connect(sigc::mem_fun(*this, &EditorWindow::on_button_delete));
    m_MoveUpButton.signal_clicked().connect(sigc::mem_fun(*this, &EditorWindow::on_button_move_up));
    m_MoveDownButton.signal_clicked().connect(sigc::mem_fun(*this, &EditorWindow::on_button_move_down));

    show_all_children();

    // Load initial config
    load_config();
    populate_treeview();
}

EditorWindow::~EditorWindow() {}

void EditorWindow::on_button_quit() {
    hide();
}

void EditorWindow::on_button_save() {
    std::string config_path = Glib::get_user_config_dir() + "/primecuts/config.json";
    if (m_configLoader.saveToFile(config_path, m_config)) {
        std::cout << "Configuration saved successfully." << std::endl;
        // Restart the primecuts service
        system("killall primecuts");
        system("primecuts &");
    } else {
        std::cerr << "Failed to save configuration." << std::endl;
    }
}

void EditorWindow::on_button_add_group() {
    EditDialog dialog(*this, "Add Group", true);
    PrimeCuts::Group new_group;
    if (dialog.run() == Gtk::RESPONSE_OK) {
        dialog.get_group_data(new_group);
        m_config.groups.push_back(new_group);
        populate_treeview();
    }
}

void EditorWindow::on_button_add_action() {
    Glib::RefPtr<Gtk::TreeSelection> selection = m_TreeView.get_selection();
    Gtk::TreeModel::iterator iter = selection->get_selected();
    if (!iter) return;

    bool is_group = (*iter)[m_Columns.m_col_is_group];
    int group_index = (*iter)[m_Columns.m_col_group_iter];

    if (is_group) {
        EditDialog dialog(*this, "Add Action", false);
        PrimeCuts::Action new_action;
        if (dialog.run() == Gtk::RESPONSE_OK) {
            dialog.get_action_data(new_action);
            m_config.groups[group_index].actions.push_back(new_action);
            populate_treeview();
        }
    }
}

void EditorWindow::on_button_edit() {
    Glib::RefPtr<Gtk::TreeSelection> selection = m_TreeView.get_selection();
    Gtk::TreeModel::iterator iter = selection->get_selected();
    if (!iter) return;

    bool is_group = (*iter)[m_Columns.m_col_is_group];
    int group_index = (*iter)[m_Columns.m_col_group_iter];

    if (is_group) {
        EditDialog dialog(*this, "Edit Group", true);
        dialog.set_group_data(m_config.groups[group_index]);
        if (dialog.run() == Gtk::RESPONSE_OK) {
            dialog.get_group_data(m_config.groups[group_index]);
            populate_treeview();
        }
    } else {
        Gtk::TreePath path(iter);
        int action_index = path.front() - 1;
        for(int i = 0; i < group_index; ++i) {
            action_index -= (m_config.groups[i].actions.size() + 1);
        }

        EditDialog dialog(*this, "Edit Action", false);
        dialog.set_action_data(m_config.groups[group_index].actions[action_index]);
        if (dialog.run() == Gtk::RESPONSE_OK) {
            dialog.get_action_data(m_config.groups[group_index].actions[action_index]);
            populate_treeview();
        }
    }
}

void EditorWindow::on_button_delete() {
    Glib::RefPtr<Gtk::TreeSelection> selection = m_TreeView.get_selection();
    Gtk::TreeModel::iterator iter = selection->get_selected();
    if (!iter) return;

    bool is_group = (*iter)[m_Columns.m_col_is_group];
    int group_index = (*iter)[m_Columns.m_col_group_iter];

    if (is_group) {
        m_config.groups.erase(m_config.groups.begin() + group_index);
    } else {
        Gtk::TreePath path(iter);
        int action_index = path.front() - 1;
        for(int i = 0; i < group_index; ++i) {
            action_index -= (m_config.groups[i].actions.size() + 1);
        }
        m_config.groups[group_index].actions.erase(m_config.groups[group_index].actions.begin() + action_index);
    }
    populate_treeview();
}

void EditorWindow::on_button_move_up() {
    Glib::RefPtr<Gtk::TreeSelection> selection = m_TreeView.get_selection();
    Gtk::TreeModel::iterator iter = selection->get_selected();
    if (!iter) return;

    bool is_group = (*iter)[m_Columns.m_col_is_group];
    int group_index = (*iter)[m_Columns.m_col_group_iter];

    if (is_group) {
        if (group_index > 0) {
            std::swap(m_config.groups[group_index], m_config.groups[group_index - 1]);
        }
    } else {
        Gtk::TreePath path(iter);
        int action_index = path.front() - 1;
        for(int i = 0; i < group_index; ++i) {
            action_index -= (m_config.groups[i].actions.size() + 1);
        }
        if (action_index > 0) {
            std::swap(m_config.groups[group_index].actions[action_index], m_config.groups[group_index].actions[action_index - 1]);
        }
    }
    populate_treeview();
}

void EditorWindow::on_button_move_down() {
    Glib::RefPtr<Gtk::TreeSelection> selection = m_TreeView.get_selection();
    Gtk::TreeModel::iterator iter = selection->get_selected();
    if (!iter) return;

    bool is_group = (*iter)[m_Columns.m_col_is_group];
    int group_index = (*iter)[m_Columns.m_col_group_iter];

    if (is_group) {
        if ((size_t)group_index < m_config.groups.size() - 1) {
            std::swap(m_config.groups[group_index], m_config.groups[group_index + 1]);
        }
    } else {
        Gtk::TreePath path(iter);
        int action_index = path.front() - 1;
        for(int i = 0; i < group_index; ++i) {
            action_index -= (m_config.groups[i].actions.size() + 1);
        }
        if ((size_t)action_index < m_config.groups[group_index].actions.size() - 1) {
            std::swap(m_config.groups[group_index].actions[action_index], m_config.groups[group_index].actions[action_index + 1]);
        }
    }
    populate_treeview();
}

void EditorWindow::load_config() {
    std::string config_dir = Glib::get_user_config_dir() + "/primecuts";
    std::string config_path = config_dir + "/config.json";
    if (!file_exists(config_path)) {
        system(("mkdir -p " + config_dir).c_str());
        PrimeCuts::Config defaultConfig;
        m_configLoader.loadFromFile("config.json.example", defaultConfig);
        m_configLoader.saveToFile(config_path, defaultConfig);
    }
    if (!m_configLoader.loadFromFile(config_path, m_config)) {
        // Handle error or create a default config
        std::cerr << "Failed to load config file: " << config_path << std::endl;
    }
}

void EditorWindow::populate_treeview() {
    m_refTreeModel->clear();
    int group_iter = 0;
    for (const auto& group : m_config.groups) {
        Gtk::TreeModel::Row row = *(m_refTreeModel->append());
        row[m_Columns.m_col_text] = "<b>" + group.name + "</b>";
        row[m_Columns.m_col_is_group] = true;
        row[m_Columns.m_col_group_iter] = group_iter;
        for (const auto& action : group.actions) {
            Gtk::TreeModel::Row childrow = *(m_refTreeModel->append());
            childrow[m_Columns.m_col_text] = "  " + action.name;
            childrow[m_Columns.m_col_is_group] = false;
            childrow[m_Columns.m_col_group_iter] = group_iter;
        }
        group_iter++;
    }
    // Make the text bold for groups
    m_TreeView.get_column(0)->set_cell_data_func(*m_TreeView.get_column(0)->get_first_cell(),
        [this](Gtk::CellRenderer* renderer, const Gtk::TreeModel::const_iterator& iter) {
        Gtk::CellRendererText* cell = dynamic_cast<Gtk::CellRendererText*>(renderer);
        if (cell) {
            bool is_group = (*iter)[m_Columns.m_col_is_group];
            Glib::ustring text = (*iter)[m_Columns.m_col_text];
            if (is_group) {
                cell->property_markup() = text;
            } else {
                cell->property_text() = text;
            }
        }
    });
}
