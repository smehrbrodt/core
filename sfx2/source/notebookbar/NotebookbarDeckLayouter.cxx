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

#include <sfx2/notebookbar/NotebookbarDeckLayouter.hxx>
#include <sfx2/abstractbar/Theme.hxx>
#include <sfx2/abstractbar/Panel.hxx>
#include <sfx2/abstractbar/PanelTitleBar.hxx>
#include <sfx2/abstractbar/Deck.hxx>

#include <vcl/window.hxx>
#include <vcl/scrbar.hxx>

using namespace css;
using namespace css::uno;
using namespace sfx2::abstractbar;

namespace sfx2 { namespace notebookbar {

namespace {
    static const sal_Int32 MinimalPanelHeight (25);

    class LayoutItem
    {
    public:
        VclPtr<Panel> mpPanel;
        css::ui::LayoutSize maLayoutSize;
        sal_Int32 mnDistributedHeight;
        sal_Int32 mnWeight;
        sal_Int32 mnPanelIndex;
        bool mbShowTitleBar;

        LayoutItem()
            : mpPanel(),maLayoutSize(0,0,0),mnDistributedHeight(0),mnWeight(0),mnPanelIndex(0),mbShowTitleBar(true)
        {}
    };
    Rectangle LayoutPanels (
        const Rectangle& rContentArea,
        sal_Int32& rMinimalWidth,
        ::std::vector<LayoutItem>& rLayoutItems,
        vcl::Window& rScrollClipWindow,
        vcl::Window& rScrollContainer);
    void GetRequestedSizes (
        ::std::vector<LayoutItem>& rLayoutItem,
        sal_Int32& rAvailableWidth,
        sal_Int32& rMinimalWidth,
        const Rectangle& rContentBox);
    sal_Int32 PlacePanels (
        ::std::vector<LayoutItem>& rLayoutItems,
        const sal_Int32 nHeight,
        vcl::Window& rScrollContainer);
    void UpdateFiller (
        vcl::Window& rFiller,
        const Rectangle& rBox);
}

#define IterateLayoutItems(iterator_name,container)                     \
    for(::std::vector<LayoutItem>::iterator                             \
                   iterator_name(container.begin()),                    \
                   iEnd(container.end());                               \
        iterator_name!=iEnd;                                            \
        ++iterator_name)

void NotebookbarDeckLayouter::LayoutDeck (
    const Rectangle& rContentArea,
    sal_Int32& rMinimalWidth,
    SharedPanelContainer& rPanels,
    vcl::Window& rScrollClipWindow,
    vcl::Window& rScrollContainer,
    vcl::Window& rFiller)
{
    if (rContentArea.GetWidth()<=0 || rContentArea.GetHeight()<=0)
        return;
    Rectangle aBox;

    if ( ! rPanels.empty())
    {
        // Prepare the layout item container.
        ::std::vector<LayoutItem> aLayoutItems;
        aLayoutItems.resize(rPanels.size());
        for (sal_Int32 nIndex(0),nCount(rPanels.size()); nIndex<nCount; ++nIndex)
        {
            aLayoutItems[nIndex].mpPanel = rPanels[nIndex];
            aLayoutItems[nIndex].mnPanelIndex = nIndex;
        }
        aBox = LayoutPanels(
            rContentArea,
            rMinimalWidth,
            aLayoutItems,
            rScrollClipWindow,
            rScrollContainer);
    }
    UpdateFiller(rFiller, aBox);
}

namespace {

Rectangle LayoutPanels (
    const Rectangle& rContentArea,
    sal_Int32& rMinimalWidth,
    ::std::vector<LayoutItem>& rLayoutItems,
    vcl::Window& rScrollClipWindow,
    vcl::Window& rScrollContainer)
{
    Rectangle aBox (rContentArea);

    const sal_Int32 nWidth (aBox.GetWidth());
    const sal_Int32 nHeight (aBox.GetHeight());

    // Get the requested heights of the panels and the available
    // height that is left when all panel titles and separators are
    // taken into account.
    sal_Int32 nAvailableWidth (aBox.GetWidth());
    GetRequestedSizes(rLayoutItems, nAvailableWidth, rMinimalWidth, aBox);

    // Analyze the requested heights.
    // Determine the height that is available for panel content
    // and count the different layouts.
    sal_Int32 nTotalPreferredHeight (0);
    sal_Int32 nTotalMinimumHeight (0);
    IterateLayoutItems(iItem,rLayoutItems)
    {
        nTotalMinimumHeight += iItem->maLayoutSize.Minimum;
        nTotalPreferredHeight += iItem->maLayoutSize.Preferred;
    }

    // Set position and size of the mpScrollClipWindow to the available
    // size.  Its child, the mpScrollContainer, may have a bigger
    // height.
    rScrollClipWindow.setPosSizePixel(aBox.Left(), aBox.Top(), aBox.GetWidth(), aBox.GetHeight());
    rScrollContainer.setPosSizePixel(
        0,
        0,
        nWidth,
        nHeight);

    const sal_Int32 nUsedWidth (PlacePanels(rLayoutItems, nHeight, rScrollContainer));
    aBox.Right() += nUsedWidth;
    return aBox;
}

sal_Int32 PlacePanels (
    ::std::vector<LayoutItem>& rLayoutItems,
    const sal_Int32 nHeight,
    vcl::Window& rScrollContainer)
{
    ::std::vector<sal_Int32> aSeparators;
    const sal_Int32 nDeckSeparatorWidth (Theme::GetInteger(Theme::Int_DeckSeparatorHeight));
    sal_Int32 nX (0);



    // Assign heights and places.
    IterateLayoutItems(iItem,rLayoutItems)
    {
        if (!iItem->mpPanel)
            continue;

        Panel& rPanel (*iItem->mpPanel);
        const sal_Int32 nPanelTitleBarHeight (Theme::GetInteger(Theme::Int_PanelTitleBarHeight) * rPanel.GetDPIScaleFactor());
        sal_Int32 nPanelHeight = nHeight - nPanelTitleBarHeight;

        // Determine the height of the panel depending on layout
        // mode and distributed heights.
        sal_Int32 nPanelWidth (300);
        //nPanelWidth = iItem->maLayoutSize.Minimum;

        // Place the panel.
        rPanel.setPosSizePixel(nX, 0, nPanelWidth, nPanelHeight);
        rPanel.Show();

        //TODO-SM Place the title bar at the bottom
        PanelTitleBar* pTitleBar = rPanel.GetTitleBar();
        if (pTitleBar != nullptr)
        {
            pTitleBar->setPosSizePixel(nX, nHeight, nPanelWidth, nPanelTitleBarHeight);
            pTitleBar->Show();
        }

        nX += nPanelWidth;
        // Separator next to the panel
        aSeparators.push_back(nX);
        nX += nDeckSeparatorWidth;
    }

    Deck::ScrollContainerWindow* pScrollContainerWindow = dynamic_cast<Deck::ScrollContainerWindow*>(&rScrollContainer);
    if (pScrollContainerWindow != nullptr)
        pScrollContainerWindow->SetSeparators(aSeparators);

    return nX;
}

void GetRequestedSizes (
    ::std::vector<LayoutItem>& rLayoutItems,
    sal_Int32& rAvailableWidth,
    sal_Int32& rMinimalWidth,
    const Rectangle& rContentBox)
{
    rAvailableWidth = rContentBox.GetWidth();

    const sal_Int32 nDeckSeparatorWidth (Theme::GetInteger(Theme::Int_DeckSeparatorHeight));

    IterateLayoutItems(iItem,rLayoutItems)
    {
        ui::LayoutSize aLayoutSize (ui::LayoutSize(0,0,0));
        if (iItem->mpPanel != nullptr)
        {
            if (rLayoutItems.size() == 1
                && iItem->mpPanel->IsTitleBarOptional())
            {
                // There is only one panel and its title bar is
                // optional => hide it.
                rAvailableWidth -= nDeckSeparatorWidth;
                iItem->mbShowTitleBar = false;
            }
            else
            {
                // Show the title bar and a separator above and below the title bar.
                /*const sal_Int32 nPanelTitleBarHeight (Theme::GetInteger(Theme::Int_PanelTitleBarHeight) * iItem->mpPanel->GetDPIScaleFactor());

                rAvailableWidth -= nPanelTitleBarHeight;
                rAvailableWidth -= nDeckSeparatorWidth;*/
            }

            if (iItem->mpPanel->IsExpanded())
            {
                sal_Int32 nWidth = 200;
                if (nWidth > rMinimalWidth) {
                    rMinimalWidth = nWidth;
                    ui::LayoutSize(rMinimalWidth, -1, 0);
                }
                else
                    aLayoutSize = ui::LayoutSize(MinimalPanelHeight, -1, 0);
            }
        }
        iItem->maLayoutSize = aLayoutSize;
    }
}

void UpdateFiller (
    vcl::Window& rFiller,
    const Rectangle& rBox)
{
    if (rBox.GetHeight() > 0)
    {
        // Show the filler.
        rFiller.SetBackground(Theme::GetPaint(Theme::Paint_PanelBackground).GetWallpaper());
        rFiller.SetPosSizePixel(rBox.TopLeft(), rBox.GetSize());
        rFiller.Show();
    }
    else
    {
        // Hide the filler.
        rFiller.Hide();
    }
}

}

} } // end of namespace sfx2::abstractbar

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
