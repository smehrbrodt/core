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

#include <sfx2/abstractbar/ResourceManager.hxx>
#include <sfx2/abstractbar/Tools.hxx>

#include <unotools/confignode.hxx>
#include <comphelper/processfactory.hxx>
#include <comphelper/namedvaluecollection.hxx>
#include <comphelper/types.hxx>

#include <rtl/ustrbuf.hxx>
#include <tools/diagnose_ex.h>

#include <com/sun/star/frame/ModuleManager.hpp>
#include <com/sun/star/ui/XUpdateModel.hpp>

#include <map>

using namespace css;
using namespace css::uno;

namespace sfx2 { namespace abstractbar {

ResourceManager::ResourceManager()
    : maDecks(),
      maPanels(),
      maProcessedApplications(),
      maMiscOptions()
{
}

ResourceManager::~ResourceManager()
{
}

void ResourceManager::InitDeckContext(const Context& rContext)
{
    DeckContainer::iterator iDeck;
    for (iDeck = maDecks.begin(); iDeck != maDecks.end(); ++iDeck)
    {
        bool bIsEnabled;
        const ContextList::Entry* pMatchingEntry = iDeck->maContextList.GetMatch(rContext);

        if (pMatchingEntry)
            bIsEnabled = pMatchingEntry->mbIsInitiallyVisible;
        else
            bIsEnabled = false;

        iDeck->mbIsEnabled = bIsEnabled;
    }
}

const DeckDescriptor* ResourceManager::ImplGetDeckDescriptor(const OUString& rsDeckId) const
{
    DeckContainer::const_iterator iDeck;

    for (iDeck = maDecks.begin(); iDeck != maDecks.end(); ++iDeck)
    {
        if (iDeck->mbExperimental && !maMiscOptions.IsExperimentalMode())
            continue;
        if (iDeck->msId.equals(rsDeckId))
            return &*iDeck;
    }
    return nullptr;
}
const DeckDescriptor* ResourceManager::GetDeckDescriptor(const OUString& rsDeckId) const
{
    return ImplGetDeckDescriptor( rsDeckId );
}

DeckDescriptor* ResourceManager::GetDeckDescriptor(const OUString& rsDeckId)
{
      const ResourceManager* constMe = this;
      return const_cast<DeckDescriptor*>( constMe->ImplGetDeckDescriptor(rsDeckId) );
}

const PanelDescriptor* ResourceManager::ImplGetPanelDescriptor(const OUString& rsPanelId) const
{
    PanelContainer::const_iterator iPanel;
    for (iPanel = maPanels.begin(); iPanel != maPanels.end(); ++iPanel)
    {
        if (iPanel->msId.equals(rsPanelId))
            return &*iPanel;
    }
    return nullptr;
}
const PanelDescriptor* ResourceManager::GetPanelDescriptor(const OUString& rsPanelId) const
{
    return ImplGetPanelDescriptor( rsPanelId );
}

PanelDescriptor* ResourceManager::GetPanelDescriptor(const OUString& rsPanelId)
{
      const ResourceManager* constMe = this;
      return const_cast<PanelDescriptor*>( constMe->ImplGetPanelDescriptor(rsPanelId) );
}

const ResourceManager::DeckContextDescriptorContainer& ResourceManager::GetMatchingDecks (
                                                            DeckContextDescriptorContainer& rDecks,
                                                            const Context& rContext,
                                                            const bool bIsDocumentReadOnly,
                                                            const Reference<frame::XController>& rxController)
{
    std::multimap<sal_Int32,DeckContextDescriptor> aOrderedIds;
    DeckContainer::const_iterator iDeck;
    for (iDeck = maDecks.begin(); iDeck != maDecks.end(); ++iDeck)
    {
        if (iDeck->mbExperimental && !maMiscOptions.IsExperimentalMode())
            continue;

        const DeckDescriptor& rDeckDescriptor (*iDeck);
        if (rDeckDescriptor.maContextList.GetMatch(rContext) == nullptr)
            continue;

        DeckContextDescriptor aDeckContextDescriptor;
        aDeckContextDescriptor.msId = rDeckDescriptor.msId;

        aDeckContextDescriptor.mbIsEnabled = (! bIsDocumentReadOnly || IsDeckEnabled(rDeckDescriptor.msId, rContext, rxController) )
                                             && rDeckDescriptor.mbIsEnabled;


        aOrderedIds.insert(::std::multimap<sal_Int32,DeckContextDescriptor>::value_type(
                rDeckDescriptor.mnOrderIndex,
                aDeckContextDescriptor));
    }

    std::multimap<sal_Int32,DeckContextDescriptor>::const_iterator iId;
    for (iId = aOrderedIds.begin(); iId != aOrderedIds.end(); ++iId)
    {
        rDecks.push_back(iId->second);
    }

    return rDecks;
}

const ResourceManager::PanelContextDescriptorContainer& ResourceManager::GetMatchingPanels (
                                                            PanelContextDescriptorContainer& rPanelIds,
                                                            const Context& rContext,
                                                            const OUString& rsDeckId,
                                                            const Reference<frame::XController>& rxController)
{
    std::multimap<sal_Int32, PanelContextDescriptor> aOrderedIds;
    PanelContainer::const_iterator iPanel;
    for (iPanel = maPanels.begin(); iPanel != maPanels.end(); ++iPanel)
    {
        const PanelDescriptor& rPanelDescriptor (*iPanel);
        if (rPanelDescriptor.mbExperimental && !maMiscOptions.IsExperimentalMode())
            continue;
        if ( ! rPanelDescriptor.msDeckId.equals(rsDeckId))
            continue;

        const ContextList::Entry* pEntry = rPanelDescriptor.maContextList.GetMatch(rContext);
        if (pEntry == nullptr)
            continue;

        PanelContextDescriptor aPanelContextDescriptor;
        aPanelContextDescriptor.msId = rPanelDescriptor.msId;
        aPanelContextDescriptor.msMenuCommand = pEntry->msMenuCommand;
        aPanelContextDescriptor.mbIsInitiallyVisible = pEntry->mbIsInitiallyVisible;
        aPanelContextDescriptor.mbShowForReadOnlyDocuments = rPanelDescriptor.mbShowForReadOnlyDocuments;
        aOrderedIds.insert(std::multimap<sal_Int32, PanelContextDescriptor>::value_type(
                                                    rPanelDescriptor.mnOrderIndex,
                                                    aPanelContextDescriptor));
    }

    std::multimap<sal_Int32,PanelContextDescriptor>::const_iterator iId;
    for (iId = aOrderedIds.begin(); iId != aOrderedIds.end(); ++iId)
    {
        rPanelIds.push_back(iId->second);
    }

    return rPanelIds;
}

void ResourceManager::SaveDecksSettings(const Context& rContext)
{

    DeckContainer::const_iterator iDeck;
    for (iDeck = maDecks.begin(); iDeck != maDecks.end(); ++iDeck)
    {
       const ContextList::Entry* pMatchingEntry = iDeck->maContextList.GetMatch(rContext);
       if (pMatchingEntry)
       {
            const DeckDescriptor* pDeckDesc = GetDeckDescriptor(iDeck->msId);
            if (pDeckDesc)
                SaveDeckSettings(pDeckDesc);
       }

    }
}

void ResourceManager::SaveDeckSettings(const DeckDescriptor* pDeckDesc)
{
    const utl::OConfigurationTreeRoot aDeckRootNode(
                                    comphelper::getProcessComponentContext(),
                                    OUString("org.openoffice.Office.UI.Sidebar/Content/DeckList"),
                                    true);
    if (!aDeckRootNode.isValid())
        return;

    // save deck settings

    ::uno::Sequence< OUString > sContextList = BuildContextList(pDeckDesc->maContextList, pDeckDesc->mbIsEnabled);

    utl::OConfigurationNode aDeckNode (aDeckRootNode.openNode(pDeckDesc->msNodeName));

    aDeckNode.setNodeValue("Title", makeAny(pDeckDesc->msTitle));
    aDeckNode.setNodeValue("OrderIndex", makeAny(pDeckDesc->mnOrderIndex));
    aDeckNode.setNodeValue("ContextList", makeAny( sContextList ));

    aDeckRootNode.commit();

    // save panel settings

    const utl::OConfigurationTreeRoot aPanelRootNode(
                                    comphelper::getProcessComponentContext(),
                                    OUString("org.openoffice.Office.UI.Sidebar/Content/PanelList"),
                                    true);

    if (!aPanelRootNode.isValid())
        return;

    if (!pDeckDesc->mpDeck) // the deck has not been edited
        return;

    SharedPanelContainer rPanels = pDeckDesc->mpDeck->GetPanels();

    for ( SharedPanelContainer::iterator iPanel(rPanels.begin()), iEnd(rPanels.end());
              iPanel!=iEnd; ++iPanel)
            {
                Panel* aPanel = *iPanel;
                OUString panelId = aPanel->GetId();
                bool isExpanded = aPanel->IsExpanded();
                const PanelDescriptor* pPanelDesc = GetPanelDescriptor(panelId);

                ::uno::Sequence< OUString > sPanelContextList = BuildContextList(pPanelDesc->maContextList, isExpanded);

                utl::OConfigurationNode aPanelNode (aPanelRootNode.openNode(pPanelDesc->msNodeName));

                aPanelNode.setNodeValue("Title", makeAny(pPanelDesc->msTitle));
                aPanelNode.setNodeValue("OrderIndex", makeAny(pPanelDesc->mnOrderIndex));
                aPanelNode.setNodeValue("ContextList", makeAny( sPanelContextList ));

            }

     aPanelRootNode.commit();

}

void ResourceManager::ReadContextList (
                        const utl::OConfigurationNode& rParentNode,
                        ContextList& rContextList,
                        const OUString& rsDefaultMenuCommand)
{
    const Any aValue = rParentNode.getNodeValue("ContextList");
    Sequence<OUString> aValues;
    sal_Int32 nCount;
    if (aValue >>= aValues)
        nCount = aValues.getLength();
    else
        nCount = 0;

    for (sal_Int32 nIndex=0; nIndex<nCount; ++nIndex)
    {
        const OUString sValue (aValues[nIndex]);

        sal_Int32 nCharacterIndex (0);
        const OUString sApplicationName (sValue.getToken(0, ',', nCharacterIndex).trim());
        if (nCharacterIndex < 0)
        {
            if (sApplicationName.getLength() == 0)
            {
                // This is a valid case: in the XML file the separator
                // was used as terminator.  Using it in the last line
                // creates an additional but empty entry.
                break;
            }
            else
            {
                OSL_FAIL("expecting three or four values per ContextList entry, separated by comma");
                continue;
            }
        }

        const OUString sContextName(sValue.getToken(0, ',', nCharacterIndex).trim());
        if (nCharacterIndex < 0)
        {
            OSL_FAIL("expecting three or four values per ContextList entry, separated by comma");
            continue;
        }

        const OUString sInitialState(sValue.getToken(0, ',', nCharacterIndex).trim());

        // The fourth argument is optional.
        const OUString sMenuCommandOverride(
            nCharacterIndex < 0
                ? OUString()
                : sValue.getToken(0, ',', nCharacterIndex).trim());

        const OUString sMenuCommand(
            sMenuCommandOverride.getLength() > 0
                ? (sMenuCommandOverride == "none"
                    ? OUString()
                    : sMenuCommandOverride)
                : rsDefaultMenuCommand);

        // Setup a list of application enums.  Note that the
        // application name may result in more than one value (eg
        // DrawImpress will result in two enums, one for Draw and one
        // for Impress).
        std::vector<EnumContext::Application> aApplications;
        EnumContext::Application eApplication (EnumContext::GetApplicationEnum(sApplicationName));

        if (eApplication == EnumContext::Application_None
            && !sApplicationName.equals(EnumContext::GetApplicationName(EnumContext::Application_None)))
        {
            // Handle some special names: abbreviations that make
            // context descriptions more readable.
            if (sApplicationName == "Writer")
                aApplications.push_back(EnumContext::Application_Writer);
            else if (sApplicationName == "Calc")
                aApplications.push_back(EnumContext::Application_Calc);
            else if (sApplicationName == "Draw")
                aApplications.push_back(EnumContext::Application_Draw);
            else if (sApplicationName == "Impress")
                aApplications.push_back(EnumContext::Application_Impress);
            else if (sApplicationName == "Chart")
                aApplications.push_back(EnumContext::Application_Chart);
            else if (sApplicationName == "DrawImpress")
            {
                // A special case among the special names:  it is
                // common to use the same context descriptions for
                // both Draw and Impress.  This special case helps to
                // avoid duplication in the .xcu file.
                aApplications.push_back(EnumContext::Application_Draw);
                aApplications.push_back(EnumContext::Application_Impress);
            }
            else if (sApplicationName == "WriterVariants")
            {
                // Another special case for all Writer variants.
                aApplications.push_back(EnumContext::Application_Writer);
                aApplications.push_back(EnumContext::Application_WriterGlobal);
                aApplications.push_back(EnumContext::Application_WriterWeb);
                aApplications.push_back(EnumContext::Application_WriterXML);
                aApplications.push_back(EnumContext::Application_WriterForm);
                aApplications.push_back(EnumContext::Application_WriterReport);
            }
            else
            {
                SAL_WARN("sfx.abstractbar", "application name " << sApplicationName << " not recognized");
                continue;
            }
        }
        else
        {
            // No conversion of the application name necessary.
            aApplications.push_back(eApplication);
        }

        // Setup the actual context enum.
        const EnumContext::Context eContext (EnumContext::GetContextEnum(sContextName));
        if (eContext == EnumContext::Context_Unknown)
        {
            SAL_WARN("sfx.abstractbar", "context name " << sContextName << " not recognized");
            continue;
        }

        // Setup the flag that controls whether a deck/pane is
        // initially visible/expanded.
        bool bIsInitiallyVisible;
        if (sInitialState == "visible")
            bIsInitiallyVisible = true;
        else if (sInitialState == "hidden")
            bIsInitiallyVisible = false;
        else
        {
            OSL_FAIL("unrecognized state");
            continue;
        }


        // Add context descriptors.
        std::vector<EnumContext::Application>::const_iterator iApplication;
        for (iApplication = aApplications.begin(); iApplication != aApplications.end(); ++iApplication)
        {
            if (*iApplication != EnumContext::Application_None)
            {
                rContextList.AddContextDescription(
                    Context(
                        EnumContext::GetApplicationName(*iApplication),
                        EnumContext::GetContextName(eContext)),
                    bIsInitiallyVisible,
                    sMenuCommand);
            }
        }
    }
}

void ResourceManager::StorePanelExpansionState (
                        const OUString& rsPanelId,
                        const bool bExpansionState,
                        const Context& rContext)
{
    PanelContainer::iterator iPanel;
    for (iPanel = maPanels.begin(); iPanel != maPanels.end(); ++iPanel)
    {
        if (iPanel->msId.equals(rsPanelId))
        {
            ContextList::Entry* pEntry(iPanel->maContextList.GetMatch(rContext));
            if (pEntry != nullptr)
                pEntry->mbIsInitiallyVisible = bExpansionState;
        }
    }
}

void ResourceManager::GetToolPanelNodeNames (
                        std::vector<OUString>& rMatchingNames,
                        const utl::OConfigurationTreeRoot& aRoot)
{
    Sequence<OUString> aChildNodeNames (aRoot.getNodeNames());
    const sal_Int32 nCount (aChildNodeNames.getLength());
    for (sal_Int32 nIndex(0); nIndex<nCount; ++nIndex)
    {
        if (aChildNodeNames[nIndex].startsWith( "private:resource/toolpanel/" ))
            rMatchingNames.push_back(aChildNodeNames[nIndex]);
    }
}

bool ResourceManager::IsDeckEnabled (
                        const OUString& rsDeckId,
                        const Context& rContext,
                        const Reference<frame::XController>& rxController)
{

    // Check if any panel that matches the current context can be
    // displayed.
    PanelContextDescriptorContainer aPanelContextDescriptors;

    GetMatchingPanels(aPanelContextDescriptors, rContext, rsDeckId, rxController);

    PanelContextDescriptorContainer::const_iterator iPanel;
    for (iPanel = aPanelContextDescriptors.begin(); iPanel != aPanelContextDescriptors.end(); ++iPanel)
    {
        if (iPanel->mbShowForReadOnlyDocuments)
            return true;
    }
    return false;
}

void ResourceManager::UpdateModel(css::uno::Reference<css::frame::XModel> xModel)
{
    for (DeckContainer::iterator itr = maDecks.begin(); itr != maDecks.end(); ++itr)
    {
        if (!itr->mpDeck)
            continue;

        const SharedPanelContainer& rContainer = itr->mpDeck->GetPanels();

        for (SharedPanelContainer::const_iterator it = rContainer.begin(); it != rContainer.end(); ++it)
        {
            css::uno::Reference<css::ui::XUpdateModel> xPanel((*it)->GetPanelComponent(), css::uno::UNO_QUERY);
            xPanel->updateModel(xModel);
        }
    }
}

void ResourceManager::disposeDecks()
{
    for (DeckContainer::iterator itr = maDecks.begin(); itr != maDecks.end(); ++itr)
        itr->mpDeck.disposeAndClear();
}

OUString ResourceManager::getString(utl::OConfigurationNode const & aNode, const char* pNodeName)
{
    return comphelper::getString(aNode.getNodeValue(pNodeName));
}
sal_Int32 ResourceManager::getInt32(utl::OConfigurationNode const & aNode, const char* pNodeName)
{
    return comphelper::getINT32(aNode.getNodeValue(pNodeName));
}
bool ResourceManager::getBool(utl::OConfigurationNode const & aNode, const char* pNodeName)
{
    return comphelper::getBOOL(aNode.getNodeValue(pNodeName));
}

css::uno::Sequence<OUString> ResourceManager::BuildContextList (ContextList rContextList, bool isEnabled)
{
    const ::std::vector<ContextList::Entry>& entries = rContextList.GetEntries();

     css::uno::Sequence<OUString> result(entries.size());
     long i = 0;

    for (::std::vector<ContextList::Entry>::const_iterator iEntry(entries.begin()), iEnd(entries.end());
                                                            iEntry!=iEnd; ++iEntry)
         {
            OUString appName = iEntry->maContext.msApplication;
            OUString contextName = iEntry->maContext.msContext;
            OUString menuCommand = iEntry->msMenuCommand;

            OUString visibility;
            if (isEnabled)
                visibility = "visible";
            else
                visibility = "hidden";

            OUString element = appName + ", " + contextName +", " + visibility;

            if (!menuCommand.isEmpty())
                element += ", "+menuCommand;

            result[i] = element;

            i++;
        }

    return result;

}

} } // end of namespace sfx2::abstractbar

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
