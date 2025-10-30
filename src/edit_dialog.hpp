#ifndef EDIT_DIALOG_HPP
#define EDIT_DIALOG_HPP

#include <gtkmm.h>
#include "config.hpp"

class EditDialog : public Gtk::Dialog {
public:
    EditDialog(Gtk::Window& parent, const std::string& title, bool is_group);

    void set_group_data(const PrimeCuts::Group& group);
    void get_group_data(PrimeCuts::Group& group);

    void set_action_data(const PrimeCuts::Action& action);
    void get_action_data(PrimeCuts::Action& action);

private:
    Gtk::Grid m_Grid;
    Gtk::Label m_NameLabel;
    Gtk::Entry m_NameEntry;
    Gtk::Label m_DescriptionLabel;
    Gtk::Entry m_DescriptionEntry;
    Gtk::Label m_IconLabel;
    Gtk::Entry m_IconEntry;
    Gtk::Label m_CommandLabel;
    Gtk::Entry m_CommandEntry;
    Gtk::Label m_KeywordsLabel;
    Gtk::Entry m_KeywordsEntry;
    Gtk::Label m_TypeLabel;
    Gtk::ComboBoxText m_TypeComboBox;
};

#endif // EDIT_DIALOG_HPP
