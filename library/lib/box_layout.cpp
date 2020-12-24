/*
    Borealis, a Nintendo Switch UI Library
    Copyright (C) 2019  natinusala
    Copyright (C) 2019  WerWolv
    Copyright (C) 2019  p-sam

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

#include <math.h>
#include <stdio.h>

#include <borealis/animations.hpp>
#include <borealis/application.hpp>
#include <borealis/box_layout.hpp>
#include <borealis/logger.hpp>
#include <iterator>

namespace brls
{

BoxLayout::BoxLayout(BoxLayoutOrientation orientation, size_t defaultFocus)
    : orientation(orientation)
    , originalDefaultFocus(defaultFocus)
    , defaultFocusedIndex(defaultFocus)
{
}

void BoxLayout::draw(NVGcontext* vg, int x, int y, unsigned width, unsigned height, Style* style, FrameContext* ctx)
{
    // Draw children
    for (BoxLayoutChild* child : this->children)
        child->view->frame(ctx);
}

void BoxLayout::setGravity(BoxLayoutGravity gravity)
{
    this->gravity = gravity;
    this->invalidate();
}

void BoxLayout::setSpacing(unsigned spacing)
{
    this->spacing = spacing;
    this->invalidate();
}

unsigned BoxLayout::getSpacing()
{
    return this->spacing;
}

void BoxLayout::setMargins(unsigned top, unsigned right, unsigned bottom, unsigned left)
{
    this->marginBottom = bottom;
    this->marginLeft   = left;
    this->marginRight  = right;
    this->marginTop    = top;
    this->invalidate();
}

void BoxLayout::setMarginBottom(unsigned bottom)
{
    this->marginBottom = bottom;
    this->invalidate();
}

size_t BoxLayout::getViewsCount()
{
    return this->children.size();
}

View* BoxLayout::getDefaultFocus()
{
    printf("BoxLayout::getDefaultFocus %p\n", this);
    // Focus default focus first
    if (this->defaultFocusedIndex < this->children.size())
    {
        View* newFocus = (*std::next(this->children.begin(), this->defaultFocusedIndex))->view->getDefaultFocus();
        if (newFocus)
            return newFocus;
    }

    // Fallback to finding the first focusable view
    for (BoxLayoutChild* child : this->children)
    {
        View* newFocus = child->view->getDefaultFocus();
        if (newFocus)
            return newFocus;
    }
    return nullptr;
}

View* BoxLayout::getNextFocus(FocusDirection direction, View* currentView)
{
    printf("direction %d\n", direction);
    // Return nullptr immediately if focus direction mismatches the layout direction
    if ((this->orientation == BoxLayoutOrientation::HORIZONTAL && direction != FocusDirection::LEFT && direction != FocusDirection::RIGHT) || (this->orientation == BoxLayoutOrientation::VERTICAL && direction != FocusDirection::UP && direction != FocusDirection::DOWN))
    {
        printf("ret nullptr\n");
        return nullptr;
    }

    // Traverse the children
    size_t offset = 1; // which way are we going in the children list

    if ((this->orientation == BoxLayoutOrientation::HORIZONTAL && direction == FocusDirection::LEFT) || (this->orientation == BoxLayoutOrientation::VERTICAL && direction == FocusDirection::UP))
    {
        offset = -1;
    }

    View* currentFocus = nullptr;

    BoxLayoutChildIterator currentFocusIterator = *((BoxLayoutChildIterator*)currentView->getParentUserData());
    currentFocusIterator                        = std::next(currentFocusIterator, offset);

    while (!currentFocus && currentFocusIterator != this->children.end())
    {
        currentFocus = (*currentFocusIterator)->view->getDefaultFocus();
        printf("Offset %d to %p\n", offset, currentFocusIterator);
        currentFocusIterator = std::next(currentFocusIterator, offset);
    }
    printf("ret %p\n", currentFocus);
    return currentFocus;
}

View* BoxLayout::removeView(BoxLayoutChildIterator childIterator, bool free)
{
    View* view = (*childIterator)->view;
    printf("BoxLayout::removeView %p free: %d childIterator %p\n", view, free, childIterator);
    view->willDisappear(true);
    if (free)
    {
        delete view;
        view = nullptr;
    }
    this->children.erase(childIterator);
    delete (*childIterator);

    return view;
}

View* BoxLayout::removeView(int index, bool free)
{
    printf("BoxLayout::removeView from index %d\n", index);
    BoxLayoutChildIterator childIterator = std::next(this->children.begin(), index);
    return this->removeView(childIterator, free);
}

void BoxLayout::clear(bool free)
{
    printf("BoxLayout::clear %d free\n", free);
    while (!this->children.empty())
        this->removeView(0, free);
}

void BoxLayout::layout(NVGcontext* vg, Style* style, FontStash* stash)
{
    // Vertical orientation
    if (this->orientation == BoxLayoutOrientation::VERTICAL)
    {
        unsigned entriesHeight = 0;
        int yAdvance           = this->y + this->marginTop;

        for (BoxLayoutChildIterator it = this->children.begin(); it != this->children.end(); ++it)
        {
            BoxLayoutChild* child = *it;
            unsigned childHeight  = child->view->getHeight();

            if (child->fill)
                child->view->setBoundaries(this->x + this->marginLeft,
                    yAdvance,
                    this->width - this->marginLeft - this->marginRight,
                    this->y + this->height - yAdvance - this->marginBottom);
            else
                child->view->setBoundaries(this->x + this->marginLeft,
                    yAdvance,
                    this->width - this->marginLeft - this->marginRight,
                    child->view->getHeight(false));

            child->view->invalidate(true); // call layout directly in case height is updated
            childHeight = child->view->getHeight();

            int spacing = (int)this->spacing;

            BoxLayoutChildIterator next_it = std::next(it);
            View* next                     = next_it != this->children.end() ? (*next_it)->view : nullptr;

            this->customSpacing(child->view, next, &spacing);

            if (child->view->isCollapsed())
                spacing = 0;

            if (!child->view->isHidden())
                entriesHeight += spacing + childHeight;

            yAdvance += spacing + childHeight;
        }

        // TODO: apply gravity

        // Update height if needed
        if (this->resize)
            this->setHeight(entriesHeight - spacing + this->marginTop + this->marginBottom);
    }
    // Horizontal orientation
    else if (this->orientation == BoxLayoutOrientation::HORIZONTAL)
    {
        // Layout
        int xAdvance = this->x + this->marginLeft;

        for (BoxLayoutChildIterator it = this->children.begin(); it != this->children.end(); ++it)
        {
            BoxLayoutChild* child = *it;
            unsigned childWidth   = child->view->getWidth();

            if (child->fill)
                child->view->setBoundaries(xAdvance,
                    this->y + this->marginTop,
                    this->x + this->width - xAdvance - this->marginRight,
                    this->height - this->marginTop - this->marginBottom);
            else
                child->view->setBoundaries(xAdvance,
                    this->y + this->marginTop,
                    childWidth,
                    this->height - this->marginTop - this->marginBottom);

            child->view->invalidate(true); // call layout directly in case width is updated
            childWidth = child->view->getWidth();

            int spacing = (int)this->spacing;

            BoxLayoutChildIterator next_it = std::next(it);
            View* next                     = next_it != this->children.end() ? (*next_it)->view : nullptr;

            this->customSpacing(child->view, next, &spacing);

            if (child->view->isCollapsed())
                spacing = 0;

            xAdvance += spacing + childWidth;
        }

        // Apply gravity
        // TODO: more efficient gravity implementation?
        if (!this->children.empty())
        {
            switch (this->gravity)
            {
                case BoxLayoutGravity::RIGHT:
                {
                    // Take the remaining empty space between the last view's
                    // right boundary and ours and push all views by this amount
                    BoxLayoutChildIterator prev_it = std::prev(this->children.end());
                    View* lastView                 = (*prev_it)->view;

                    unsigned lastViewRight = lastView->getX() + lastView->getWidth();
                    unsigned ourRight      = this->getX() + this->getWidth();

                    if (lastViewRight <= ourRight)
                    {
                        unsigned difference = ourRight - lastViewRight;

                        for (BoxLayoutChild* child : this->children)
                        {
                            View* view = child->view;
                            view->setBoundaries(
                                view->getX() + difference,
                                view->getY(),
                                view->getWidth(),
                                view->getHeight());
                            view->invalidate();
                        }
                    }

                    break;
                }
                default:
                    break;
            }
        }

        // TODO: update width if needed (introduce entriesWidth)
    }
}

void BoxLayout::setResize(bool resize)
{
    this->resize = resize;
    this->invalidate();
}

BoxLayoutChildIterator BoxLayout::addView(View* view, bool fill, bool resetState)
{
    BoxLayoutChild* child = new BoxLayoutChild();
    child->view           = view;
    child->fill           = fill;

    this->children.push_back(child);

    BoxLayoutChildIterator childIterator = std::prev(this->children.end());

    BoxLayoutChildIterator* userdata = (BoxLayoutChildIterator*)malloc(sizeof(BoxLayoutChildIterator));
    *userdata                        = childIterator;

    view->setParent(this, userdata);

    view->willAppear(resetState);
    this->invalidate();

    return childIterator;
}

View* BoxLayout::getChild(size_t index)
{
    return (*std::next(this->children.begin(), index))->view;
}

bool BoxLayout::isEmpty()
{
    return this->children.size() == 0;
}

bool BoxLayout::isChildFocused()
{
    return this->childFocused;
}

void BoxLayout::onChildFocusGained(View* child)
{
    this->childFocused = true;
    printf("BoxLayout::onChildFocusGained %p\n", child);
    // Remember focus if needed
    if (this->rememberFocus)
    {
        BoxLayoutChildIterator childIterator = *((BoxLayoutChildIterator*)child->getParentUserData());
        this->defaultFocusedIndex            = std::distance(this->children.begin(), childIterator);
        printf("defaultFocusedIndex %d\n", this->defaultFocusedIndex);
    }

    View::onChildFocusGained(child);
}

void BoxLayout::onChildFocusLost(View* child)
{
    this->childFocused = false;

    View::onChildFocusLost(child);
}

BoxLayout::~BoxLayout()
{
    for (std::list<BoxLayoutChild*>::reverse_iterator it = this->children.rbegin(); it != this->children.rend(); it++)
    {
        BoxLayoutChild* child = (*it);
        child->view->willDisappear(true);
        delete child->view;
        delete child;
    }
    this->children.clear();
}

void BoxLayout::willAppear(bool resetState)
{
    for (BoxLayoutChild* child : this->children)
        child->view->willAppear(resetState);
}

void BoxLayout::willDisappear(bool resetState)
{
    for (BoxLayoutChild* child : this->children)
        child->view->willDisappear(resetState);

    // Reset default focus to original one if needed
    if (this->rememberFocus)
        this->defaultFocusedIndex = this->originalDefaultFocus;
}

void BoxLayout::onWindowSizeChanged()
{
    for (BoxLayoutChild* child : this->children)
        child->view->onWindowSizeChanged();
}

void BoxLayout::setRememberFocus(bool remember)
{
    this->rememberFocus = remember;
}

} // namespace brls
