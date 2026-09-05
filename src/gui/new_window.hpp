//+----------------------------------------------------------------------------+
//| Description:  Magic Set Editor - Program to make card games                |
//| Copyright:    (C) Twan van Laarhoven and the other MSE developers          |
//| License:      GNU General Public License 2 or later (see file COPYING)     |
//+----------------------------------------------------------------------------+

#pragma once

// ----------------------------------------------------------------------------- : Includes

#include <util/prec.hpp>
#include <gui/control/filter_ctrl.hpp>

class PackageList;
class Game;
DECLARE_POINTER_TYPE(Set);
DECLARE_POINTER_TYPE(StyleSheet);

// ----------------------------------------------------------------------------- : NewSetWindow

/// Show the new set window, return the new set, if any
SetP new_set_window(Window* parent);

/// "Create a new set" dialog. First select game, then matching style.
class NewSetWindow : public wxDialog {
public:
  /// The newly created set, if any
  SetP set;
  
  NewSetWindow(Window* parent);
  
  // --------------------------------------------------- : data
private:
  DECLARE_EVENT_TABLE();

  // gui items
  PackageList*  game_list, *stylesheet_list;
  
  FilterCtrl* game_filter;
  String game_filter_value;

  FilterCtrl* stylesheet_filter;
  String stylesheet_filter_value;
    
  // --------------------------------------------------- : events
  
  void onGameSelect  (wxCommandEvent&);
  
  void onStyleSheetSelect  (wxCommandEvent&);
  void onStyleSheetActivate(wxCommandEvent&);
  void onStylesheetFilterUpdate(wxCommandEvent&);
  void onGameFilterUpdate(wxCommandEvent&);
    
  virtual void OnOK(wxCommandEvent&);
  
  void onUpdateUI(wxUpdateUIEvent&);
  void onIdle(wxIdleEvent&);
  
  // we are done, close the window
  void done();
};

// ----------------------------------------------------------------------------- : SelectStyleSheetWindow
// very similair, so in the same file

/// Show the select stylesheet window, return the selected stylesheet, if any
StyleSheetP select_stylesheet(const Game& game, const String& failed_name, const String& failed_dep = String());

/// "Create a new set" dialog. First select game, then matching style.
class SelectStyleSheetWindow : public wxDialog {
public:
  /// The selected stylesheet, if any
  StyleSheetP stylesheet;
  
  SelectStyleSheetWindow(Window* parent, const Game& game, const String& failed_name, const String& failed_dep = String());
  
  // --------------------------------------------------- : data
private:
  DECLARE_EVENT_TABLE();
  
  const Game& game;
  
  // gui items
  PackageList*  stylesheet_list;

  FilterCtrl*   stylesheet_filter;
  String        stylesheet_filter_value;

  wxButton*     ok_button, *cancel_button;

  wxButton*     find_online_button;
  wxStaticText* find_online_status_text;
  bool          searching_online = false;
  bool          auto_installing  = false;
  String        failed_name;
  String        failed_dep; // non-empty if failed_name was found but this dependency of it wasn't

  // --------------------------------------------------- : background install
  
  // Download+install on a background thread
  class InstallThread;

  enum InstallState { INSTALL_DOWNLOADING, INSTALL_INSTALLING, INSTALL_SUCCESS, INSTALL_FAILURE };

  wxMutex           install_mutex;
  InstallState      install_state = INSTALL_DOWNLOADING; // guarded by install_mutex
  unique_ptr<Error> install_error;                       // guarded by install_mutex

  // --------------------------------------------------- : events
  
  void onStyleSheetSelect  (wxCommandEvent&);
  void onStyleSheetActivate(wxCommandEvent&);
  void onStylesheetFilterUpdate(wxCommandEvent&);
  void onFindOnline(wxCommandEvent&);
  virtual void OnOK(wxCommandEvent&);
  
  void onUpdateUI(wxUpdateUIEvent&);
  void onIdle(wxIdleEvent&);
  void onClose(wxCloseEvent&);
  void onCharHook(wxKeyEvent&);

  void tryAutoInstall();
  void pollAutoInstall();

  void done();
};

