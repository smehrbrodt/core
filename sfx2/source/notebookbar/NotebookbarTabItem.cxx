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

#include <sfx2/notebookbar/NotebookbarTabItem.hxx>

#include <sfx2/abstractbar/DrawHelper.hxx>
#include <sfx2/abstractbar/Paint.hxx>
#include <sfx2/abstractbar/Tools.hxx>

#include <sfx2/abstractbar/Theme.hxx>

using namespace css;
using namespace css::uno;
using namespace sfx2::abstractbar;

namespace sfx2 { namespace notebookbar {

NotebookbarTabItem::NotebookbarTabItem (vcl::Window* pParentWindow)
    : RadioButton(pParentWindow),
      mbIsLeftButtonDown(false),
      mePaintType(PT_Theme)
{
    SetStyle(GetStyle() | WB_TABSTOP | WB_DIALOGCONTROL | WB_NOPOINTERFOCUS);
    SetBackground(Theme::GetPaint(Theme::Paint_TabBarBackground).GetWallpaper());
#ifdef DEBUG
    SetText(OUString("TabItem"));
#endif
}

void NotebookbarTabItem::Paint(vcl::RenderContext& rRenderContext, const Rectangle& rUpdateArea)
{
    switch (mePaintType)
    {
        case PT_Theme:
        default:
        {
            const bool bIsSelected (IsChecked());
            const bool bIsHighlighted (IsMouseOver() || HasFocus());
            Rectangle aSize = Rectangle(Point(0,0), GetSizePixel());
            DrawHelper::DrawRoundedRectangle(
                rRenderContext,
                aSize,
                0,
                bIsHighlighted||bIsSelected
                    ? Theme::GetColor(Theme::Color_TabItemBorder)
                    : Color(0xffffffff),
                bIsHighlighted
                    ? Theme::GetPaint(Theme::Paint_TabItemBackgroundHighlight)
                    : Theme::GetPaint(Theme::Paint_TabItemBackgroundNormal));

            rRenderContext.DrawText(aSize, GetText(), DrawTextFlags::Center | DrawTextFlags::VCenter);
            break;
        }
        case PT_Native:
            Button::Paint(rRenderContext, rUpdateArea);
            break;
    }
}

void NotebookbarTabItem::MouseMove(const MouseEvent& rEvent)
{
    if (rEvent.IsEnterWindow() || rEvent.IsLeaveWindow())
        Invalidate();
    RadioButton::MouseMove(rEvent);
}

void NotebookbarTabItem::MouseButtonDown(const MouseEvent& rMouseEvent)
{
    if (rMouseEvent.IsLeft())
    {
        mbIsLeftButtonDown = true;
        CaptureMouse();
        Invalidate();
    }
}

void NotebookbarTabItem::MouseButtonUp(const MouseEvent& rMouseEvent)
{
    if (IsMouseCaptured())
        ReleaseMouse();

    if (rMouseEvent.IsLeft())
    {
        if (mbIsLeftButtonDown)
        {
            Check();
            Click();
            GetParent()->Invalidate();
        }
    }
    if (mbIsLeftButtonDown)
    {
        mbIsLeftButtonDown = false;
        Invalidate();
    }
}

} } // end of namespace sfx2::abstractbar

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
