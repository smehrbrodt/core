/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef INCLUDED_SFX2_SOURCE_NOTEBOOKBAR_NOTEBOOKBARDECK_HXX
#define INCLUDED_SFX2_SOURCE_NOTEBOOKBAR_NOTEBOOKBARDECK_HXX

#include <sfx2/dllapi.h>
#include <sfx2/abstractbar/TabBar.hxx>
#include <sfx2/abstractbar/Deck.hxx>

namespace sfx2 { namespace abstractbar {
class DeckDescriptor;
}}

namespace sfx2 { namespace notebookbar {

class SFX2_DLLPUBLIC NotebookbarDeck
    : public sfx2::abstractbar::Deck
{
public:
    NotebookbarDeck (
        const sfx2::abstractbar::DeckDescriptor& rDeckDescriptor,
         vcl::Window* pParentWindow,
         const std::function<void()>& rCloserAction);
    virtual ~NotebookbarDeck();

    virtual void Paint(vcl::RenderContext& rRenderContext, const Rectangle& rUpdateArea) override;
    virtual void RequestLayout() override;
};


} } // end of namespace sfx2::notebookbar

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
