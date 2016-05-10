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

#include <sfx2/notebookbar/NotebookbarResourceManager.hxx>
#include <sfx2/notebookbar/NotebookbarDeckDescriptor.hxx>
#include <sfx2/notebookbar/NotebookbarPanelDescriptor.hxx>

#include <comphelper/processfactory.hxx>

using namespace css;
using namespace css::uno;

namespace sfx2 { namespace notebookbar {


NotebookbarResourceManager::NotebookbarResourceManager()
    : ResourceManager()
{
    ReadDeckList();
    ReadPanelList();
}

NotebookbarResourceManager::~NotebookbarResourceManager()
{
}

void NotebookbarResourceManager::ReadDeckList()
{
    const utl::OConfigurationTreeRoot aDeckRootNode(
                                        comphelper::getProcessComponentContext(),
                                        OUString("org.openoffice.Office.UI.Notebookbar/Content/DeckList"),
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

        NotebookbarDeckDescriptor& rDeckDescriptor (maDecks[nWriteIndex++]);

        rDeckDescriptor.msTitle = getString(aDeckNode, "Title");
        rDeckDescriptor.msId = getString(aDeckNode, "Id");
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

void NotebookbarResourceManager::ReadPanelList()
{
    const utl::OConfigurationTreeRoot aPanelRootNode(
                                        comphelper::getProcessComponentContext(),
                                        OUString("org.openoffice.Office.UI.Notebookbar/Content/PanelList"),
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

        NotebookbarPanelDescriptor& rPanelDescriptor (maPanels[nWriteIndex++]);

        rPanelDescriptor.msTitle = getString(aPanelNode, "Title");
        rPanelDescriptor.msId = getString(aPanelNode, "Id");
        rPanelDescriptor.msDeckId = getString(aPanelNode, "DeckId");
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


} } // end of namespace sfx2::notebookbar

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
