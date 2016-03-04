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

#include <sfx2/notebookbar/NotebookbarDeck.hxx>
#include <sfx2/notebookbar/NotebookbarDeckLayouter.hxx>

#include <sfx2/abstractbar/DrawHelper.hxx>
#include <sfx2/abstractbar/Theme.hxx>

using namespace sfx2::abstractbar;

namespace sfx2 { namespace notebookbar {

NotebookbarDeck::NotebookbarDeck (const DeckDescriptor& rDeckDescriptor,
                                  vcl::Window* pParentWindow,
                                  const std::function<void()>& rCloserAction)
    : Deck(rDeckDescriptor, pParentWindow, rCloserAction)
{
}

NotebookbarDeck::~NotebookbarDeck()
{
}

void NotebookbarDeck::Paint(vcl::RenderContext& rRenderContext, const Rectangle& /*rUpdateArea*/)
{
    fprintf(stderr, "NotebookbarDeck::Paint\n");
    const Size aWindowSize (GetSizePixel());
    const SvBorder aPadding(Theme::GetInteger(Theme::Int_DeckLeftPadding),
                            Theme::GetInteger(Theme::Int_DeckTopPadding),
                            Theme::GetInteger(Theme::Int_DeckRightPadding),
                            Theme::GetInteger(Theme::Int_DeckBottomPadding));

    // Paint deck background outside the border.
    Rectangle aBox(0, 0, aWindowSize.Width() - 1, aWindowSize.Height() - 1);
    DrawHelper::DrawBorder(rRenderContext, aBox, aPadding,
                           Theme::GetPaint(Theme::Paint_DeckBackground),
                           Theme::GetPaint(Theme::Paint_DeckBackground));

    // Paint the border.
    const int nBorderSize(Theme::GetInteger(Theme::Int_DeckBorderSize));
    aBox.Left() += aPadding.Left();
    aBox.Top() += aPadding.Top();
    aBox.Right() -= aPadding.Right();
    aBox.Bottom() -= aPadding.Bottom();
    const sfx2::abstractbar::Paint& rHorizontalBorderPaint(Theme::GetPaint(Theme::Paint_HorizontalBorder));
    DrawHelper::DrawBorder(rRenderContext, aBox,
                           SvBorder(nBorderSize, nBorderSize, nBorderSize, nBorderSize),
                           rHorizontalBorderPaint,
                           Theme::GetPaint(Theme::Paint_VerticalBorder));
}

void NotebookbarDeck::RequestLayout()
{
    fprintf(stderr, "NotebookbarDeck::requestLayout\n");
    mnMinimalWidth = 0;
    NotebookbarDeckLayouter::LayoutDeck(GetContentArea(), mnMinimalWidth, maPanels,
                                        *mpScrollClipWindow, *mpScrollContainer, *mpFiller);
}

} } // end of namespace sfx2::notebookbar

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
