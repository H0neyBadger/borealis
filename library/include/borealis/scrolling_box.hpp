/*
    Borealis, a Nintendo Switch UI Library
    Copyright (C) 2020  natinusala

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <borealis/box.hpp>

namespace brls
{

// TODO: horizontal scrolling, either in here or in a separate class (like Android has)
// TODO: Scrollbar

// Scrolling works by having a 0-height "flat" box. All of the
// ScrollingBox items are added to the flatbox, and since it has
// a height of 0 the items are guaranteed to overflow.
// To scroll, we adjust the relative top position of the flatbox.
class ScrollingBoxInternalBox : public Box
{
  public:
    ScrollingBoxInternalBox();

    void getCullingBounds(float* top, float* right, float* bottom, float* left) override;
};

// A Box that can scroll vertically if its content
// overflows
class ScrollingBox : public Box
{
  public:
    ScrollingBox();

    void draw(NVGcontext* vg, float x, float y, float width, float height, Style style, FrameContext* ctx) override;
    void onChildFocusGained(View* directChild, View* focusedView) override;
    void willAppear(bool resetState) override;

    void addView(View* view) override;

  private:
    ScrollingBoxInternalBox* flatBox = nullptr;

    bool updateScrollingOnNextFrame = false;

    float middleY = 0; // y + height/2
    float bottomY = 0; // y + height

    float scrollY = 0.0f; // from 0.0f to 1.0f, in % of content view height

    void prebakeScrolling();
    bool updateScrolling(bool animated);
    void startScrolling(bool animated, float newScroll);
    void scrollAnimationTick();

    float getScrollingAreaTopBoundary();
    float getScrollingAreaHeight();

    float getContentHeight();
};

} // namespace brls
