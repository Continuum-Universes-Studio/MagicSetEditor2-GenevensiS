//+----------------------------------------------------------------------------+
//| Description:  Magic Set Editor - Program to make card games                |
//| Copyright:    (C) Twan van Laarhoven and the other MSE developers          |
//| License:      GNU General Public License 2 or later (see file COPYING)     |
//+----------------------------------------------------------------------------+

// ----------------------------------------------------------------------------- : Includes

#include <util/prec.hpp>
#include <gui/preferences_window.hpp>
#include <data/settings.hpp>
#include <util/window_id.hpp>
#include <util/io/package_manager.hpp>
#include <gui/packages_window.hpp>
#include <gui/downloadable_installers.hpp>
#include <wx/spinctrl.h>
#include <wx/filename.h>
#include <wx/notebook.h>

// use a combo box for the zoom choices instead of a spin control
#define USE_ZOOM_COMBOBOX 1

// ----------------------------------------------------------------------------- : Preferences pages

// A page from the preferences dialog
class PreferencesPage : public wxPanel {
public:
  PreferencesPage(Window* parent)
    : wxPanel(parent, wxID_ANY)
  {}
  
  /// Stores the settings from the panel in the global settings object
  virtual void store() = 0;
};

// Preferences page for global MSE settings
class GlobalPreferencesPage : public PreferencesPage {
public:
  GlobalPreferencesPage(Window* parent);
  void store() override;  
  
private:
  wxComboBox* language, *dark_mode;
  wxCheckBox* open_sets_in_new_window;
};

// Preferences page for card viewing related settings
class DisplayPreferencesPage : public PreferencesPage {
public:
  DisplayPreferencesPage(Window* parent);
  void store() override;  
  
private:
  DECLARE_EVENT_TABLE();
  
  wxCheckBox* high_quality, *borders, *draw_editing, *spellcheck_enabled, *hide_back_faces;
  
  wxComboBox* zoom;
  int zoom_int;
  
  void onSelectColumns(wxCommandEvent&);
  void onZoomChange(wxCommandEvent&);
  void updateZoom();
};

class TransfersPreferencesPage : public PreferencesPage {
public:
  TransfersPreferencesPage(Window* parent);
  void store() override;

private:
  wxCheckBox* non_normal_export, *bleed_export, *notes_export, *metaimage_export, *dfc_export, *allow_image_download, *dfc_copy;

  wxChoice*   export_scale, *import_scale, *clipboard_scale;
};

// Preferences page for directories of programs
// i.e. Apprentice, Magic Workstation
// perhaps in the future also directories for packages?
class DirsPreferencesPage : public PreferencesPage {
public:
  DirsPreferencesPage(Window* parent);
  void store() override;
  
private:
  DECLARE_EVENT_TABLE();
  
  wxTextCtrl* apprentice;
  
  void onApprenticeBrowse(wxCommandEvent&);
};

// Preferences page for automatic updates
class UpdatePreferencesPage : public PreferencesPage {
public:
  UpdatePreferencesPage(Window* parent);
  void store() override;
  
private:
  DECLARE_EVENT_TABLE();

  wxChoice* check_what;
  wxChoice* check_when;

  // check for updates
  void onCheckUpdatesNow(wxCommandEvent&);
};


// ----------------------------------------------------------------------------- : PreferencesWindow

PreferencesWindow::PreferencesWindow(Window* parent)
  : wxDialog(parent, wxID_ANY, _TITLE_("preferences"), wxDefaultPosition)
{
  // init notebook
  wxNotebook* nb = new wxNotebook(this, ID_NOTEBOOK);
  nb->AddPage(new GlobalPreferencesPage (nb), _TITLE_("global"));
  nb->AddPage(new DisplayPreferencesPage(nb), _TITLE_("display"));
  nb->AddPage(new TransfersPreferencesPage(nb), _TITLE_("transfers"));
  nb->AddPage(new DirsPreferencesPage   (nb), _TITLE_("directories"));
  nb->AddPage(new UpdatePreferencesPage (nb), _TITLE_("updates"));
  
  // init sizer
  wxSizer* s = new wxBoxSizer(wxVERTICAL);
  s->Add(nb,                                 1, wxEXPAND | (wxALL & ~wxBOTTOM), 8);
  s->AddSpacer(4);
  s->Add(CreateButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | (wxALL & ~wxTOP),    8);
  SetSizer(s);
  s->SetSizeHints(this);
}

void PreferencesWindow::onOk(wxCommandEvent&) {
  // store each page
  wxNotebook* nb = static_cast<wxNotebook*>(FindWindow(ID_NOTEBOOK));
  size_t count = nb->GetPageCount();
  for (size_t i = 0 ; i < count ; ++i) {
    static_cast<PreferencesPage*>(nb->GetPage(i))->store();
  }
  // close
  EndModal(wxID_OK);
}

BEGIN_EVENT_TABLE(PreferencesWindow, wxDialog)
  EVT_BUTTON       (wxID_OK, PreferencesWindow::onOk)
END_EVENT_TABLE  ()


// ----------------------------------------------------------------------------- : Preferences page : global

bool compare_package_name(const PackagedP& a, const PackagedP& b) {
  return a->name() < b->name();
}

GlobalPreferencesPage::GlobalPreferencesPage(Window* parent)
  : PreferencesPage(parent)
{
  // init controls
  language = new wxComboBox(this, wxID_ANY, _(""), wxDefaultPosition, wxDefaultSize, 0, nullptr, wxCB_READONLY);
  open_sets_in_new_window = new wxCheckBox(this, wxID_ANY, _BUTTON_("open sets in new window"));
  dark_mode = new wxComboBox(this, wxID_ANY, _(""), wxDefaultPosition, wxDefaultSize, 0, nullptr, wxCB_READONLY);
  // set values
  vector<PackagedP> locales;
  package_manager.findMatching(_("*.mse-locale"), locales);
  sort(locales.begin(), locales.end(), compare_package_name);
  int n = 0;
  FOR_EACH(package, locales) {
    language->Append(package->name() + _(": ") + package->full_name, package.get());
    if (settings.locale == package->name()) {
      language->SetSelection(n);
    }
    n++;
  }
  open_sets_in_new_window->SetValue(settings.open_sets_in_new_window);
  dark_mode->Append(_LABEL_("dark mode system"));
  dark_mode->Append(_LABEL_("dark mode no"));
  dark_mode->Append(_LABEL_("dark mode yes"));
  dark_mode->SetSelection((int)settings.dark_mode_type);
  // init sizer
  wxSizer* s = new wxBoxSizer(wxVERTICAL);
    wxSizer* s2 = new wxStaticBoxSizer(wxVERTICAL, this, _LABEL_("language"));
      s2->Add(new wxStaticText(this, wxID_ANY, _LABEL_("app language")), 0,             wxALL,           4);
      s2->Add(language,                                                  0, wxEXPAND | (wxALL & ~wxTOP), 4);
      s2->Add(new wxStaticText(this, wxID_ANY, _HELP_( "app language")), 0,             wxALL,           4);
    s->Add(s2, 0, wxEXPAND | wxALL, 8);
    wxSizer* s3 = new wxStaticBoxSizer(wxVERTICAL, this, _LABEL_("windows"));
      s3->Add(open_sets_in_new_window, 0, wxALL, 4);
    s->Add(s3, 0, wxEXPAND | (wxALL & ~wxTOP), 8);
    wxSizer* s4 = new wxStaticBoxSizer(wxVERTICAL, this, _LABEL_("dark mode"));
      s4->Add(dark_mode,                                                 0, wxEXPAND | (wxALL & ~wxTOP), 4);
      s4->Add(new wxStaticText(this, wxID_ANY, _HELP_( "app language")), 0,             wxALL,           4);
    s->Add(s4, 0, wxEXPAND | wxALL, 8);
  SetSizer(s);
  s->SetSizeHints(this);
}

void GlobalPreferencesPage::store() {
  // locale
  int n = language->GetSelection();
  if (n == wxNOT_FOUND) return;
  Packaged* p = (Packaged*)language->GetClientData(n);
  settings.locale = p->name();
  // set the_locale?
  // open_sets_in_new_window
  settings.open_sets_in_new_window = open_sets_in_new_window->GetValue();
  // dark mode
  settings.dark_mode_type = (DarkModeType)dark_mode->GetSelection();
}

// ----------------------------------------------------------------------------- : Preferences page : display

DisplayPreferencesPage::DisplayPreferencesPage(Window* parent)
  : PreferencesPage(parent)
{
  // init controls
  high_quality       = new wxCheckBox(this, wxID_ANY, _BUTTON_("high quality"));
  borders            = new wxCheckBox(this, wxID_ANY, _BUTTON_("show lines"));
  draw_editing       = new wxCheckBox(this, wxID_ANY, _BUTTON_("show editing hints"));
  spellcheck_enabled = new wxCheckBox(this, wxID_ANY, _BUTTON_("spellcheck enabled"));
  hide_back_faces    = new wxCheckBox(this, wxID_ANY, _BUTTON_("hide back faces"));
  zoom               = new wxComboBox(this, ID_ZOOM);

  // set values
  high_quality->      SetValue( settings.default_stylesheet_settings.card_anti_alias());
  borders->           SetValue( settings.default_stylesheet_settings.card_borders());
  draw_editing->      SetValue( settings.default_stylesheet_settings.card_draw_editing());
  spellcheck_enabled->SetValue( settings.default_stylesheet_settings.card_spellcheck_enabled());
  hide_back_faces->   SetValue( settings.default_stylesheet_settings.list_hide_back_faces());
  zoom_int = static_cast<int>(  settings.default_stylesheet_settings.card_zoom() * 100);
  zoom->SetValue(String::Format(_("%d%%"),zoom_int));
  for (int i : Settings::scale_choices) {
    zoom->Append(String::Format(_("%d%%"), i));
  }

  // init sizer
  wxSizer* s = new wxBoxSizer(wxVERTICAL);
  wxSizer* s2 = new wxStaticBoxSizer(wxVERTICAL, this, _LABEL_("card display"));
  s2->Add(high_quality,       0, wxEXPAND | wxALL, 4);
  s2->Add(borders,            0, wxEXPAND | wxALL, 4);
  s2->Add(draw_editing,       0, wxEXPAND | wxALL, 4);
  s2->Add(spellcheck_enabled, 0, wxEXPAND | wxALL, 4);
  wxSizer* s3 = new wxBoxSizer(wxHORIZONTAL);
  s3->Add(new wxStaticText(this, wxID_ANY, _LABEL_("zoom")),             0, wxALL & ~wxLEFT,  4);
  s3->AddSpacer(2);
  s3->Add(zoom);
  s3->Add(new wxStaticText(this, wxID_ANY, _LABEL_("percent of normal")),1, wxALL & ~wxRIGHT, 4);
  s2->Add(s3, 0, wxEXPAND | wxALL, 4);
  s->Add(s2, 0, wxEXPAND | wxALL, 8);
  wxSizer* s4 = new wxStaticBoxSizer(wxVERTICAL, this, _LABEL_("card list display"));
  s4->Add(hide_back_faces, 0, wxEXPAND | wxALL, 4);
  s->Add(s4, 0, wxEXPAND | wxALL, 8);
  SetSizer(s);
  s->SetSizeHints(this);
}

void DisplayPreferencesPage::store() {
  settings.default_stylesheet_settings.card_anti_alias         = high_quality->GetValue();
  settings.default_stylesheet_settings.card_borders            = borders->GetValue();
  settings.default_stylesheet_settings.card_draw_editing       = draw_editing->GetValue();
  settings.default_stylesheet_settings.card_spellcheck_enabled = spellcheck_enabled->GetValue();
  settings.default_stylesheet_settings.list_hide_back_faces    = hide_back_faces->GetValue();

  updateZoom();
  settings.default_stylesheet_settings.card_zoom   = zoom_int / 100.0;
}

void DisplayPreferencesPage::onSelectColumns(wxCommandEvent&) {
  // Impossible, set specific
}

void DisplayPreferencesPage::onZoomChange(wxCommandEvent&) {
  updateZoom();
}

void DisplayPreferencesPage::updateZoom() {
  String s = zoom->GetValue();
  int i = zoom_int;
  if (wxSscanf(s.c_str(),_("%u"),&i)) {
    zoom_int = min(max(i,1),1000);
  }
  zoom->SetValue(String::Format(_("%d%%"),(int)zoom_int));
}

BEGIN_EVENT_TABLE(DisplayPreferencesPage, wxPanel)
  EVT_BUTTON       (ID_SELECT_COLUMNS, DisplayPreferencesPage::onSelectColumns)
  EVT_COMBOBOX     (ID_ZOOM, DisplayPreferencesPage::onZoomChange)
  EVT_TEXT_ENTER   (ID_ZOOM, DisplayPreferencesPage::onZoomChange)
END_EVENT_TABLE  ()

// ----------------------------------------------------------------------------- : Preferences page : internal

TransfersPreferencesPage::TransfersPreferencesPage(Window* parent) : PreferencesPage(parent) {
  // init controls
  non_normal_export        = new wxCheckBox(this, wxID_ANY, _BUTTON_("rotation export"));
  bleed_export             = new wxCheckBox(this, wxID_ANY, _BUTTON_("bleed export"));
  notes_export             = new wxCheckBox(this, wxID_ANY, _BUTTON_("notes export"));
  metaimage_export         = new wxCheckBox(this, wxID_ANY, _BUTTON_("metaimage export"));
  dfc_export               = new wxCheckBox(this, wxID_ANY, _BUTTON_("dfc export"));
  export_scale             = new wxChoice  (this, ID_EXPORT_ZOOM);

  allow_image_download     = new wxCheckBox(this, wxID_ANY, _BUTTON_("allow image download"));
  import_scale             = new wxChoice  (this, ID_IMPORT_ZOOM);
  clipboard_scale          = new wxChoice  (this, ID_CLIPBOARD_ZOOM);
  dfc_copy                 = new wxCheckBox(this, wxID_ANY, _BUTTON_("dfc copy"));

  // set values
  non_normal_export-> SetValue(!settings.default_stylesheet_settings.card_normal_export());
  bleed_export->      SetValue( settings.default_stylesheet_settings.card_bleed_export());
  notes_export->      SetValue( settings.default_stylesheet_settings.card_notes_export());
  metaimage_export->  SetValue( settings.default_stylesheet_settings.card_metaimage_export());
  dfc_export->        SetValue( settings.default_stylesheet_settings.card_dfc_export());
  export_scale->Append(_LABEL_("export around 300"));
  export_scale->Append(_LABEL_("export force 300"));
  export_scale->Append(_LABEL_("export force 150"));
  for (int i : Settings::scale_choices) {
    export_scale->Append(String::Format(_("%d%%"), i));
  }
  int default_export_scale = settings.default_stylesheet_settings.export_scale_selection();
  if (default_export_scale < 0 || default_export_scale > (int)export_scale->GetCount() - 1) default_export_scale = 0;
  export_scale->SetSelection(default_export_scale);

  allow_image_download->SetValue(settings.allow_image_download);
  import_scale->Append(_LABEL_("use export scale"));
  import_scale->Append(_LABEL_("export around 300"));
  import_scale->Append(_LABEL_("export force 300"));
  import_scale->Append(_LABEL_("export force 150"));
  for (int i : Settings::scale_choices) {
    import_scale->Append(String::Format(_("%d%%"), i));
  }
  int default_import_scale = settings.import_scale_selection;
  if (default_import_scale < 0 || default_import_scale > import_scale->GetCount() - 1) default_import_scale = 0;
  import_scale->SetSelection(default_import_scale);

  clipboard_scale->Append(_LABEL_("use export scale"));
  clipboard_scale->Append(_LABEL_("export around 300"));
  clipboard_scale->Append(_LABEL_("export force 300"));
  clipboard_scale->Append(_LABEL_("export force 150"));
  for (int i : Settings::scale_choices) {
    clipboard_scale->Append(String::Format(_("%d%%"), i));
  }
  int default_clipboard_scale = settings.clipboard_scale_selection;
  if (default_clipboard_scale < 0 || default_clipboard_scale > clipboard_scale->GetCount() - 1) default_clipboard_scale = 0;
  clipboard_scale->SetSelection(default_clipboard_scale);
  dfc_copy->SetValue(settings.card_dfc_copy);

  // init sizers
  wxSizer* s = new wxBoxSizer(wxVERTICAL);
    wxSizer* s2 = new wxStaticBoxSizer(wxVERTICAL, this, _LABEL_("export"));
      s2->Add(new wxStaticText(this, wxID_ANY, _LABEL_("export desc")), 0, wxALL & ~wxLEFT, 4);
      wxSizer* s3 = new wxBoxSizer(wxHORIZONTAL);
        s3->Add(new wxStaticText(this, wxID_ANY, _LABEL_("scale")), 0, wxALL & ~wxLEFT, 4);
        s3->AddSpacer(2);
        s3->Add(export_scale);
      s2->Add(s3, 0, wxEXPAND | wxALL, 4);
      s2->Add(non_normal_export, 0, wxEXPAND | wxALL, 4);
      s2->Add(bleed_export, 0, wxEXPAND | wxALL, 4);
      s2->Add(notes_export, 0, wxEXPAND | wxALL, 4);
      s2->Add(metaimage_export, 0, wxEXPAND | wxALL, 4);
      s2->Add(dfc_export, 0, wxEXPAND | wxALL, 4);
    wxSizer* s5 = new wxStaticBoxSizer(wxVERTICAL, this, _LABEL_("import"));
      s5->Add(new wxStaticText(this, wxID_ANY, _LABEL_("import desc")), 0, wxALL & ~wxLEFT, 4);
      wxSizer* s6 = new wxBoxSizer(wxHORIZONTAL);
        s6->Add(new wxStaticText(this, wxID_ANY, _LABEL_("scale")), 0, wxALL & ~wxLEFT, 4);
        s6->AddSpacer(2);
        s6->Add(import_scale);
      s5->Add(s6, 0, wxEXPAND | wxALL & ~wxBottom, 4);
      s5->Add(new wxStaticText(this, wxID_ANY, _LABEL_("internal scale desc")), 0, wxALL & ~wxTOP, 4);
      s5->Add(allow_image_download, 0, wxEXPAND | wxALL, 4);
    wxSizer* s7 = new wxStaticBoxSizer(wxVERTICAL, this, _LABEL_("clipboard"));
      s7->Add(new wxStaticText(this, wxID_ANY, _LABEL_("clipboard desc")), 0, wxALL & ~wxLEFT, 4);
      wxSizer* s8 = new wxBoxSizer(wxHORIZONTAL);
        s8->Add(new wxStaticText(this, wxID_ANY, _LABEL_("scale")), 0, wxALL & ~wxLEFT, 4);
        s8->AddSpacer(2);
        s8->Add(clipboard_scale);
      s7->Add(s8, 0, wxEXPAND | wxALL, 4);
      s7->Add(dfc_copy, 0, wxEXPAND | wxALL, 4);
    s->Add(s2, 0, wxEXPAND | wxALL, 8);
    s->Add(s5, 0, wxEXPAND | wxALL, 8);
    s->Add(s7, 0, wxEXPAND | wxALL, 8);
  export_scale->SetFocus();
  SetSizer(s);
  s->SetSizeHints(this);
}

void TransfersPreferencesPage::store() {
  settings.default_stylesheet_settings.card_normal_export     = !non_normal_export->GetValue();
  settings.default_stylesheet_settings.card_bleed_export      = bleed_export->GetValue();
  settings.default_stylesheet_settings.card_notes_export      = notes_export->GetValue();
  settings.default_stylesheet_settings.card_metaimage_export  = metaimage_export->GetValue();
  settings.default_stylesheet_settings.card_dfc_export        = dfc_export->GetValue();
  settings.default_stylesheet_settings.export_scale_selection = export_scale->GetSelection();

  settings.allow_image_download                               = allow_image_download->GetValue();
  settings.import_scale_selection                             = import_scale->GetSelection();
  settings.clipboard_scale_selection                          = clipboard_scale->GetSelection();
  settings.card_dfc_copy                                      = dfc_copy->GetValue();
}

// ----------------------------------------------------------------------------- : Preferences page : directories

DirsPreferencesPage::DirsPreferencesPage(Window* parent)
  : PreferencesPage(parent)
{
  // init controls
  apprentice   = new wxTextCtrl(this, wxID_ANY);
  wxButton* ab = new wxButton(this, ID_APPRENTICE_BROWSE, _BUTTON_("browse"));
  // set values
  apprentice->SetValue(settings.apprentice_location);
  // init sizer
  wxSizer* s = new wxBoxSizer(wxVERTICAL);
    wxSizer* s2 = new wxStaticBoxSizer(wxVERTICAL, this, _LABEL_("external programs"));
      s2->Add(new wxStaticText(this, wxID_ANY, _LABEL_("apprentice")), 0, wxALL, 4);
      wxSizer* s3 = new wxBoxSizer(wxHORIZONTAL);
        s3->Add(apprentice, 1, wxEXPAND | wxRIGHT, 4);
        s3->Add(ab,         0, wxEXPAND);
      s2->Add(s3, 0, wxEXPAND | (wxALL & ~wxTOP), 4);
    s->Add(s2, 0, wxEXPAND | wxALL, 8);
  SetSizer(s);
  s->SetSizeHints(this);
}

void DirsPreferencesPage::store() {
  settings.apprentice_location = apprentice->GetValue();
}

void DirsPreferencesPage::onApprenticeBrowse(wxCommandEvent&) {
  // browse for appr.exe
  wxFileDialog dlg(this, _TITLE_("locate apprentice"), apprentice->GetValue(), _(""), _LABEL_("apprentice exe") + _("|appr.exe"), wxFD_OPEN);
  if (dlg.ShowModal() == wxID_OK) {
    wxFileName fn(dlg.GetPath());
    apprentice->SetValue(fn.GetPath());
  }
}
  
BEGIN_EVENT_TABLE(DirsPreferencesPage, wxPanel)
  EVT_BUTTON     (ID_APPRENTICE_BROWSE, DirsPreferencesPage::onApprenticeBrowse)
END_EVENT_TABLE  ();


// ----------------------------------------------------------------------------- : Preferences page : updates

UpdatePreferencesPage::UpdatePreferencesPage(Window* parent)
  : PreferencesPage(parent)
{
  // init controls
  check_what          = new wxChoice(this, wxID_ANY);
  check_when          = new wxChoice(this, wxID_ANY);
  wxButton* check_now = new wxButton(this, ID_CHECK_UPDATES_NOW, _BUTTON_("check now"));
  // set values
  check_when->Append(_BUTTON_("every 7 days"));      // 0
  check_when->Append(_BUTTON_("every 30 days"));     // 1
  check_when->Append(_BUTTON_("never"));             // 2
  check_when->SetSelection(settings.check_updates_when);
  check_what->Append(_BUTTON_("check app"));         // 0
  check_what->Append(_BUTTON_("check games"));       // 1
  check_what->Append(_BUTTON_("check everything"));  // 2
  check_what->SetSelection(settings.check_updates_what);
  // init sizer
  wxSizer* s = new wxBoxSizer(wxVERTICAL);
    s->Add(new wxStaticText(this, wxID_ANY, _LABEL_("check at startup")),           0, wxALL, 8);
    s->Add(check_when,                                                              0, wxALL & ~wxTOP, 8);
    s->Add(check_now,                                                               0, wxALL & ~wxTOP, 8);
    s->Add(new wxStaticText(this, wxID_ANY, _LABEL_("check what targets")),         0, wxALL, 8);
    s->Add(check_what,                                                              0, wxALL & ~wxTOP, 8);
    s->Add(new wxStaticText(this, wxID_ANY, _LABEL_("checking requires internet")), 0, wxALL, 8);
  SetSizer(s);
}

void UpdatePreferencesPage::store() {
  int sel1 = check_when->GetSelection();
  if      (sel1 == 0) settings.check_updates_when = CHECK_7_DAYS;
  else if (sel1 == 1) settings.check_updates_when = CHECK_30_DAYS;
  else                settings.check_updates_when = CHECK_NEVER;

  int sel2 = check_what->GetSelection();
  if      (sel2 == 0) settings.check_updates_what = CHECK_APP;
  else if (sel2 == 1) settings.check_updates_what = CHECK_GAMES;
  else                settings.check_updates_what = CHECK_EVERYTHING;
}

void UpdatePreferencesPage::onCheckUpdatesNow(wxCommandEvent&) {
  downloadable_installers.shown_dialog = true;
  downloadable_installers.check_updates_now(false);
  if (downloadable_installers.check_status == DownloadableInstallerList::CheckStatus::FAILED) {
    wxMessageBox(_ERROR_("checking updates failed"), _TITLE_("update check"), wxICON_ERROR | wxOK);
  } else if (downloadable_installers.check_status == DownloadableInstallerList::CheckStatus::NOT_FOUND) {
    wxMessageBox(_ERROR_("no updates"),              _TITLE_("update check"), wxICON_INFORMATION | wxOK);
  } else {
    wxWindow* set_window = this;
    while (set_window->GetParent()) set_window = set_window->GetParent();
    (new PackagesWindow(set_window))->Show();
  }
}

BEGIN_EVENT_TABLE(UpdatePreferencesPage, wxPanel)
  EVT_BUTTON      (ID_CHECK_UPDATES_NOW, UpdatePreferencesPage::onCheckUpdatesNow)
END_EVENT_TABLE  ()
