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
#ifndef INCLUDED_SFX2_SOURCE_NOTEBOOKBAR_NOTEBOOKBARDOCKINGWINDOW_HXX
#define INCLUDED_SFX2_SOURCE_NOTEBOOKBAR_NOTEBOOKBARDOCKINGWINDOW_HXX

#include <sfx2/dockwin.hxx>

#include <rtl/ref.hxx>

namespace sfx2 { namespace notebookbar {

class NotebookbarChildWindow;

class NotebookbarController;

class NotebookbarDockingWindow : public SfxDockingWindow
{
public:
    NotebookbarDockingWindow(SfxBindings* pBindings, NotebookbarChildWindow& rChildWindow,
                             vcl::Window* pParent, WinBits nBits);
    virtual ~NotebookbarDockingWindow();
    virtual void dispose() override;

    virtual bool Close() override;

protected:
    // Window overridables
    virtual void GetFocus() override;

    virtual SfxChildAlignment CheckAlignment (
        SfxChildAlignment eCurrentAlignment,
        SfxChildAlignment eRequestedAlignment) override;

private:
    ::rtl::Reference<sfx2::notebookbar::NotebookbarController> mpNotebookbarController;

    void DoDispose();
};

} } // end of namespace sfx2::notebookbar


#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
