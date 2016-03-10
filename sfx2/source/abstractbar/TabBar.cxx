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

#include <sfx2/abstractbar/TabBar.hxx>
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

namespace sfx2 { namespace abstractbar {

TabBar::TabBar(vcl::Window* pParentWindow,
               const Reference<frame::XFrame>& rxFrame,
               const std::function<void (const OUString&)>& rDeckActivationFunctor,
               sfx2::abstractbar::IController* rParentAbstractbarController
              )
    : Window(pParentWindow, WB_DIALOGCONTROL),
      mpParentWindow(pParentWindow),
      mxFrame(rxFrame),
      maItems(),
      maDeckActivationFunctor(rDeckActivationFunctor),
      pParentAbstractbarController(rParentAbstractbarController)
{

    SetBackground(Theme::GetPaint(Theme::Paint_TabBarBackground).GetWallpaper());

#ifdef DEBUG
    SetText(OUString("TabBar"));
#endif
}

TabBar::~TabBar()
{
    disposeOnce();
}

void TabBar::dispose()
{
    for(ItemContainer::iterator
            iItem(maItems.begin()), iEnd(maItems.end());
        iItem!=iEnd;
        ++iItem)
        iItem->mpButton.disposeAndClear();
    maItems.clear();
    vcl::Window::dispose();
}

void TabBar::SetDecks(const ResourceManager::DeckContextDescriptorContainer& rDecks)
{
    // Remove the current buttons.
    {
        for(ItemContainer::iterator iItem(maItems.begin()); iItem != maItems.end(); ++iItem)
        {
            iItem->mpButton.disposeAndClear();
        }
        maItems.clear();
    }
    maItems.resize(rDecks.size());
    sal_Int32 nIndex (0);
    for (ResourceManager::DeckContextDescriptorContainer::const_iterator
             iDeck(rDecks.begin()); iDeck != rDecks.end(); ++iDeck)
    {
        const DeckDescriptor* pDescriptor = pParentAbstractbarController->GetResourceManager()->GetDeckDescriptor(iDeck->msId);
        if (pDescriptor == nullptr)
        {
            OSL_ASSERT(pDescriptor!=nullptr);
            continue;
        }

        Item& rItem (maItems[nIndex++]);
        rItem.msDeckId = pDescriptor->msId;
        rItem.mpButton.disposeAndClear();
        rItem.mpButton = CreateTabItem(*pDescriptor);
        rItem.mpButton->SetClickHdl(LINK(&rItem, TabBar::Item, HandleClick));
        rItem.maDeckActivationFunctor = maDeckActivationFunctor;
        rItem.mbIsHidden = ! pDescriptor->mbIsEnabled;
        rItem.mbIsHiddenByDefault = rItem.mbIsHidden; // the default is the state while creating

        rItem.mpButton->Enable(iDeck->mbIsEnabled);
    }

    UpdateTabs();
    Layout();
}

void TabBar::HighlightDeck (const OUString& rsDeckId)
{
    for (ItemContainer::iterator iItem(maItems.begin()); iItem != maItems.end(); ++iItem)
    {
        if (iItem->msDeckId.equals(rsDeckId))
            iItem->mpButton->Check();
        else
            iItem->mpButton->Check(false);
    }
}

void TabBar::RemoveDeckHighlight ()
{
    for (ItemContainer::iterator iItem(maItems.begin()),iEnd(maItems.end());
         iItem!=iEnd;
         ++iItem)
    {
        iItem->mpButton->Check(false);
    }
}

void TabBar::DataChanged (const DataChangedEvent& rDataChangedEvent)
{
    SetBackground(Theme::GetPaint(Theme::Paint_TabBarBackground).GetWallpaper());
    UpdateTabs();

    Window::DataChanged(rDataChangedEvent);
}

bool TabBar::Notify (NotifyEvent& rEvent)
{
    if(rEvent.GetType() == MouseNotifyEvent::COMMAND)
    {
        const CommandEvent& rCommandEvent = *rEvent.GetCommandEvent();
        if(rCommandEvent.GetCommand() == CommandEventId::Wheel)
        {
            const CommandWheelData* pData = rCommandEvent.GetWheelData();
            if(!pData->GetModifier() && (pData->GetMode() == CommandWheelMode::SCROLL))
            {
                auto pItem = std::find_if(maItems.begin(), maItems.end(),
                    [] (Item const& rItem) { return rItem.mpButton->IsChecked(); });
                if(pItem == maItems.end())
                    return true;
                if(pData->GetNotchDelta()<0)
                {
                    if(pItem+1 == maItems.end())
                        return true;
                    ++pItem;
                }
                else
                {
                    if(pItem == maItems.begin())
                        return true;
                    --pItem;
                }
                try
                {
                    pItem->maDeckActivationFunctor(pItem->msDeckId);
                }
                catch(const css::uno::Exception&) {};
                return true;
            }
        }
    }
    return false;
}

IMPL_LINK_NOARG_TYPED(TabBar::Item, HandleClick, Button*, void)
{
    try
    {
        maDeckActivationFunctor(msDeckId);
    }
    catch(const css::uno::Exception&)
    {} // workaround for #i123198#
}

const ::rtl::OUString TabBar::GetDeckIdForIndex (const sal_Int32 nIndex) const
{
    if (nIndex<0 || static_cast<size_t>(nIndex)>=maItems.size())
        throw RuntimeException();
    else
        return maItems[nIndex].msDeckId;
}

void TabBar::ToggleHideFlag (const sal_Int32 nIndex)
{
    if (nIndex<0 || static_cast<size_t>(nIndex) >= maItems.size())
        throw RuntimeException();
    else
    {
        maItems[nIndex].mbIsHidden = ! maItems[nIndex].mbIsHidden;

        DeckDescriptor* pDeckDescriptor = pParentAbstractbarController->GetResourceManager()->GetDeckDescriptor(maItems[nIndex].msDeckId);
        if (pDeckDescriptor)
            pDeckDescriptor->mbIsEnabled = ! maItems[nIndex].mbIsHidden;

        Layout();
    }
}

void TabBar::RestoreHideFlags()
{
    bool bNeedsLayout(false);
    for (ItemContainer::iterator iItem(maItems.begin()); iItem != maItems.end(); ++iItem)
    {
        if (iItem->mbIsHidden != iItem->mbIsHiddenByDefault)
        {
            iItem->mbIsHidden = iItem->mbIsHiddenByDefault;
            bNeedsLayout = true;

            DeckDescriptor* pDeckDescriptor = pParentAbstractbarController->GetResourceManager()->GetDeckDescriptor(iItem->msDeckId);
            if (pDeckDescriptor)
                pDeckDescriptor->mbIsEnabled = ! iItem->mbIsHidden;

        }
    }
    if (bNeedsLayout)
        Layout();
}

} } // end of namespace sfx2::abstractbar

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
