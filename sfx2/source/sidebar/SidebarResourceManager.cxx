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

#include <sfx2/sidebar/SidebarResourceManager.hxx>
#include <sfx2/sidebar/SidebarDeckDescriptor.hxx>
#include <sfx2/sidebar/SidebarPanelDescriptor.hxx>

#include <comphelper/processfactory.hxx>

using namespace css;
using namespace css::uno;

namespace sfx2 { namespace sidebar {


SidebarResourceManager::SidebarResourceManager()
    : ResourceManager()
{
    ReadDeckList();
    ReadPanelList();
}

SidebarResourceManager::~SidebarResourceManager()
{
}

void SidebarResourceManager::ReadDeckList()
{
    const utl::OConfigurationTreeRoot aDeckRootNode(
                                        comphelper::getProcessComponentContext(),
                                        OUString("org.openoffice.Office.UI.Sidebar/Content/DeckList"),
                                        false);
    if (!aDeckRootNode.isValid())
        return;

    const Sequence<OUString> aDeckNodeNames (aDeckRootNode.getNodeNames());
    const sal_Int32 nCount(aDeckNodeNames.getLength());
    maDecks.resize(nCount);
    sal_Int32 nWriteIndex(0);
    for (sal_Int32 nReadIndex(0); nReadIndex<nCount; ++nReadIndex)
    {
        const utl::OConfigurationNode aDeckNode(aDeckRootNode.openNode(aDeckNodeNames[nReadIndex]));
        if (!aDeckNode.isValid())
            continue;

        SidebarDeckDescriptor& rDeckDescriptor (maDecks[nWriteIndex++]);

        rDeckDescriptor.msTitle = getString(aDeckNode, "Title");
        rDeckDescriptor.msId = getString(aDeckNode, "Id");
        rDeckDescriptor.msIconURL = getString(aDeckNode, "IconURL");
        rDeckDescriptor.msHighContrastIconURL = getString(aDeckNode, "HighContrastIconURL");
        rDeckDescriptor.msTitleBarIconURL = getString(aDeckNode, "TitleBarIconURL");
        rDeckDescriptor.msHighContrastTitleBarIconURL = getString(aDeckNode, "HighContrastTitleBarIconURL");
        rDeckDescriptor.msHelpURL = getString(aDeckNode, "HelpURL");
        rDeckDescriptor.msHelpText = rDeckDescriptor.msTitle;
    //  rDeckDescriptor.mbIsEnabled = true; // TODO ??? update rDeckDescriptor.mbIsEnabled according to context , see IsDeckEnabled ?
        rDeckDescriptor.mnOrderIndex = getInt32(aDeckNode, "OrderIndex");
        rDeckDescriptor.mbExperimental = getBool(aDeckNode, "IsExperimental");

        rDeckDescriptor.msNodeName = aDeckNodeNames[nReadIndex];

        ReadContextList(
            aDeckNode,
            rDeckDescriptor.maContextList,
            OUString());

    }

    // When there where invalid nodes then we have to adapt the size
    // of the deck vector.
    if (nWriteIndex<nCount)
        maDecks.resize(nWriteIndex);
}

void SidebarResourceManager::ReadPanelList()
{
    const utl::OConfigurationTreeRoot aPanelRootNode(
                                        comphelper::getProcessComponentContext(),
                                        OUString("org.openoffice.Office.UI.Sidebar/Content/PanelList"),
                                        false);
    if (!aPanelRootNode.isValid())
        return;

    const Sequence<OUString> aPanelNodeNames (aPanelRootNode.getNodeNames());
    const sal_Int32 nCount (aPanelNodeNames.getLength());
    maPanels.resize(nCount);
    sal_Int32 nWriteIndex (0);
    for (sal_Int32 nReadIndex(0); nReadIndex<nCount; ++nReadIndex)
    {
        const utl::OConfigurationNode aPanelNode (aPanelRootNode.openNode(aPanelNodeNames[nReadIndex]));
        if (!aPanelNode.isValid())
            continue;

        SidebarPanelDescriptor& rPanelDescriptor (maPanels[nWriteIndex++]);

        rPanelDescriptor.msTitle = getString(aPanelNode, "Title");
        rPanelDescriptor.mbIsTitleBarOptional = getBool(aPanelNode, "TitleBarIsOptional");
        rPanelDescriptor.msId = getString(aPanelNode, "Id");
        rPanelDescriptor.msDeckId = getString(aPanelNode, "DeckId");
        rPanelDescriptor.msTitleBarIconURL = getString(aPanelNode, "TitleBarIconURL");
        rPanelDescriptor.msHighContrastTitleBarIconURL = getString(aPanelNode, "HighContrastTitleBarIconURL");
        rPanelDescriptor.msHelpURL = getString(aPanelNode, "HelpURL");
        rPanelDescriptor.msImplementationURL = getString(aPanelNode, "ImplementationURL");
        rPanelDescriptor.mnOrderIndex = getInt32(aPanelNode, "OrderIndex");
        rPanelDescriptor.mbShowForReadOnlyDocuments = getBool(aPanelNode, "ShowForReadOnlyDocument");
        rPanelDescriptor.mbWantsCanvas = getBool(aPanelNode, "WantsCanvas");
        rPanelDescriptor.mbExperimental = getBool(aPanelNode, "IsExperimental");
        const OUString sDefaultMenuCommand(getString(aPanelNode, "DefaultMenuCommand"));

        rPanelDescriptor.msNodeName = aPanelNodeNames[nReadIndex];

        ReadContextList(aPanelNode, rPanelDescriptor.maContextList, sDefaultMenuCommand);
    }

    // When there where invalid nodes then we have to adapt the size
    // of the deck vector.
    if (nWriteIndex<nCount)
        maPanels.resize(nWriteIndex);
}

const ResourceManager::DeckContextDescriptorContainer& SidebarResourceManager::GetMatchingDecks (
                                                            DeckContextDescriptorContainer& rDecks,
                                                            const Context& rContext,
                                                            const bool bIsDocumentReadOnly,
                                                            const Reference<frame::XController>& rxController)
{
    ReadLegacyAddons(rxController);

    return ResourceManager::GetMatchingDecks(rDecks, rContext, bIsDocumentReadOnly, rxController);
}

const ResourceManager::PanelContextDescriptorContainer& SidebarResourceManager::GetMatchingPanels (
                                                            PanelContextDescriptorContainer& rPanelIds,
                                                            const Context& rContext,
                                                            const OUString& rsDeckId,
                                                            const Reference<frame::XController>& rxController)
{
    ReadLegacyAddons(rxController);

    return ResourceManager::GetMatchingPanels(rPanelIds, rContext, rsDeckId, rxController);
}

void SidebarResourceManager::ReadLegacyAddons (const Reference<frame::XController>& rxController)
{
    // Get module name for given frame.
    OUString sModuleName (Tools::GetModuleName(rxController));
    if (sModuleName.getLength() == 0)
        return;
    if (maProcessedApplications.find(sModuleName) != maProcessedApplications.end())
    {
        // Addons for this application have already been read.
        // There is nothing more to do.
        return;
    }

    // Mark module as processed.  Even when there is an error that
    // prevents the configuration data from being read, this error
    // will not be triggered a second time.
    maProcessedApplications.insert(sModuleName);

    // Get access to the configuration root node for the application.
    utl::OConfigurationTreeRoot aLegacyRootNode (GetLegacyAddonRootNode(sModuleName));
    if (!aLegacyRootNode.isValid())
        return;

    // Process child nodes.
    std::vector<OUString> aMatchingNodeNames;
    GetToolPanelNodeNames(aMatchingNodeNames, aLegacyRootNode);
    const sal_Int32 nCount (aMatchingNodeNames.size());
    size_t nDeckWriteIndex (maDecks.size());
    size_t nPanelWriteIndex (maPanels.size());
    maDecks.resize(maDecks.size() + nCount);
    maPanels.resize(maPanels.size() + nCount);
    for (sal_Int32 nReadIndex(0); nReadIndex<nCount; ++nReadIndex)
    {
        const OUString& rsNodeName (aMatchingNodeNames[nReadIndex]);
        const utl::OConfigurationNode aChildNode (aLegacyRootNode.openNode(rsNodeName));
        if (!aChildNode.isValid())
            continue;

        if ( rsNodeName == "private:resource/toolpanel/DrawingFramework/CustomAnimations" ||
             rsNodeName == "private:resource/toolpanel/DrawingFramework/Layouts" ||
             rsNodeName == "private:resource/toolpanel/DrawingFramework/MasterPages" ||
             rsNodeName == "private:resource/toolpanel/DrawingFramework/SlideTransitions" ||
             rsNodeName == "private:resource/toolpanel/DrawingFramework/TableDesign" )
          continue;

        SidebarDeckDescriptor& rDeckDescriptor (maDecks[nDeckWriteIndex++]);
        rDeckDescriptor.msTitle = getString(aChildNode, "UIName");
        rDeckDescriptor.msId = rsNodeName;
        rDeckDescriptor.msIconURL = getString(aChildNode, "ImageURL");
        rDeckDescriptor.msHighContrastIconURL = rDeckDescriptor.msIconURL;
        rDeckDescriptor.msTitleBarIconURL.clear();
        rDeckDescriptor.msHighContrastTitleBarIconURL.clear();
        rDeckDescriptor.msHelpURL = getString(aChildNode, "HelpURL");
        rDeckDescriptor.msHelpText = rDeckDescriptor.msTitle;
        rDeckDescriptor.mbIsEnabled = true;
        rDeckDescriptor.mnOrderIndex = 100000 + nReadIndex;
        rDeckDescriptor.maContextList.AddContextDescription(Context(sModuleName, OUString("any")), true, OUString());

        SidebarPanelDescriptor& rPanelDescriptor (maPanels[nPanelWriteIndex++]);
        rPanelDescriptor.msTitle = getString(aChildNode, "UIName");
        rPanelDescriptor.mbIsTitleBarOptional = true;
        rPanelDescriptor.msId = rsNodeName;
        rPanelDescriptor.msDeckId = rsNodeName;
        rPanelDescriptor.msTitleBarIconURL.clear();
        rPanelDescriptor.msHighContrastTitleBarIconURL.clear();
        rPanelDescriptor.msHelpURL = getString(aChildNode, "HelpURL");
        rPanelDescriptor.msImplementationURL = rsNodeName;
        rPanelDescriptor.mnOrderIndex = 100000 + nReadIndex;
        rPanelDescriptor.mbShowForReadOnlyDocuments = false;
        rPanelDescriptor.mbWantsCanvas = false;
        rPanelDescriptor.maContextList.AddContextDescription(Context(sModuleName, OUString("any")), true, OUString());
    }

    // When there where invalid nodes then we have to adapt the size
    // of the deck and panel vectors.
    if (nDeckWriteIndex < maDecks.size())
        maDecks.resize(nDeckWriteIndex);
    if (nPanelWriteIndex < maPanels.size())
        maPanels.resize(nPanelWriteIndex);
}


utl::OConfigurationTreeRoot SidebarResourceManager::GetLegacyAddonRootNode (const OUString& rsModuleName)
{
    try
    {
        const Reference<XComponentContext> xContext(comphelper::getProcessComponentContext());
        const Reference<frame::XModuleManager2> xModuleAccess = frame::ModuleManager::create(xContext);
        const comphelper::NamedValueCollection aModuleProperties(xModuleAccess->getByName(rsModuleName));
        const OUString sWindowStateRef(aModuleProperties.getOrDefault(
                                       "ooSetupFactoryWindowStateConfigRef",
                                       OUString()));

        OUStringBuffer aPathComposer;
        aPathComposer.append("org.openoffice.Office.UI.");
        aPathComposer.append(sWindowStateRef);
        aPathComposer.append("/UIElements/States");

        return utl::OConfigurationTreeRoot(xContext, aPathComposer.makeStringAndClear(), false);
    }
    catch (const Exception&)
    {
        DBG_UNHANDLED_EXCEPTION();
    }

    return utl::OConfigurationTreeRoot();
}



} } // end of namespace sfx2::sidebar

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
