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
#include <sfx2/notebookbar/NotebookbarDockingWindow.hxx>
#include <sfx2/notebookbar/NotebookbarChildWindow.hxx>
#include <sfx2/notebookbar/NotebookbarController.hxx>

#include <sfx2/bindings.hxx>
#include <sfx2/dispatch.hxx>
#include <tools/link.hxx>

using namespace css;
using namespace css::uno;

namespace sfx2 { namespace notebookbar {

NotebookbarDockingWindow::NotebookbarDockingWindow(SfxBindings* pSfxBindings, NotebookbarChildWindow& rChildWindow,
                                           vcl::Window* pParentWindow, WinBits nBits)
    : SfxDockingWindow(pSfxBindings, &rChildWindow, pParentWindow, nBits)
    , mpNotebookbarController()
{
    // Get the XFrame from the bindings.
    if (pSfxBindings==nullptr || pSfxBindings->GetDispatcher()==nullptr)
    {
        OSL_ASSERT(pSfxBindings!=nullptr);
        OSL_ASSERT(pSfxBindings->GetDispatcher()!=nullptr);
    }
    else
    {
        const SfxViewFrame* pViewFrame = pSfxBindings->GetDispatcher()->GetFrame();
        const SfxFrame& rFrame = pViewFrame->GetFrame();
        mpNotebookbarController.set(new sfx2::notebookbar::NotebookbarController(this, rFrame.GetFrameInterface()));
    }
}

NotebookbarDockingWindow::~NotebookbarDockingWindow()
{
    disposeOnce();
}

void NotebookbarDockingWindow::dispose()
{
    DoDispose();
    SfxDockingWindow::dispose();
}

void NotebookbarDockingWindow::DoDispose()
{
    Reference<lang::XComponent> xComponent (static_cast<XWeak*>(mpNotebookbarController.get()), UNO_QUERY);
    mpNotebookbarController.clear();
    if (xComponent.is())
    {
        xComponent->dispose();
    }
}

void NotebookbarDockingWindow::GetFocus()
{
    if (mpNotebookbarController.is())
        mpNotebookbarController->GetFocusManager().GrabFocus();
    else
        SfxDockingWindow::GetFocus();
}

// fdo#87217
bool NotebookbarDockingWindow::Close()
{
    return SfxDockingWindow::Close();
}

SfxChildAlignment NotebookbarDockingWindow::CheckAlignment (
    SfxChildAlignment eCurrentAlignment,
    SfxChildAlignment eRequestedAlignment)
{
    switch (eRequestedAlignment)
    {
        case SfxChildAlignment::TOP:
        case SfxChildAlignment::HIGHESTTOP:
        case SfxChildAlignment::LOWESTTOP:
        case SfxChildAlignment::BOTTOM:
        case SfxChildAlignment::LOWESTBOTTOM:
        case SfxChildAlignment::HIGHESTBOTTOM:
            return eCurrentAlignment;

        case SfxChildAlignment::LEFT:
        case SfxChildAlignment::RIGHT:
        case SfxChildAlignment::FIRSTLEFT:
        case SfxChildAlignment::LASTLEFT:
        case SfxChildAlignment::FIRSTRIGHT:
        case SfxChildAlignment::LASTRIGHT:
            return eRequestedAlignment;

        default:
            return eRequestedAlignment;
    }
}

} } // end of namespace sfx2::notebookbar

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
