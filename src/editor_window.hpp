#ifndef EDITOR_WINDOW_HPP
#define EDITOR_WINDOW_HPP

#include <gtkmm.h>
#include "config_loader.hpp"
#include "edit_dialog.hpp"

class EditorWindow : public Gtk::Window {
public:
  EditorWindow();
  virtual ~EditorWindow();

protected:
  // Signal handlers
  void on_button_quit();
  void on_button_add_group();
  void on_button_add_action();
  void on_button_edit();
  void on_button_delete();
  void on_button_move_up();
  void on_button_move_down();
  void on_button_save();

  // TreeView model
  class ModelColumns : public Gtk::TreeModel::ColumnRecord {
  public:
    ModelColumns() {
      add(m_col_text);
      add(m_col_is_group);
      add(m_col_group_iter);
    }
    Gtk::TreeModelColumn<Glib::ustring> m_col_text;
    Gtk::TreeModelColumn<bool> m_col_is_group;
    Gtk::TreeModelColumn<int> m_col_group_iter;
  };

  ModelColumns m_Columns;

  // Member widgets
  Gtk::Box m_VBox;
  Gtk::ScrolledWindow m_ScrolledWindow;
  Gtk::TreeView m_TreeView;
  Glib::RefPtr<Gtk::ListStore> m_refTreeModel;

  Gtk::Box m_ButtonBox;
  Gtk::Button m_AddGroupButton;
  Gtk::Button m_AddActionButton;
  Gtk::Button m_EditButton;
  Gtk::Button m_DeleteButton;
  Gtk::Button m_MoveUpButton;
  Gtk::Button m_MoveDownButton;
  Gtk::Button m_SaveButton;
  Gtk::Button m_QuitButton;

private:
  PrimeCuts::ConfigLoader m_configLoader;
  PrimeCuts::Config m_config;

  void load_config();
  void populate_treeview();
};

#endif // EDITOR_WINDOW_HPP
