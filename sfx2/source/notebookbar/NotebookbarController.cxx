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
#include <sfx2/notebookbar/NotebookbarController.hxx>
#include <sfx2/abstractbar/IController.hxx>
#include <sfx2/abstractbar/DeckTitleBar.hxx>
#include <sfx2/abstractbar/Panel.hxx>
#include <sfx2/abstractbar/PanelTitleBar.hxx>
#include <sfx2/abstractbar/SidebarResource.hxx>
#include <sfx2/abstractbar/TabBar.hxx>
#include <sfx2/abstractbar/Theme.hxx>
#include <sfx2/notebookbar/NotebookbarTabBar.hxx>
#include <sfx2/notebookbar/NotebookbarChildWindow.hxx>
#include <sfx2/abstractbar/Tools.hxx>
#include <sfx2/notebookbar/NotebookbarDockingWindow.hxx>
#include <sfx2/notebookbar/NotebookbarDeck.hxx>
#include <sfx2/abstractbar/Context.hxx>
#include <sfx2/abstractbar/ContextList.hxx>
#include <sfx2/notebookbar/NotebookbarResourceManager.hxx>

#include <sfx2/sfxresid.hxx>
#include <sfx2/sfxsids.hrc>
#include <sfx2/titledockwin.hxx>
#include "sfxlocal.hrc"
#include <framework/ContextChangeEventMultiplexerTunnel.hxx>
#include <vcl/floatwin.hxx>
#include <vcl/fixed.hxx>
#include "splitwin.hxx"
#include <svl/smplhint.hxx>
#include <tools/link.hxx>
#include <toolkit/helper/vclunohelper.hxx>
#include <comphelper/processfactory.hxx>
#include <comphelper/namedvaluecollection.hxx>

#include <com/sun/star/frame/XDispatchProvider.hpp>
#include <com/sun/star/lang/XInitialization.hpp>
#include <com/sun/star/ui/ContextChangeEventMultiplexer.hpp>
#include <com/sun/star/ui/ContextChangeEventObject.hpp>
#include <com/sun/star/ui/theUIElementFactoryManager.hpp>
#include <com/sun/star/util/XURLTransformer.hpp>
#include <com/sun/star/util/URL.hpp>
#include <com/sun/star/rendering/XSpriteCanvas.hpp>


using namespace css;
using namespace css::uno;
using namespace sfx2::abstractbar;

namespace
{
    const static char gsReadOnlyCommandName[] = ".uno:EditDoc";
    const static char gsHideNotebookbarCommandName[] = ".uno:Notebookbar";
}

namespace sfx2 { namespace notebookbar {

namespace {
    enum MenuId
    {
        MID_CUSTOMIZATION = 1,
        MID_RESTORE_DEFAULT,
        MID_FIRST_PANEL,
        MID_FIRST_HIDE = 1000
    };

    /** When in doubt, show this deck.
    */
    static const char gsDefaultDeckId[] = "PropertyDeck";
}

NotebookbarController::NotebookbarController (
    NotebookbarDockingWindow* pParentWindow,
    const css::uno::Reference<css::frame::XFrame>& rxFrame)
    : NotebookbarControllerInterfaceBase(m_aMutex),
      mpCurrentDeck(),
      mpParentWindow(pParentWindow),
      mpTabBar(VclPtr<NotebookbarTabBar>::Create(
              mpParentWindow,
              rxFrame,
              [this](const ::rtl::OUString& rsDeckId) { return this->OpenThenSwitchToDeck(rsDeckId); },
              this)),
      mxFrame(rxFrame),
      maCurrentContext(OUString(), OUString()),
      maRequestedContext(),
      mnRequestedForceFlags(SwitchFlag_NoForce),
      msCurrentDeckId(gsDefaultDeckId),
      maPropertyChangeForwarder([this](){ return this->BroadcastPropertyChange(); }),
      maContextChangeUpdate([this](){ return this->UpdateConfigurations(); }),
      maAsynchronousDeckSwitch(),
      mbIsDeckRequestedOpen(),
      mbIsDeckOpen(),
      mbCanDeckBeOpened(true),
      mnSavedSidebarWidth(pParentWindow->GetSizePixel().Width()),
      maFocusManager([this](const Panel& rPanel){ return this->ShowPanel(rPanel); }),
      mxReadOnlyModeDispatch(),
      mbIsDocumentReadOnly(false),
      mpSplitWindow(nullptr),
      mnWidthOnSplitterButtonDown(0),
      mpResourceManager()
{
    // Decks and panel collections for this Notebookbar
    mpResourceManager = std::unique_ptr<ResourceManager>(new NotebookbarResourceManager());

    registerNotebookbarForFrame(this, mxFrame->getController());
    // Listen for window events.
    mpParentWindow->AddEventListener(LINK(this, NotebookbarController, WindowEventHandler));

    // Listen for theme property changes.
    Theme::GetPropertySet()->addPropertyChangeListener(
        "",
        static_cast<css::beans::XPropertyChangeListener*>(this));

    // Get the dispatch object as preparation to listen for changes of
    // the read-only state.
    const util::URL aURL (Tools::GetURL(gsReadOnlyCommandName));
    mxReadOnlyModeDispatch = Tools::GetDispatch(mxFrame, aURL);
    if (mxReadOnlyModeDispatch.is())
        mxReadOnlyModeDispatch->addStatusListener(this, aURL);

    SwitchToDeck(gsDefaultDeckId);
}

NotebookbarController::~NotebookbarController()
{
}

NotebookbarController* NotebookbarController::GetNotebookbarControllerForFrame (
    const css::uno::Reference<css::frame::XFrame>& rxFrame)
{
    uno::Reference<frame::XController> const xController(rxFrame->getController());
    if (!xController.is()) // this may happen during dispose of Draw controller but perhaps it's a bug
    {
        SAL_WARN("sfx.notebookbar", "GetNotebookbarControllerForFrame: frame has no XController");
        return nullptr;
    }
    uno::Reference<ui::XContextChangeEventListener> const xListener(
        framework::GetFirstListenerWith(xController,
            [] (uno::Reference<uno::XInterface> const& xRef)
            { return nullptr != dynamic_cast<NotebookbarController*>(xRef.get()); }
        ));

    return dynamic_cast<NotebookbarController*>(xListener.get());
}

void NotebookbarController::registerNotebookbarForFrame(NotebookbarController* pController, css::uno::Reference<css::frame::XController> xController)
{
    // Listen for context change events.
    css::uno::Reference<css::ui::XContextChangeEventMultiplexer> xMultiplexer (
        css::ui::ContextChangeEventMultiplexer::get(
            ::comphelper::getProcessComponentContext()));
    if (xMultiplexer.is())
        xMultiplexer->addContextChangeEventListener(
            static_cast<css::ui::XContextChangeEventListener*>(pController),
            xController);
}

void NotebookbarController::unregisterNotebookbarForFrame(NotebookbarController* pController, css::uno::Reference<css::frame::XController> xController)
{
    pController->disposeDecks();
    css::uno::Reference<css::ui::XContextChangeEventMultiplexer> xMultiplexer (
        css::ui::ContextChangeEventMultiplexer::get(
            ::comphelper::getProcessComponentContext()));
    if (xMultiplexer.is())
        xMultiplexer->removeContextChangeEventListener(
            static_cast<css::ui::XContextChangeEventListener*>(pController),
            xController);
}

void NotebookbarController::disposeDecks()
{
    mpCurrentDeck.clear();
    maFocusManager.Clear();
    mpResourceManager->disposeDecks();
}

void SAL_CALL NotebookbarController::disposing()
{
    mpCloseIndicator.disposeAndClear();

    maFocusManager.Clear();
    mpTabBar.disposeAndClear();

    // save decks settings
    // Impress shutdown : context (frame) is disposed before sidebar disposing
    // calc writer : context (frame) is disposed after sidebar disposing
    // so need to test if GetCurrentContext is still valid regarding msApplication

    if (GetCurrentContext().msApplication != "none")
        mpResourceManager->SaveDecksSettings(GetCurrentContext());

    // clear decks
    ResourceManager::DeckContextDescriptorContainer aDecks;

    mpResourceManager->GetMatchingDecks (
            aDecks,
            GetCurrentContext(),
            IsDocumentReadOnly(),
            mxFrame->getController());

    for (ResourceManager::DeckContextDescriptorContainer::const_iterator
            iDeck(aDecks.begin()), iEnd(aDecks.end());
            iDeck!=iEnd; ++iDeck)
    {
        const DeckDescriptor* deckDesc = mpResourceManager->GetDeckDescriptor(iDeck->msId);

        VclPtr<Deck> aDeck = deckDesc->mpDeck;
        if (aDeck)
            aDeck.disposeAndClear();
    }

    uno::Reference<css::frame::XController> xController = mxFrame->getController();
    if (!xController.is())
        xController = mxCurrentController;

    unregisterNotebookbarForFrame(this, xController);

    if (mxReadOnlyModeDispatch.is())
        mxReadOnlyModeDispatch->removeStatusListener(this, Tools::GetURL(gsReadOnlyCommandName));
    if (mpSplitWindow != nullptr)
    {
        mpSplitWindow->RemoveEventListener(LINK(this, NotebookbarController, WindowEventHandler));
        mpSplitWindow = nullptr;
    }

    if (mpParentWindow != nullptr)
    {
        mpParentWindow->RemoveEventListener(LINK(this, NotebookbarController, WindowEventHandler));
        mpParentWindow = nullptr;
    }

    Theme::GetPropertySet()->removePropertyChangeListener(
        "",
        static_cast<css::beans::XPropertyChangeListener*>(this));

    maContextChangeUpdate.CancelRequest();
    maAsynchronousDeckSwitch.CancelRequest();

}

void SAL_CALL NotebookbarController::notifyContextChangeEvent (const css::ui::ContextChangeEventObject& rEvent)
    throw(css::uno::RuntimeException, std::exception)
{
    // Update to the requested new context asynchronously to avoid
    // subtle errors caused by SFX2 which in rare cases can not
    // properly handle a synchronous update.

    maRequestedContext = Context(
        rEvent.ApplicationName,
        rEvent.ContextName);

    if (maRequestedContext != maCurrentContext)
    {
        mxCurrentController.set(rEvent.Source, css::uno::UNO_QUERY);
        maAsynchronousDeckSwitch.CancelRequest();
        maContextChangeUpdate.RequestCall();
        // TODO: this call is redundant but mandatory for unit test to update context on document loading
        UpdateConfigurations();
    }
}

void SAL_CALL NotebookbarController::disposing (const css::lang::EventObject& )
    throw(css::uno::RuntimeException, std::exception)
{
    dispose();
}

void SAL_CALL NotebookbarController::propertyChange (const css::beans::PropertyChangeEvent& )
    throw(css::uno::RuntimeException, std::exception)
{
    maPropertyChangeForwarder.RequestCall();
}

void SAL_CALL NotebookbarController::statusChanged (const css::frame::FeatureStateEvent& rEvent)
    throw(css::uno::RuntimeException, std::exception)
{
    bool bIsReadWrite (true);
    if (rEvent.IsEnabled)
        rEvent.State >>= bIsReadWrite;

    if (mbIsDocumentReadOnly != !bIsReadWrite)
    {
        mbIsDocumentReadOnly = !bIsReadWrite;

        // Force the current deck to update its panel list.
        if ( ! mbIsDocumentReadOnly)
            msCurrentDeckId = gsDefaultDeckId;

        mnRequestedForceFlags |= SwitchFlag_ForceSwitch;
        maAsynchronousDeckSwitch.CancelRequest();
        maContextChangeUpdate.RequestCall();
    }
}

void SAL_CALL NotebookbarController::requestLayout()
    throw(css::uno::RuntimeException, std::exception)
{
    if (mpCurrentDeck)
        mpCurrentDeck->RequestLayout();
}

void NotebookbarController::BroadcastPropertyChange()
{
    DataChangedEvent aEvent (DataChangedEventType::USER);
    mpParentWindow->NotifyAllChildren(aEvent);
    mpParentWindow->Invalidate(InvalidateFlags::Children);
}

void NotebookbarController::NotifyResize()
{
    if (!mpTabBar)
    {
        OSL_ASSERT(mpTabBar!=nullptr);
        return;
    }

    vcl::Window* pParentWindow = mpTabBar->GetParent();
    sal_Int32 nTabBarDefaultWidth = NotebookbarTabBar::GetDefaultWidth() * mpTabBar->GetDPIScaleFactor();
    sal_Int32 nTabBarDefaultHeight = NotebookbarTabBar::GetDefaultHeight() * mpTabBar->GetDPIScaleFactor();

    const sal_Int32 nWidth (pParentWindow->GetSizePixel().Width());
    const sal_Int32 nHeight (pParentWindow->GetSizePixel().Height());

    mbIsDeckOpen = (nWidth > nTabBarDefaultWidth);

    if (mnSavedSidebarWidth <= 0)
        mnSavedSidebarWidth = nWidth;

    if (mpCurrentDeck)
    {
        mpCurrentDeck->setPosSizePixel(0, nTabBarDefaultHeight, nWidth, nHeight);
        mpCurrentDeck->Show();
        mpCurrentDeck->RequestLayout();

        // Now place the tab bar.
        mpTabBar->setPosSizePixel(0, 0, nWidth, nTabBarDefaultHeight);
        mpTabBar->Show();

    }

    // Determine if the closer of the deck can be shown.
    if (mpCurrentDeck)
    {
        DeckTitleBar* pTitleBar = mpCurrentDeck->GetTitleBar();
        if (pTitleBar != nullptr && pTitleBar->IsVisible())
            pTitleBar->SetCloserVisible(CanModifyChildWindowWidth());
    }
}

void NotebookbarController::UpdateConfigurations()
{

    if (maCurrentContext != maRequestedContext
        || mnRequestedForceFlags!=SwitchFlag_NoForce)
    {

        if (maCurrentContext.msApplication != "none")
            mpResourceManager->SaveDecksSettings(maCurrentContext);

        maCurrentContext = maRequestedContext;

        mpResourceManager->InitDeckContext(GetCurrentContext());

        // Find the set of decks that could be displayed for the new context.
        ResourceManager::DeckContextDescriptorContainer aDecks;

        css::uno::Reference<css::frame::XController> xController = mxCurrentController.is() ? mxCurrentController : mxFrame->getController();

        mpResourceManager->GetMatchingDecks (
            aDecks,
            maCurrentContext,
            mbIsDocumentReadOnly,
            xController);

        // Notify the tab bar about the updated set of decks.
        mpTabBar->SetDecks(aDecks);

        // Find the new deck.  By default that is the same as the old
        // one.  If that is not set or not enabled, then choose the
        // first enabled deck (which is PropertyDeck).
        OUString sNewDeckId;
        for (ResourceManager::DeckContextDescriptorContainer::const_iterator
                 iDeck(aDecks.begin()),
                 iEnd(aDecks.end());
             iDeck!=iEnd;
             ++iDeck)
        {
            if (iDeck->mbIsEnabled)
            {
                if (iDeck->msId.equals(msCurrentDeckId))
                {
                    sNewDeckId = msCurrentDeckId;
                    break;
                }
                else if (sNewDeckId.getLength() == 0)
                    sNewDeckId = iDeck->msId;
            }
        }

        if (sNewDeckId.getLength() == 0)
        {
            // We did not find a valid deck.
            RequestCloseDeck();
            return;
        }

        // Tell the tab bar to highlight the button associated
        // with the deck.
        mpTabBar->HighlightDeck(sNewDeckId);

        const DeckDescriptor* pDescriptor = mpResourceManager->GetDeckDescriptor(sNewDeckId);

        if (pDescriptor)
        {
            SwitchToDeck(
                *pDescriptor,
                maCurrentContext);
        }
    }
}

void NotebookbarController::OpenThenSwitchToDeck (
    const ::rtl::OUString& rsDeckId)
{
    SfxSplitWindow* pSplitWindow = GetSplitWindow();
    if ( pSplitWindow && !pSplitWindow->IsFadeIn() )
        // tdf#83546 Collapsed sidebar should expand first
        pSplitWindow->FadeIn();
    else if ( IsDeckVisible( rsDeckId ) )
    {
        if ( pSplitWindow )
            // tdf#67627 Clicking a second time on a Deck icon will close the Deck
            RequestCloseDeck();
        else
            // tdf#88241 Summoning an undocked sidebar a second time should close sidebar
            mpParentWindow->Close();
        return;
    }
    RequestOpenDeck();
    SwitchToDeck(rsDeckId);
    mpTabBar->Invalidate();
    mpTabBar->HighlightDeck(rsDeckId);
}

void NotebookbarController::SwitchToDefaultDeck()
{
    SwitchToDeck(gsDefaultDeckId);
}

void NotebookbarController::SwitchToDeck (
    const ::rtl::OUString& rsDeckId)
{
    if ( ! msCurrentDeckId.equals(rsDeckId)
        || ! mbIsDeckOpen
        || mnRequestedForceFlags!=SwitchFlag_NoForce)
    {
        const DeckDescriptor* pDeckDescriptor = mpResourceManager->GetDeckDescriptor(rsDeckId);

        if (pDeckDescriptor != nullptr)
            SwitchToDeck(*pDeckDescriptor, maCurrentContext);
    }
}


void NotebookbarController::CreateDeck(const ::rtl::OUString& rDeckId, bool bForceCreate)
{
    DeckDescriptor* pDeckDescriptor = mpResourceManager->GetDeckDescriptor(rDeckId);

    if (pDeckDescriptor)
    {
        VclPtr<Deck> aDeck = pDeckDescriptor->mpDeck;
        if (aDeck.get()==nullptr || bForceCreate)
        {
            if (aDeck.get()!=nullptr)
                aDeck.disposeAndClear();

            aDeck = VclPtr<NotebookbarDeck>::Create(
                            *pDeckDescriptor,
                            mpParentWindow,
                            [this]() { return this->RequestCloseDeck(); });
        }
        pDeckDescriptor->mpDeck = aDeck;
        CreatePanels(rDeckId);
    }
}

void NotebookbarController::CreatePanels(const ::rtl::OUString& rDeckId)
{
    DeckDescriptor* pDeckDescriptor = mpResourceManager->GetDeckDescriptor(rDeckId);

    // init panels bounded to that deck, do not wait them being displayed as may be accessed through API

    VclPtr<Deck> pDeck = pDeckDescriptor->mpDeck;

    ResourceManager::PanelContextDescriptorContainer aPanelContextDescriptors;

    css::uno::Reference<css::frame::XController> xController = mxCurrentController.is() ? mxCurrentController : mxFrame->getController();

    mpResourceManager->GetMatchingPanels(
                                        aPanelContextDescriptors,
                                        maCurrentContext,
                                        rDeckId,
                                        xController);

    // Update the panel list.
    const sal_Int32 nNewPanelCount (aPanelContextDescriptors.size());
    SharedPanelContainer aNewPanels;

    aNewPanels.resize(nNewPanelCount);
    sal_Int32 nWriteIndex (0);

    for (sal_Int32 nReadIndex=0; nReadIndex<nNewPanelCount; ++nReadIndex)
    {
        const ResourceManager::PanelContextDescriptor& rPanelContexDescriptor (
            aPanelContextDescriptors[nReadIndex]);

        // Determine if the panel can be displayed.
        const bool bIsPanelVisible (!mbIsDocumentReadOnly || rPanelContexDescriptor.mbShowForReadOnlyDocuments);
        if ( ! bIsPanelVisible)
            continue;

        Panel *const pPanel(pDeck->GetPanel(rPanelContexDescriptor.msId));
        if (pPanel != nullptr)
        {
            aNewPanels[nWriteIndex] = pPanel;
            ++nWriteIndex;
        }
        else
        {
                VclPtr<Panel> aPanel = CreatePanel(
                                            rPanelContexDescriptor.msId,
                                            pDeck->GetPanelParentWindow(),
                                            rPanelContexDescriptor.mbIsInitiallyVisible,
                                            maCurrentContext,
                                            pDeck);
                if (aPanel.get()!=nullptr )
                {
                    aNewPanels[nWriteIndex] = aPanel;

                    // Depending on the context we have to change the command
                    // for the "more options" dialog.
                    PanelTitleBar* pTitleBar = aNewPanels[nWriteIndex]->GetTitleBar();
                    if (pTitleBar != nullptr)
                    {
                        pTitleBar->SetMoreOptionsCommand(
                            rPanelContexDescriptor.msMenuCommand,
                            mxFrame, xController);
                    }
                    ++nWriteIndex;
                 }

            }
        }

    // mpCurrentPanels - may miss stuff (?)
    aNewPanels.resize(nWriteIndex);
    pDeck->ResetPanels(aNewPanels);
}

void NotebookbarController::SwitchToDeck (
    const DeckDescriptor& rDeckDescriptor,
    const Context& rContext)
{

    maFocusManager.Clear();

    const bool bForceNewDeck ((mnRequestedForceFlags&SwitchFlag_ForceNewDeck)!=0);
    const bool bForceNewPanels ((mnRequestedForceFlags&SwitchFlag_ForceNewPanels)!=0);
    mnRequestedForceFlags = SwitchFlag_NoForce;

    if ( ! msCurrentDeckId.equals(rDeckDescriptor.msId)
        || bForceNewDeck)
    {
        if (mpCurrentDeck)
            mpCurrentDeck->Hide();

        msCurrentDeckId = rDeckDescriptor.msId;
    }
    mpTabBar->HighlightDeck(msCurrentDeckId);

    // Determine the panels to display in the deck.
    ResourceManager::PanelContextDescriptorContainer aPanelContextDescriptors;

    css::uno::Reference<css::frame::XController> xController = mxCurrentController.is() ? mxCurrentController : mxFrame->getController();

    mpResourceManager->GetMatchingPanels(
        aPanelContextDescriptors,
        rContext,
        rDeckDescriptor.msId,
        xController);

    if (aPanelContextDescriptors.empty())
    {
        // There are no panels to be displayed in the current context.
        if (EnumContext::GetContextEnum(rContext.msContext) != EnumContext::Context_Empty)
        {
            // Switch to the "empty" context and try again.
            SwitchToDeck(
                rDeckDescriptor,
                Context(
                    rContext.msApplication,
                    EnumContext::GetContextName(EnumContext::Context_Empty)));
            return;
        }
        else
        {
            // This is already the "empty" context. Looks like we have
            // to live with an empty deck.
        }
    }

    // Provide a configuration and Deck object.

    CreateDeck(rDeckDescriptor.msId, bForceNewDeck);

    if (bForceNewPanels && !bForceNewDeck) // already forced if bForceNewDeck
        CreatePanels(rDeckDescriptor.msId);

    mpCurrentDeck.reset(rDeckDescriptor.mpDeck);

    if ( ! mpCurrentDeck)
        return;

#ifdef DEBUG
    // Show the context name in the deck title bar.
    DeckTitleBar* pDebugTitleBar = mpCurrentDeck->GetTitleBar();
    if (pDebugTitleBar != NULL)
        pDebugTitleBar->SetTitle(rDeckDescriptor.msTitle + " (" + maCurrentContext.msContext + ")");
#endif

    SfxSplitWindow* pSplitWindow = GetSplitWindow();
    sal_Int32 nTabBarDefaultWidth = NotebookbarTabBar::GetDefaultWidth() * mpTabBar->GetDPIScaleFactor();
    WindowAlign eAlign = pSplitWindow ? pSplitWindow->GetAlign() : WindowAlign::Right;
    long nDeckX;
    if (eAlign == WindowAlign::Left)     // attach the Sidebar towards the left-side of screen
    {
        nDeckX = nTabBarDefaultWidth;
    }
    else   // attach the Sidebar towards the right-side of screen
    {
        nDeckX = 0;
    }


    // Activate the deck and the new set of panels.
    mpCurrentDeck->setPosSizePixel(
        nDeckX,
        0,
        mpParentWindow->GetSizePixel().Width() - nTabBarDefaultWidth,
        mpParentWindow->GetSizePixel().Height());

    mpCurrentDeck->Show();

    mpParentWindow->SetText(rDeckDescriptor.msTitle);

    NotifyResize();

    // Tell the focus manager about the new panels and tab bar
    // buttons.
    maFocusManager.SetDeckTitle(mpCurrentDeck->GetTitleBar());
    maFocusManager.SetPanels(mpCurrentDeck->GetPanels());

    mpTabBar->UpdateFocusManager(maFocusManager);
    UpdateTitleBarIcons();
}

void NotebookbarController::notifyDeckTitle(const OUString& targetDeckId)
{
    if (msCurrentDeckId == targetDeckId)
    {
        maFocusManager.SetDeckTitle(mpCurrentDeck->GetTitleBar());
        mpTabBar->UpdateFocusManager(maFocusManager);
        UpdateTitleBarIcons();
    }
}

VclPtr<Panel> NotebookbarController::CreatePanel (
    const OUString& rsPanelId,
    vcl::Window* pParentWindow,
    const bool bIsInitiallyExpanded,
    const Context& rContext,
    VclPtr<Deck> pDeck)
{
    const PanelDescriptor* pPanelDescriptor = mpResourceManager->GetPanelDescriptor(rsPanelId);

    if (pPanelDescriptor == nullptr)
        return nullptr;

    // Create the panel which is the parent window of the UIElement.
    VclPtr<Panel> pPanel = VclPtr<Panel>::Create(
        *pPanelDescriptor,
        pParentWindow,
        bIsInitiallyExpanded,
        [pDeck]() { return pDeck.get()->RequestLayout(); },
        [this]() { return this->GetCurrentContext(); },
        mxFrame);

    // Create the XUIElement.
    Reference<ui::XUIElement> xUIElement (CreateUIElement(
            pPanel->GetComponentInterface(),
            pPanelDescriptor->msImplementationURL,
            pPanelDescriptor->mbWantsCanvas,
            rContext));
    if (xUIElement.is())
    {
        // Initialize the panel and add it to the active deck.
        pPanel->SetUIElement(xUIElement);
    }
    else
    {
        pPanel.disposeAndClear();
    }

    return pPanel;
}

Reference<ui::XUIElement> NotebookbarController::CreateUIElement (
    const Reference<awt::XWindowPeer>& rxWindow,
    const ::rtl::OUString& rsImplementationURL,
    const bool bWantsCanvas,
    const Context& rContext)
{
    try
    {
        const Reference<XComponentContext> xComponentContext (::comphelper::getProcessComponentContext() );
        const Reference<ui::XUIElementFactory> xUIElementFactory =
               ui::theUIElementFactoryManager::get( xComponentContext );

       // Create the XUIElement.
        ::comphelper::NamedValueCollection aCreationArguments;
        aCreationArguments.put("Frame", makeAny(mxFrame));
        aCreationArguments.put("ParentWindow", makeAny(rxWindow));
        SfxDockingWindow* pSfxDockingWindow = dynamic_cast<SfxDockingWindow*>(mpParentWindow.get());
        if (pSfxDockingWindow != nullptr)
            aCreationArguments.put("SfxBindings", makeAny(sal_uInt64(&pSfxDockingWindow->GetBindings())));
        aCreationArguments.put("Theme", Theme::GetPropertySet());
        aCreationArguments.put("Sidebar", makeAny(Reference<ui::XSidebar>(static_cast<ui::XSidebar*>(this))));
        if (bWantsCanvas)
        {
            Reference<rendering::XSpriteCanvas> xCanvas (VCLUnoHelper::GetWindow(rxWindow)->GetSpriteCanvas());
            aCreationArguments.put("Canvas", makeAny(xCanvas));
        }

        if (mxCurrentController.is())
        {
            OUString aModule = Tools::GetModuleName(mxCurrentController);
            if (!aModule.isEmpty())
            {
                aCreationArguments.put("Module", makeAny(aModule));
            }
            aCreationArguments.put("Controller", makeAny(mxCurrentController));
        }

        aCreationArguments.put("ApplicationName", makeAny(rContext.msApplication));
        aCreationArguments.put("ContextName", makeAny(rContext.msContext));

        Reference<ui::XUIElement> xUIElement(
            xUIElementFactory->createUIElement(
                rsImplementationURL,
                Sequence<beans::PropertyValue>(aCreationArguments.getPropertyValues())),
            UNO_QUERY_THROW);

        return xUIElement;
    }
    catch(const Exception& rException)
    {
        SAL_WARN("sfx.notebookbar", "Cannot create panel " << rsImplementationURL << ": " << rException.Message);
        return nullptr;
    }
}

IMPL_LINK_TYPED(NotebookbarController, WindowEventHandler, VclWindowEvent&, rEvent, void)
{
    if (rEvent.GetWindow() == mpParentWindow)
    {
        switch (rEvent.GetId())
        {
            case VCLEVENT_WINDOW_SHOW:
            case VCLEVENT_WINDOW_RESIZE:
                NotifyResize();
                break;

            case VCLEVENT_WINDOW_DATACHANGED:
                // Force an update of deck and tab bar to reflect
                // changes in theme (high contrast mode).
                Theme::HandleDataChange();
                UpdateTitleBarIcons();
                mpParentWindow->Invalidate();
                mnRequestedForceFlags |= SwitchFlag_ForceNewDeck | SwitchFlag_ForceNewPanels;
                maAsynchronousDeckSwitch.CancelRequest();
                maContextChangeUpdate.RequestCall();
                break;

            case SFX_HINT_DYING:
                dispose();
                break;

            case VCLEVENT_WINDOW_PAINT:
                OSL_TRACE("Paint");
                break;

            default:
                break;
        }
    }
    else if (rEvent.GetWindow()==mpSplitWindow && mpSplitWindow!=nullptr)
    {
        switch (rEvent.GetId())
        {
            case VCLEVENT_WINDOW_MOUSEBUTTONDOWN:
                mnWidthOnSplitterButtonDown = mpParentWindow->GetSizePixel().Width();
                break;

            case VCLEVENT_WINDOW_MOUSEBUTTONUP:
            {
                mnWidthOnSplitterButtonDown = 0;
                break;
            }

            case SFX_HINT_DYING:
                dispose();
                break;
         }
    }
}

void NotebookbarController::RequestCloseDeck()
{
    mbIsDeckRequestedOpen = false;
    UpdateDeckOpenState();

    // remove highlight from TabBar, because Deck will be closed
    mpTabBar->RemoveDeckHighlight();
}

void NotebookbarController::RequestOpenDeck()
{
    mbIsDeckRequestedOpen = true;
    UpdateDeckOpenState();
}

bool NotebookbarController::IsDeckVisible(const OUString& rsDeckId)
{
    return mbIsDeckOpen && mbIsDeckOpen.get() && msCurrentDeckId == rsDeckId;
}

void NotebookbarController::UpdateDeckOpenState()
{
    if ( ! mbIsDeckRequestedOpen)
        // No state requested.
        return;

    sal_Int32 nTabBarDefaultWidth = NotebookbarTabBar::GetDefaultWidth() * mpTabBar->GetDPIScaleFactor();

    // Update (change) the open state when it either has not yet been initialized
    // or when its value differs from the requested state.
    if ( ! mbIsDeckOpen
        || mbIsDeckOpen.get() != mbIsDeckRequestedOpen.get())
    {
        if (!mbIsDeckRequestedOpen.get())
        {
            if ( ! mpParentWindow->IsFloatingMode())
                mnSavedSidebarWidth = SetChildWindowWidth(nTabBarDefaultWidth);
            if (mnWidthOnSplitterButtonDown > nTabBarDefaultWidth)
                mnSavedSidebarWidth = mnWidthOnSplitterButtonDown;
            mpParentWindow->SetStyle(mpParentWindow->GetStyle() & ~WB_SIZEABLE);
        }

        mbIsDeckOpen = mbIsDeckRequestedOpen.get();
        if (mbIsDeckOpen.get() && mpCurrentDeck)
            mpCurrentDeck->Show(mbIsDeckOpen.get());
        NotifyResize();
    }
}

bool NotebookbarController::CanModifyChildWindowWidth()
{
    SfxSplitWindow* pSplitWindow = GetSplitWindow();
    if (pSplitWindow == nullptr)
        return false;

    sal_uInt16 nRow (0xffff);
    sal_uInt16 nColumn (0xffff);
    if (pSplitWindow->GetWindowPos(mpParentWindow, nColumn, nRow))
    {
        sal_uInt16 nRowCount (pSplitWindow->GetWindowCount(nColumn));
        return nRowCount==1;
    }
    else
        return false;
}

sal_Int32 NotebookbarController::SetChildWindowWidth (const sal_Int32 nNewWidth)
{
    SfxSplitWindow* pSplitWindow = GetSplitWindow();
    if (pSplitWindow == nullptr)
        return 0;

    sal_uInt16 nRow (0xffff);
    sal_uInt16 nColumn (0xffff);
    pSplitWindow->GetWindowPos(mpParentWindow, nColumn, nRow);
    const long nColumnWidth (pSplitWindow->GetLineSize(nColumn));

    vcl::Window* pWindow = mpParentWindow;
    const Size aWindowSize (pWindow->GetSizePixel());

    pSplitWindow->MoveWindow(
        mpParentWindow,
        Size(nNewWidth, aWindowSize.Height()),
        nColumn,
        nRow);
    static_cast<SplitWindow*>(pSplitWindow)->Split();

    return static_cast<sal_Int32>(nColumnWidth);
}

SfxSplitWindow* NotebookbarController::GetSplitWindow()
{
    if (mpParentWindow != nullptr)
    {
        SfxSplitWindow* pSplitWindow = dynamic_cast<SfxSplitWindow*>(mpParentWindow->GetParent());
        if (pSplitWindow != mpSplitWindow)
        {
            if (mpSplitWindow != nullptr)
                mpSplitWindow->RemoveEventListener(LINK(this, NotebookbarController, WindowEventHandler));

            mpSplitWindow = pSplitWindow;

            if (mpSplitWindow != nullptr)
                mpSplitWindow->AddEventListener(LINK(this, NotebookbarController, WindowEventHandler));
        }
        return mpSplitWindow;
    }
    else
        return nullptr;
}

void NotebookbarController::UpdateCloseIndicator (const bool bCloseAfterDrag)
{
    if (mpParentWindow == nullptr)
        return;

    if (bCloseAfterDrag)
    {
        // Make sure that the indicator exists.
        if ( ! mpCloseIndicator)
        {
            mpCloseIndicator.reset(VclPtr<FixedImage>::Create(mpParentWindow));
            FixedImage* pFixedImage = static_cast<FixedImage*>(mpCloseIndicator.get());
            const Image aImage (Theme::GetImage(Theme::Image_CloseIndicator));
            pFixedImage->SetImage(aImage);
            pFixedImage->SetSizePixel(aImage.GetSizePixel());
            pFixedImage->SetBackground(Theme::GetWallpaper(Theme::Paint_DeckBackground));
        }

        // Place and show the indicator.
        const Size aWindowSize (mpParentWindow->GetSizePixel());
        const Size aImageSize (mpCloseIndicator->GetSizePixel());
        mpCloseIndicator->SetPosPixel(
            Point(
                aWindowSize.Width() - NotebookbarTabBar::GetDefaultWidth() * mpTabBar->GetDPIScaleFactor() - aImageSize.Width(),
                (aWindowSize.Height() - aImageSize.Height())/2));
        mpCloseIndicator->Show();
    }
    else
    {
        // Hide but don't delete the indicator.
        if (mpCloseIndicator)
            mpCloseIndicator->Hide();
    }
}

void NotebookbarController::UpdateTitleBarIcons()
{
    if ( ! mpCurrentDeck)
        return;

    const bool bIsHighContrastModeActive (Theme::IsHighContrastMode());

    const ResourceManager& rResourceManager = *mpResourceManager;

    // Update the deck icon.
    const DeckDescriptor* pDeckDescriptor = rResourceManager.GetDeckDescriptor(mpCurrentDeck->GetId());
    if (pDeckDescriptor != nullptr && mpCurrentDeck->GetTitleBar())
    {
        const OUString sIconURL(
            bIsHighContrastModeActive
                ? pDeckDescriptor->msHighContrastTitleBarIconURL
                : pDeckDescriptor->msTitleBarIconURL);
        mpCurrentDeck->GetTitleBar()->SetIcon(Tools::GetImage(sIconURL, mxFrame));
    }

    // Update the panel icons.
    const SharedPanelContainer& rPanels (mpCurrentDeck->GetPanels());
    for (SharedPanelContainer::const_iterator
             iPanel(rPanels.begin()), iEnd(rPanels.end());
             iPanel!=iEnd;
             ++iPanel)
    {
        if ( ! *iPanel)
            continue;
        if ((*iPanel)->GetTitleBar() == nullptr)
            continue;
        const PanelDescriptor* pPanelDescriptor = rResourceManager.GetPanelDescriptor((*iPanel)->GetId());
        if (pPanelDescriptor == nullptr)
            continue;
        const OUString sIconURL (
            bIsHighContrastModeActive
               ? pPanelDescriptor->msHighContrastTitleBarIconURL
               : pPanelDescriptor->msTitleBarIconURL);
        (*iPanel)->GetTitleBar()->SetIcon(Tools::GetImage(sIconURL, mxFrame));
    }
}

void NotebookbarController::ShowPanel (const Panel& rPanel)
{
    if (mpCurrentDeck)
        mpCurrentDeck->ShowPanel(rPanel);
}

ResourceManager::DeckContextDescriptorContainer NotebookbarController::GetMatchingDecks()
{
    ResourceManager::DeckContextDescriptorContainer aDecks;
    mpResourceManager->GetMatchingDecks (aDecks,
                                        GetCurrentContext(),
                                        IsDocumentReadOnly(),
                                        mxFrame->getController());
    return aDecks;
}

ResourceManager::PanelContextDescriptorContainer NotebookbarController::GetMatchingPanels(const ::rtl::OUString& rDeckId)
{
    ResourceManager::PanelContextDescriptorContainer aPanels;

    mpResourceManager->GetMatchingPanels(aPanels,
                                        GetCurrentContext(),
                                        rDeckId,
                                        mxFrame->getController());
    return aPanels;
}

void NotebookbarController::updateModel(css::uno::Reference<css::frame::XModel> xModel)
{
    mpResourceManager->UpdateModel(xModel);
}


} } // end of namespace sfx2::sidebar

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
