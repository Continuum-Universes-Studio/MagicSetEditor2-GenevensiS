//+----------------------------------------------------------------------------+
//| Description:  Magic Set Editor - Program to make card games                |
//| Copyright:    (C) Twan van Laarhoven and the other MSE developers          |
//| License:      GNU General Public License 2 or later (see file COPYING)     |
//+----------------------------------------------------------------------------+

#pragma once

// ----------------------------------------------------------------------------- : Includes

#include <util/prec.hpp>
#include <gui/control/tree_list.hpp>
#include <data/installer.hpp>

// ----------------------------------------------------------------------------- : PackageUpdateList

/// A list of installed and downloadable packages
class PackageUpdateList : public TreeList {
private:
  class TreeItem;
public:
  typedef intrusive_ptr<TreeItem> TreeItemP;

  PackageUpdateList(Window* parent, const InstallablePackages& packages, bool show_only_installable, int id = wxID_ANY);
  ~PackageUpdateList();

  inline InstallablePackageP getSelectedPackage() const {
    TreeItem* ti = getSelectedItem();
    return ti ? ti->package : InstallablePackageP();
  }

  inline bool selectionIsGroup() const {
    TreeItem* ti = getSelectedItem();
    return ti && !ti->package;
  }

  inline void forEachSelectedPackage(const std::function<void(const InstallablePackageP&)>& fn) const {
    forEachPackage(getSelectedItem(), fn);
  }

  inline bool anySelectedPackage(const std::function<bool(const InstallablePackageP&)>& predicate) const {
    return anyPackage(getSelectedItem(), predicate);
  }

  inline bool allSelectedPackages(const std::function<bool(const InstallablePackageP&)>& predicate) const {
    return allPackages(getSelectedItem(), predicate);
  }

protected:
  // overridden methods from TreeList
  void initItems() override;
  void drawItem(DC& dc, size_t index, size_t column, int x, int y, bool selected) const override;

  size_t columnCount() const override { return 3; }
  String columnText(size_t column) const override;
  int    columnWidth(size_t column) const override;
  
private:
  inline TreeItem* getSelectedItem() const {
    return selection >= items.size() ? nullptr : static_pointer_cast<TreeItem>(items[selection]).get();
  }
  inline void forEachPackage(const TreeItem* item, const std::function<void(const InstallablePackageP&)>& fn) const {
    if (!item) return;
    if (item->package) {
      fn(item->package);
    }
    for (const auto& child : item->children) {
      forEachPackage(child.get(), fn);
    }
  }
  inline bool anyPackage(const TreeItem* item, const std::function<bool(const InstallablePackageP&)>& predicate) const {
    if (!item) return false;
    if (item->package && predicate(item->package)) {
      return true;
    }
    for (const auto& child : item->children) {
      if (anyPackage(child.get(), predicate)) {
        return true;
      }
    }
    return false;
  }
  inline bool allPackages(const TreeItem* item, const std::function<bool(const InstallablePackageP&)>& predicate) const {
    if (!item) return false;
    if (item->package && !predicate(item->package)) {
      return false;
    }
    for (const auto& child : item->children) {
      if (!allPackages(child.get(), predicate)) {
        return false;
      }
    }
    return true;
  }

  /// The list of packages we are displaying
  const InstallablePackages& packages;
  /// Show only packages with an installer?
  bool show_only_installable;
  
  class TreeItem : public Item {
  public:
    TreeItem() : position_type(TYPE_OTHER), position_hint(1000000) {}
    String label;
    vector<TreeItemP> children;
    InstallablePackageP package;
    Bitmap icon, icon_grey;
    // positioning
    enum PackageType {
      TYPE_PROG,
      TYPE_LOCALE,
      TYPE_GAME,
      TYPE_STYLESHEET,
      TYPE_EXPORT_TEMPLATE,
      TYPE_IMPORT_TEMPLATE,
      TYPE_SYMBOL_FONT,
      TYPE_INCLUDE,
      TYPE_FONT,
      TYPE_OTHER,
    }   position_type;
    int position_hint;
    
    void add(const InstallablePackageP& package, const String& path, int level = -1);
    void toItems(vector<TreeList::ItemP>& items);
    void setIcon(const Image& image);
    bool highlight() const;
    
    static PackageType package_type(const PackageDescription& desc);

  };

  bool CheckChildrenForUpdates(const TreeItem& ti) const;

  friend class PackageIconRequest;
};

