//+----------------------------------------------------------------------------+
//| Description:  Magic Set Editor - Program to make card games                |
//| Copyright:    (C) Twan van Laarhoven and the other MSE developers          |
//| License:      GNU General Public License 2 or later (see file COPYING)     |
//+----------------------------------------------------------------------------+

// ----------------------------------------------------------------------------- : Includes

#include <util/prec.hpp>
#include <gui/new_window.hpp>
#include <gui/control/gallery_list.hpp>
#include <gui/control/package_list.hpp>
#include <gui/downloadable_installers.hpp>
#include <data/installer.hpp>
#include <data/game.hpp>
#include <data/stylesheet.hpp>
#include <data/set.hpp>
#include <data/card.hpp>
#include <data/settings.hpp>
#include <util/window_id.hpp>

// ----------------------------------------------------------------------------- : NewSetWindow

SetP new_set_window(Window* parent) {
  NewSetWindow wnd(parent);
  wnd.ShowModal();
  return wnd.set;
}

NewSetWindow::NewSetWindow(Window* parent)
  : wxDialog(parent, wxID_ANY, _TITLE_("new set"), wxDefaultPosition, wxSize(830,360), wxDEFAULT_DIALOG_STYLE)
{
  wxBusyCursor wait;
  // init controls
  game_list       = new PackageList (this, ID_GAME_LIST,       wxHORIZONTAL, false);
  stylesheet_list = new PackageList (this, ID_STYLESHEET_LIST, wxHORIZONTAL, false);
  wxStaticText* game_text       = new wxStaticText(this, ID_GAME_LIST,       _LABEL_("game type"));
  wxStaticText* stylesheet_text = new wxStaticText(this, ID_STYLESHEET_LIST, _LABEL_("style type"));

  game_filter = new FilterCtrl(this, ID_GAME_FILTER, _LABEL_("search game list"), _HELP_("search game list control"));
  game_filter->setFilter(game_filter_value);

  stylesheet_filter = new FilterCtrl(this, ID_STYLESHEET_FILTER, _LABEL_("search stylesheet list"), _HELP_("search stylesheet list control"));
  stylesheet_filter->setFilter(stylesheet_filter_value);

  // init sizer
  wxSizer* s = new wxBoxSizer(wxVERTICAL);
    wxSizer* s2 = new wxBoxSizer(wxHORIZONTAL);
      s2->Add(game_text, 0, wxALL & ~wxLEFT, 4);
      s2->AddStretchSpacer();
      s2->Add(game_filter, 1, wxRIGHT, 4);
    s->Add(s2, wxSizerFlags().Expand().Border(wxALL, 6));
    s->Add(game_list,       0, wxEXPAND | (wxALL & ~wxTOP), 4);
    wxSizer* s3 = new wxBoxSizer(wxHORIZONTAL);
      s3->Add(stylesheet_text, 0, wxALL & ~wxLEFT, 4);
      s3->AddStretchSpacer();
      s3->Add(stylesheet_filter, 1, wxRIGHT, 4);
    s->Add(s3, wxSizerFlags().Expand().Border(wxALL, 6));
    s->Add(stylesheet_list, 0, wxEXPAND | (wxALL & ~wxTOP), 4);
    s->Add(CreateButtonSizer(wxOK | wxCANCEL) , 0, wxEXPAND | wxALL, 8);
  SetSizer(s);
  s->SetSizeHints(this);
  // Resize
  Layout();
  wxSize min_size = GetSizer()->GetMinSize() + GetSize() - GetClientSize();
  SetSize(830, min_size.y);
  // init lists
  game_list->showData<Game>();
  try {
    game_list->select(settings.default_game);
  } catch (FileNotFoundError const& e) {
    handle_error(e);
  }
  game_list->SetFocus();
  UpdateWindowUI(wxUPDATE_UI_RECURSE);
}

void NewSetWindow::onGameSelect(wxCommandEvent&) {
  wxBusyCursor wait;
  GameP game = game_list->getSelection<Game>(false);
  //handle_pending_errors(); // errors are ignored until set window is shown
  settings.default_game = game->name();
  stylesheet_list->showData<StyleSheet>(game->name() + _("*"));
  stylesheet_list->select(settings.gameSettingsFor(*game).default_stylesheet);
  UpdateWindowUI(wxUPDATE_UI_RECURSE);
  // resize (yuck)
  Layout();
  wxSize min_size = GetSizer()->GetMinSize() + GetSize() - GetClientSize();
  SetSize(830,min_size.y);
}

void NewSetWindow::onStyleSheetSelect(wxCommandEvent&) {
  // store this as default selection
  GameP       game       = game_list      ->getSelection<Game>(false);
  StyleSheetP stylesheet = stylesheet_list->getSelection<StyleSheet>(false);
  //handle_pending_errors(); // errors are ignored until set window is shown
  settings.gameSettingsFor(*game).default_stylesheet = stylesheet->name();
  UpdateWindowUI(wxUPDATE_UI_RECURSE);
}

void NewSetWindow::onStyleSheetActivate(wxCommandEvent&) {
  done();
}

void NewSetWindow::onGameFilterUpdate(wxCommandEvent&) {
    if (game_list->hasSelection()) {
        GameP existingGameSelection = game_list->getSelection<Game>(false);
        game_list->setFilter(game_filter->getFilter<PackageData>());
        game_list->select(existingGameSelection->name());
    }
    else {
        game_list->setFilter(game_filter->getFilter<PackageData>());
    }
}

void NewSetWindow::onStylesheetFilterUpdate(wxCommandEvent&) {
    if (stylesheet_list->hasSelection()) {
        StyleSheetP existingStylesheetSelection = stylesheet_list->getSelection<StyleSheet>(false);
        stylesheet_list->setFilter(stylesheet_filter->getFilter<PackageData>());
        stylesheet_list->select(existingStylesheetSelection->name());
    }
    else {
        stylesheet_list->setFilter(stylesheet_filter->getFilter<PackageData>());
    }
}

void NewSetWindow::OnOK(wxCommandEvent&) {
  done();
}

void NewSetWindow::done() {
  try {
    if (!stylesheet_list->hasSelection()) return;
    StyleSheetP stylesheet = stylesheet_list->getSelection<StyleSheet>();
    set = make_intrusive<Set>(stylesheet);
    set->validate();
    EndModal(wxID_OK);
  } catch (const Error& e) {
    handle_error_now(e);
  }
}

void NewSetWindow::onUpdateUI(wxUpdateUIEvent& ev) {
  switch (ev.GetId()) {
    case ID_STYLESHEET_LIST:
      ev.Enable(game_list->hasSelection());
      break;
    case wxID_OK:
      ev.Enable(stylesheet_list->hasSelection());
      break;
  }
}

void NewSetWindow::onIdle(wxIdleEvent& ev) {
  // Stuff that must be done in the main thread
  //handle_pending_errors(); // errors are ignored until set window is shown
}

BEGIN_EVENT_TABLE(NewSetWindow, wxDialog)
  EVT_GALLERY_SELECT(ID_GAME_LIST, NewSetWindow::onGameSelect)
  EVT_GALLERY_SELECT(ID_STYLESHEET_LIST, NewSetWindow::onStyleSheetSelect)
  EVT_GALLERY_ACTIVATE(ID_STYLESHEET_LIST, NewSetWindow::onStyleSheetActivate)
  EVT_COMMAND_RANGE(ID_STYLESHEET_FILTER, ID_STYLESHEET_FILTER, wxEVT_COMMAND_TEXT_UPDATED, NewSetWindow::onStylesheetFilterUpdate)
  EVT_COMMAND_RANGE(ID_GAME_FILTER, ID_GAME_FILTER, wxEVT_COMMAND_TEXT_UPDATED, NewSetWindow::onGameFilterUpdate)
  EVT_BUTTON          (wxID_OK,            NewSetWindow::OnOK)
  EVT_UPDATE_UI       (wxID_ANY,           NewSetWindow::onUpdateUI)
  EVT_IDLE            (                    NewSetWindow::onIdle)
END_EVENT_TABLE  ()

// ----------------------------------------------------------------------------- : SelectStyleSheetWindow


StyleSheetP select_stylesheet(const Game& game, const String& failed_name, const String& failed_dep) {
  SelectStyleSheetWindow wnd(nullptr, game, failed_name, failed_dep);
  wnd.ShowModal();
  return wnd.stylesheet;
}

SelectStyleSheetWindow::SelectStyleSheetWindow(Window* parent, const Game& game, const String& failed_name, const String& failed_dep)
  : wxDialog(parent, wxID_ANY, _TITLE_("select stylesheet"), wxDefaultPosition, wxSize(830,320), wxDEFAULT_DIALOG_STYLE)
  , game(game)
  , failed_name(failed_name)
  , failed_dep(failed_dep)
{
  wxBusyCursor wait;
  // init controls
  ok_button                     = new wxButton    (this, wxID_OK);
  cancel_button                 = new wxButton    (this, wxID_CANCEL);
  find_online_button            = nullptr;
  find_online_status_text       = nullptr;
  bool find_online_possible     = downloadable_installers.check_status != DownloadableInstallerList::FAILED;
  if (find_online_possible) {
    find_online_button          = new wxButton    (this, ID_DOWNLOAD_STYLESHEET, _BUTTON_("find package online"));
    find_online_status_text     = new wxStaticText(this, wxID_ANY, _(""));
  }
  stylesheet_list               = new PackageList (this, ID_STYLESHEET_LIST);
  wxStaticText* description     = new wxStaticText(this, ID_GAME_LIST, failed_dep.empty() ?
                                                                       _LABEL_1_("stylesheet not found", failed_name) :
                                                                       _LABEL_2_("stylesheet dep not found", failed_name, failed_dep));
  wxStaticText* stylesheet_text = new wxStaticText(this, ID_STYLESHEET_LIST, _LABEL_("style type"));

  stylesheet_filter = new FilterCtrl(this, ID_STYLESHEET_FILTER, _LABEL_("search stylesheet list"), _HELP_("search stylesheet list control"));
  stylesheet_filter->setFilter(stylesheet_filter_value);

  // init sizer
  wxSizer* s = new wxBoxSizer(wxVERTICAL);
    s->Add(description,     0, wxALL,                     4);
    wxSizer* s2 = new wxBoxSizer(wxHORIZONTAL);
      s2->Add(stylesheet_text, 0, wxALL & ~wxLEFT, 4);
      s2->AddStretchSpacer();
      s2->Add(stylesheet_filter, 1, wxRIGHT, 4);
    s->Add(s2, wxSizerFlags().Expand().Border(wxALL, 6));
    s->Add(stylesheet_list, 0, wxEXPAND | (wxALL & ~wxTOP), 4);
    wxBoxSizer* s3 = new wxBoxSizer(wxHORIZONTAL);
      if (find_online_possible) {
        s3->Add(find_online_button, 0, wxRIGHT, 8);
        s3->Add(find_online_status_text, 1, wxALIGN_CENTER_VERTICAL);
      }
      s3->AddStretchSpacer();
      s3->Add(ok_button, 0, wxRIGHT, 4);
      s3->Add(cancel_button, 0);
    s->Add(s3, 0, wxEXPAND | wxALL, 8);
  SetSizer(s);
  s->SetSizeHints(this);
  // init list
  stylesheet_list->showData<StyleSheet>(game.name() + _("*"));
  stylesheet_list->select(settings.gameSettingsFor(game).default_stylesheet);
  // Resize
  Layout();
  wxSize min_size = GetSizer()->GetMinSize() + GetSize() - GetClientSize();
  SetSize(830,min_size.y);
  UpdateWindowUI(wxUPDATE_UI_RECURSE);
  // start downloading the installer list
  downloadable_installers.download();
}

void SelectStyleSheetWindow::onStyleSheetSelect(wxCommandEvent&) {
  //handle_pending_errors(); // errors are ignored until set window is shown
  UpdateWindowUI(wxUPDATE_UI_RECURSE);
}
void SelectStyleSheetWindow::onStyleSheetActivate(wxCommandEvent&) {
  done();
}

void SelectStyleSheetWindow::onStylesheetFilterUpdate(wxCommandEvent&) {
  if (stylesheet_list->hasSelection()) {
    StyleSheetP existingStylesheetSelection = stylesheet_list->getSelection<StyleSheet>(false);
    stylesheet_list->setFilter(stylesheet_filter->getFilter<PackageData>());
    stylesheet_list->select(existingStylesheetSelection->name());
  }
  else {
    stylesheet_list->setFilter(stylesheet_filter->getFilter<PackageData>());
  }
}

void SelectStyleSheetWindow::onFindOnline(wxCommandEvent&) {
  if (searching_online) return;
  searching_online = true;
  find_online_status_text->SetLabel(_LABEL_("searching online"));
  find_online_button->Enable(false);
  //downloadable_installers.download();
}

// ----------------------------------------------------------------------------- : SelectStyleSheetWindow : background install

class SelectStyleSheetWindow::InstallThread : public wxThread {
public:
  InstallThread(SelectStyleSheetWindow* window, InstallablePackages packages)
    : wxThread(wxTHREAD_DETACHED)
    , window(window)
    , packages(std::move(packages))
  {}

protected:
  ExitCode Entry() override {
    try {
      // download stylesheet and dependencies
      FOR_EACH(p, packages) {
        if (!p->has(PACKAGE_ACT_INSTALL)) continue;
        if (!p->ensureIsDownloaded()) {
          throw Error(_("downloading installer fail"));
        }
      }
      setState(INSTALL_INSTALLING);
      // install stylesheet and dependencies
      FOR_EACH(p, packages) {
        if (!p->has(PACKAGE_ACT_INSTALL)) continue;
        if (!package_manager.install(*p)) {
          throw Error(_("installing package fail"));
        }
      }
      setState(INSTALL_SUCCESS);
    } catch (const Error& e) {
      setState(INSTALL_FAILURE, &e);
    } catch (...) {
      setState(INSTALL_FAILURE, nullptr);
    }
    return 0;
  }

private:
  SelectStyleSheetWindow* window;
  InstallablePackages packages;

  void setState(InstallState state, const Error* error = nullptr) {
    wxMutexLocker l(window->install_mutex);
    window->install_state = state;
    window->install_error.reset(error ? new Error(*error) : nullptr);
  }
};

void SelectStyleSheetWindow::tryAutoInstall() {
  auto_installing = true;
  searching_online = false;
  wxBusyCursor busy;
  // search for missing stylesheet
  InstallablePackages packages;
  package_manager.findAllInstalledPackages(packages);
  FOR_EACH(inst, downloadable_installers.installers) {
    merge(packages, inst);
  }
  FOR_EACH(p, packages) {
    if (!p) continue;
    p->determineStatus();
  }
  InstallablePackageP target;
  FOR_EACH(p, packages) {
    if (!p || !p->description) continue;
    if (p->description->name == failed_name) {
      target = p;
      break;
    }
  }
  if (!target) {
    find_online_status_text->SetLabel(_LABEL_("searching online fail"));
    auto_installing = false;
    return;
  }
  try {
    // resolve dependencies
    PackageAction where = is_install_local(settings.install_type) ? PACKAGE_ACT_LOCAL : PACKAGE_ACT_GLOBAL;
    if (!set_package_action(packages, target, PACKAGE_ACT_INSTALL | where)) {
      throw Error(_("resolving dependencies fail"));
    }
    // update UI
    find_online_status_text->SetLabel(_LABEL_("found online downloading"));
    ok_button->Enable(false);
    cancel_button->Enable(false);
    stylesheet_list->Enable(false);
    stylesheet_filter->Enable(false);
    Layout();
    // hand off to an installer background thread, onIdle/pollAutoInstall will pick up the result
    {
      wxMutexLocker l(install_mutex);
      install_state = INSTALL_DOWNLOADING;
      install_error.reset();
    }
    InstallThread* thread = new InstallThread(this, std::move(packages));
    if (thread->Create() != wxTHREAD_NO_ERROR || thread->Run() != wxTHREAD_NO_ERROR) {
      delete thread;
      throw Error(_("starting install thread fail"));
    }
  } catch (const Error& e) {
    handle_error(e);
    auto_installing = false;
    find_online_status_text->SetLabel(_LABEL_("found online fail"));
    ok_button->Enable(true);
    cancel_button->Enable(true);
    stylesheet_list->Enable(true);
    stylesheet_filter->Enable(true);
  }
}

void SelectStyleSheetWindow::pollAutoInstall() {
  InstallState state;
  {
    wxMutexLocker l(install_mutex);
    state = install_state;
  }
  if (state == INSTALL_DOWNLOADING) {
    find_online_status_text->SetLabel(_LABEL_("found online downloading"));
    return; // still running
  }
  if (state == INSTALL_INSTALLING) {
    find_online_status_text->SetLabel(_LABEL_("found online installing"));
    return; // still running
  }
  // thread has finished
  auto_installing = false;
  if (state == INSTALL_SUCCESS) {
    try {
      stylesheet = StyleSheet::byGameAndName(game, failed_name);
      EndModal(wxID_OK);
      return;
    } catch (const Error& e) {
      handle_error(e);
      // fall through to failure UI reset below
    }
  } else {
    unique_ptr<Error> error;
    {
      wxMutexLocker l(install_mutex);
      error = std::move(install_error);
    }
    if (error) handle_error(*error);
  }
  find_online_status_text->SetLabel(_LABEL_("found online fail"));
  ok_button->Enable(true);
  cancel_button->Enable(true);
  stylesheet_list->Enable(true);
  stylesheet_filter->Enable(true);
}

void SelectStyleSheetWindow::OnOK(wxCommandEvent&) {
  done();
}

void SelectStyleSheetWindow::done() {
  try {
    stylesheet = stylesheet_list->getSelection<StyleSheet>();
    EndModal(wxID_OK);
  } catch (const Error& e) {
    handle_error(e);
    EndModal(wxID_CANCEL);
  }
}

void SelectStyleSheetWindow::onUpdateUI(wxUpdateUIEvent& ev) {
  switch (ev.GetId()) {
    case wxID_OK:
      if (!auto_installing) ev.Enable(stylesheet_list->hasSelection());
      break;
  }
}

void SelectStyleSheetWindow::onIdle(wxIdleEvent& ev) {
  if (searching_online && !auto_installing) {
    // still downloading installer list?
    if (!downloadable_installers.download()) {
      ev.RequestMore();
      return;
    }
    // installer list ready, prepare and start installer thread, return
    tryAutoInstall();
  }
  if (auto_installing) {
    pollAutoInstall();
    if (auto_installing) ev.RequestMore();
  }
}

void SelectStyleSheetWindow::onClose(wxCloseEvent& ev) {
  if (auto_installing) {
    ev.Veto(); // installer thread holds a pointer to this window, don't let it be destroyed
    return;
  }
  ev.Skip();
}

void SelectStyleSheetWindow::onCharHook(wxKeyEvent& ev) {
  if (ev.GetKeyCode() == WXK_ESCAPE && auto_installing) {
    return; // swallow Escape while installer thread is running
  }
  ev.Skip();
}

BEGIN_EVENT_TABLE(SelectStyleSheetWindow, wxDialog)
  EVT_GALLERY_SELECT  (ID_STYLESHEET_LIST,     SelectStyleSheetWindow::onStyleSheetSelect)
  EVT_GALLERY_ACTIVATE(ID_STYLESHEET_LIST,     SelectStyleSheetWindow::onStyleSheetActivate)
  EVT_COMMAND_RANGE   (ID_STYLESHEET_FILTER, ID_STYLESHEET_FILTER, wxEVT_COMMAND_TEXT_UPDATED, SelectStyleSheetWindow::onStylesheetFilterUpdate)
  EVT_BUTTON          (ID_DOWNLOAD_STYLESHEET, SelectStyleSheetWindow::onFindOnline)
  EVT_BUTTON          (wxID_OK,                SelectStyleSheetWindow::OnOK)
  EVT_UPDATE_UI       (wxID_ANY,               SelectStyleSheetWindow::onUpdateUI)
  EVT_IDLE            (                        SelectStyleSheetWindow::onIdle)
  EVT_CLOSE           (                        SelectStyleSheetWindow::onClose)
  EVT_CHAR_HOOK       (                        SelectStyleSheetWindow::onCharHook)
END_EVENT_TABLE  ()

