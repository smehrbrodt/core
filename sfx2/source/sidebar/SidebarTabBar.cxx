/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Licensed to the Apache Software Foundation (ASF) under one or more
 *   contributor license agreements. See the NOTICE file distributed
 *   with this work for additional information regarding copyright
 *   ownership. The ASF licenses this file to you under the Apache
 *   License, Version 2.0 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.apache.org/licenses/LICENSE-2.0 .
 */

#include <sfx2/sidebar/SidebarTabBar.hxx>
#include <sfx2/abstractbar/ControlFactory.hxx>
#include <sfx2/abstractbar/DeckDescriptor.hxx>
#include <sfx2/abstractbar/Paint.hxx>
#include <sfx2/abstractbar/Theme.hxx>
#include <sfx2/abstractbar/Tools.hxx>
#include <sfx2/abstractbar/FocusManager.hxx>
#include <sfx2/abstractbar/IController.hxx>

#include <sfx2/sfxresid.hxx>
#include <sfx2/sidebar/Sidebar.hrc>

#include <vcl/gradient.hxx>
#include <vcl/image.hxx>
#include <vcl/wrkwin.hxx>
#include <comphelper/processfactory.hxx>
#include <tools/svborder.hxx>

#include <com/sun/star/graphic/XGraphicProvider.hpp>

using namespace css;
using namespace css::uno;
using namespace sfx2::abstractbar;

namespace sfx2 { namespace sidebar {

SidebarTabBar::SidebarTabBar(vcl::Window* pParentWindow,
               const Reference<frame::XFrame>& rxFrame,
               const std::function<void (const OUString&)>& rDeckActivationFunctor,
               const PopupMenuProvider& rPopupMenuProvider,
               sfx2::abstractbar::IController* rParentAbstractbarController
              )
    : TabBar(pParentWindow, rxFrame, rDeckActivationFunctor, rPopupMenuProvider, rParentAbstractbarController)
{
    Layout();
}

SidebarTabBar::~SidebarTabBar() {

}

void SidebarTabBar::Paint(vcl::RenderContext& rRenderContext, const Rectangle& rUpdateArea)
{
    Window::Paint(rRenderContext, rUpdateArea);

    const sal_Int32 nHorizontalPadding(Theme::GetInteger(Theme::Int_TabMenuSeparatorPadding));
    rRenderContext.SetLineColor(Theme::GetColor(Theme::Color_TabMenuSeparator));
    rRenderContext.DrawLine(Point(nHorizontalPadding, mnMenuSeparatorY),
                            Point(GetSizePixel().Width() - nHorizontalPadding, mnMenuSeparatorY));
}

void SidebarTabBar::Layout()
{
    const SvBorder aPadding (
        Theme::GetInteger(Theme::Int_TabBarLeftPadding),
        Theme::GetInteger(Theme::Int_TabBarTopPadding),
        Theme::GetInteger(Theme::Int_TabBarRightPadding),
        Theme::GetInteger(Theme::Int_TabBarBottomPadding));
    sal_Int32 nX (aPadding.Top());
    sal_Int32 nY (aPadding.Left());
    const Size aTabItemSize (
        Theme::GetInteger(Theme::Int_TabItemWidth) * GetDPIScaleFactor(),
        Theme::GetInteger(Theme::Int_TabItemHeight) * GetDPIScaleFactor());

    // Place the menu button and the separator.
    if (mpMenuButton != nullptr)
    {
        mpMenuButton->SetPosSizePixel(
            Point(nX,nY),
            aTabItemSize);
        mpMenuButton->Show();
        nY += mpMenuButton->GetSizePixel().Height() + 1 + Theme::GetInteger(Theme::Int_TabMenuPadding);
        mnMenuSeparatorY = nY - Theme::GetInteger(Theme::Int_TabMenuPadding)/2 - 1;
    }

    // Place the deck selection buttons.
    for(ItemContainer::const_iterator
            iItem(maItems.begin()), iEnd(maItems.end());
        iItem!=iEnd;
        ++iItem)
    {
        Button& rButton (*iItem->mpButton);
        rButton.Show( ! iItem->mbIsHidden);

        if (iItem->mbIsHidden)
            continue;

        // Place and size the icon.
        rButton.SetPosSizePixel(
            Point(nX,nY),
            aTabItemSize);
        rButton.Show();

        nY += rButton.GetSizePixel().Height() + 1 + aPadding.Bottom();
    }
    Invalidate();
}

sal_Int32 SidebarTabBar::GetDefaultWidth()
{
    return Theme::GetInteger(Theme::Int_TabItemWidth)
        + Theme::GetInteger(Theme::Int_TabBarLeftPadding)
        + Theme::GetInteger(Theme::Int_TabBarRightPadding);
}

VclPtr<RadioButton> SidebarTabBar::CreateTabItem(const DeckDescriptor& rDeckDescriptor)
{
    VclPtr<RadioButton> pItem = ControlFactory::CreateTabImageItem(this);
    pItem->SetAccessibleName(rDeckDescriptor.msTitle);
    pItem->SetAccessibleDescription(rDeckDescriptor.msHelpText);
    pItem->SetHelpText(rDeckDescriptor.msHelpText);
    pItem->SetQuickHelpText(rDeckDescriptor.msHelpText);
    return pItem;
}

void SidebarTabBar::UpdateTabs()
{
    Image aImage = Theme::GetImage(Theme::Image_TabBarMenu);
    if ( mpMenuButton->GetDPIScaleFactor() > 1 )
    {
        BitmapEx b = aImage.GetBitmapEx();
        b.Scale(mpMenuButton->GetDPIScaleFactor(), mpMenuButton->GetDPIScaleFactor(), BmpScaleFlag::Fast);
        aImage = Image(b);
    }
    mpMenuButton->SetModeImage(aImage);

    for(ItemContainer::const_iterator
            iItem(maItems.begin()), iEnd(maItems.end());
        iItem!=iEnd;
        ++iItem)
    {
        const DeckDescriptor* pDeckDescriptor = pParentAbstractbarController->GetResourceManager()->GetDeckDescriptor(iItem->msDeckId);

        if (pDeckDescriptor != nullptr)
        {
            aImage = GetItemImage(*pDeckDescriptor);
            if ( mpMenuButton->GetDPIScaleFactor() > 1 )
            {
                BitmapEx b = aImage.GetBitmapEx();
                b.Scale(mpMenuButton->GetDPIScaleFactor(), mpMenuButton->GetDPIScaleFactor(), BmpScaleFlag::Fast);
                aImage = Image(b);
            }

            iItem->mpButton->SetModeImage(aImage);
        }
    }

    Invalidate();
}

Image SidebarTabBar::GetItemImage(const DeckDescriptor& rDeckDescriptor) const
{
    return Tools::GetImage(
        rDeckDescriptor.msIconURL,
        rDeckDescriptor.msHighContrastIconURL,
        mxFrame);
}

} } // end of namespace sfx2::abstractbar

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
