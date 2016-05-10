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
#ifndef INCLUDED_SFX2_SOURCE_NOTEBOOKBAR_NOTEBOOKBARRESOURCEMANAGER_HXX
#define INCLUDED_SFX2_SOURCE_NOTEBOOKBAR_NOTEBOOKBARRESOURCEMANAGER_HXX

#include <sfx2/abstractbar/ResourceManager.hxx>


namespace sfx2 { namespace notebookbar {


/** Read the content of the Notebookbar.xcu file and provide access
    methods so that the sidebar can easily decide which content panels
    to display for a certain context.
*/
class NotebookbarResourceManager
    : public sfx2::abstractbar::ResourceManager
{
public:
    virtual void ReadDeckList() override;
    virtual void ReadPanelList() override;

    NotebookbarResourceManager();
    ~NotebookbarResourceManager();
};

} } // end of namespace sfx2::abstractbar

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
