#pragma once

#include "TextField.h"

namespace vive {
namespace ui {

//! a graphical Element for browsing for a file

/** \note QFileBrowser
    \note vvTUIFileBrowserButton */
class VIVE_UI_EXPORT FileBrowser: public TextField {

 public:
   enum UpdateMask: UpdateMaskType
   {
       UpdateFilter = 0x200,
   };

   FileBrowser(Group *parent, const std::string &name, bool save=false);
   FileBrowser(const std::string &name, Owner *owner, bool save=false);
   virtual ~FileBrowser();

   void setFilter(const std::string &filter);
   std::string filter() const;

    void update(UpdateMaskType mask) const override;

    bool forSaving() const;

 protected:
    std::string m_filter;
    bool m_save = false;
};

}
}
