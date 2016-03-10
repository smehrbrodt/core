/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef INCLUDED_SFX2_SOURCE_SIDEBAR_SIDEBARTABBAR_HXX
#define INCLUDED_SFX2_SOURCE_SIDEBAR_SIDEBARTABBAR_HXX

#include <sfx2/abstractbar/TabBar.hxx>

namespace sfx2 { namespace abstractbar {
class TabBar;
}}

namespace sfx2 { namespace sidebar {

class SFX2_DLLPUBLIC SidebarTabBar
    : public sfx2::abstractbar::TabBar
{
public:
    SidebarTabBar (
        vcl::Window* pParentWindow,
        const css::uno::Reference<css::frame::XFrame>& rxFrame,
        const ::std::function<void (const ::rtl::OUString&rsDeckId)>& rDeckActivationFunctor,
        const PopupMenuProvider& rPopupMenuProvider,
        sfx2::abstractbar::IController* rParentController);
    virtual ~SidebarTabBar();

    virtual void Paint (vcl::RenderContext& /*rRenderContext*/, const Rectangle& rUpdateArea) override;
    static sal_Int32 GetDefaultWidth();

private:
    virtual void Layout() override;
    virtual void UpdateTabs() override;
    virtual VclPtr<RadioButton> CreateTabItem (const sfx2::abstractbar::DeckDescriptor& rDeckDescriptor) override;
    Image GetItemImage (const sfx2::abstractbar::DeckDescriptor& rDeskDescriptor) const;
    sal_Int32 mnMenuSeparatorY;
};


} } // end of namespace sfx2::sidebar

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
