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

#include <borealis/animations.hpp>
#include <borealis/application.hpp>
#include <borealis/scrolling_box.hpp>

namespace brls
{

// TODO: streamline TransitionAnimation, it should be generic to activities
// TODO: does the fade in animation work?
// TODO: cleanup background, make it an XML attribute
// TODO: use max-width and max-height=100% for labels - set their width as measured text size (custom measure function?), then apply ellipsis on the final measured label layout size
// TODO: rework ScrollingBox to only accept one child w/ shrink at 0 and grow at 1 (it should expand horizontally like the flatbox), no flatbox required
// TODO: add registerTag for custom tags instead of overriding handleTag
// TODO: use fmt to format the ugly XML logic_errors
// TODO: what to do with setShrink? remove it if the setminwidth/setminheight is still there in setwidth / setheight

// TODO: it's time to do proper documentation using doxygen or whatever

// TODO: make sure all useless invalidate calls are removed
// TODO: solve all TODOs in the diff
// TODO: clean the other TODOs

// TODO: incomment everything in borealis.hpp
// TODO: cleanup the old example files
// TODO: copyright on all changed files
// TODO: clean the project board and rewrite documentation once this is out

ScrollingBox::ScrollingBox()
    : Box(Axis::COLUMN)
{
    // Create and add the flat box
    this->flatBox = new ScrollingBoxInternalBox();
    this->flatBox->setHeight(0);

    YGNodeStyleSetPositionType(this->flatBox->getYGNode(), YGPositionTypeRelative);

    Box::addView(this->flatBox);
}

void ScrollingBox::draw(NVGcontext* vg, float x, float y, float width, float height, Style style, FrameContext* ctx)
{
    // Update scrolling - try until it works
    if (this->updateScrollingOnNextFrame && this->updateScrolling(false))
        this->updateScrollingOnNextFrame = false;

    // Enable scissoring
    nvgSave(vg);
    float scrollingTop    = this->getScrollingAreaTopBoundary();
    float scrollingHeight = this->getScrollingAreaHeight();
    nvgScissor(vg, x, scrollingTop, this->getWidth(), scrollingHeight);

    // Draw children
    Box::draw(vg, x, y, width, height, style, ctx);

    //Disable scissoring
    nvgRestore(vg);
}

void ScrollingBox::addView(View* view)
{
    // Only allow adding children with explicit height because we need
    // to know the total height for the bottom scroll boundary
    if (YGFloatIsUndefined(YGNodeStyleGetHeight(view->getYGNode()).value))
        throw std::logic_error("Cannot use brls::View::AUTO as height in a ScrollingBox child");

    this->flatBox->addView(view);
}

float ScrollingBox::getScrollingAreaTopBoundary()
{
    return this->getY() + ntz(YGNodeLayoutGetPadding(this->ygNode, YGEdgeTop));
}

float ScrollingBox::getScrollingAreaHeight()
{
    return this->getHeight() - ntz(YGNodeLayoutGetPadding(this->ygNode, YGEdgeTop)) - ntz(YGNodeLayoutGetPadding(this->ygNode, YGEdgeBottom));
}

void ScrollingBox::willAppear(bool resetState)
{
    this->prebakeScrolling();

    // First scroll all the way to the top
    // then wait for the first frame to scroll
    // to the selected view if needed (only known then)
    if (resetState)
    {
        this->startScrolling(false, 0.0f);
        this->updateScrollingOnNextFrame = true; // focus may have changed since
    }

    Box::willAppear(resetState);
}

void ScrollingBox::prebakeScrolling()
{
    // Prebaked values for scrolling
    float y      = this->getScrollingAreaTopBoundary();
    float height = this->getScrollingAreaHeight();

    this->middleY = y + height / 2;
    this->bottomY = y + height;
}

void ScrollingBox::startScrolling(bool animated, float newScroll)
{
    if (newScroll == this->scrollY)
        return;

    menu_animation_ctx_tag tag = (uintptr_t) & this->scrollY;
    menu_animation_kill_by_tag(&tag);

    if (animated)
    {
        Style style = Application::getStyle();

        menu_animation_ctx_entry_t entry;
        entry.cb           = [](void* userdata) {};
        entry.duration     = style["brls/animations_durations/highlight"];
        entry.easing_enum  = EASING_OUT_QUAD;
        entry.subject      = &this->scrollY;
        entry.tag          = tag;
        entry.target_value = newScroll;
        entry.tick         = [this](void* userdata) { this->scrollAnimationTick(); };
        entry.userdata     = nullptr;

        menu_animation_push(&entry);
    }
    else
    {
        this->scrollY = newScroll;
    }

    this->invalidate();
}

float ScrollingBox::getContentHeight()
{
    std::vector<View*> flatBoxChildren = this->flatBox->getChildren();

    if (flatBoxChildren.size() == 0)
        return 0.0f;

    View* firstChild = flatBoxChildren[0];
    View* lastChild  = flatBoxChildren[flatBoxChildren.size() - 1];

    return YGNodeLayoutGetTop(lastChild->getYGNode()) + // last child Y
        (YGNodeLayoutGetHeight(lastChild->getYGNode()) + ntz(YGNodeLayoutGetMargin(lastChild->getYGNode(), YGEdgeBottom))) - // last child height + bottom margin
        YGNodeLayoutGetTop(firstChild->getYGNode()); // top child Y (already includes top margin)
}

void ScrollingBox::scrollAnimationTick()
{
    YGNodeStyleSetPosition(this->flatBox->getYGNode(), YGEdgeTop, -(this->scrollY * this->getContentHeight()));
    this->invalidate();
}

void ScrollingBox::onChildFocusGained(View* directChild, View* focusedView)
{
    // Start scrolling
    this->updateScrolling(true);

    Box::onChildFocusGained(directChild, focusedView);
}

bool ScrollingBox::updateScrolling(bool animated)
{
    if (this->flatBox->getChildren().size() == 0)
        return false;

    float contentHeight = this->getContentHeight();

    View* focusedView                  = Application::getCurrentFocus();
    int currentSelectionMiddleOnScreen = focusedView->getY() + focusedView->getHeight() / 2;
    float newScroll                    = -(this->scrollY * contentHeight) - (currentSelectionMiddleOnScreen - this->middleY);

    // Bottom boundary
    if (this->getScrollingAreaTopBoundary() + newScroll + contentHeight < this->bottomY)
        newScroll = this->getScrollingAreaHeight() - contentHeight;

    // Top boundary
    if (newScroll > 0.0f)
        newScroll = 0.0f;

    // Apply 0.0f -> 1.0f scale
    newScroll = abs(newScroll) / contentHeight;

    //Start animation
    this->startScrolling(animated, newScroll);

    return true;
}

ScrollingBoxInternalBox::ScrollingBoxInternalBox()
    : Box(Axis::COLUMN)
{
    // Never cull the flatbox itself since it will be out of bounds
    this->setCulled(false);
}

void ScrollingBoxInternalBox::getCullingBounds(float* top, float* right, float* bottom, float* left)
{
    // Pipe the culling bounds to the containing ScrollingBox
    // Because the flatbox height is 0, everything would always be culled
    this->getParent()->getCullingBounds(top, right, bottom, left);
}

} // namespace brls
