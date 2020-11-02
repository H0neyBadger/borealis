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

#include <borealis/view.hpp>

namespace brls
{

enum class JustifyContent
{
    FLEX_START,
    CENTER,
    FLEX_END,
    SPACE_BETWEEN,
    SPACE_AROUND,
    SPACE_EVENLY,
};

enum class Axis
{
    ROW,
    COLUMN,
};

enum class Direction
{
    INHERIT,
    LEFT_TO_RIGHT,
    RIGHT_TO_LEFT,
};

// Generic FlexBox layout
class Box : public View
{
  public:
    Box(Axis flexDirection);
    Box();
    // TODO: dtor, use asan to check for leaks once it's done

    void draw(NVGcontext* vg, float x, float y, float width, float height, Style style, FrameContext* ctx) override;
    View* getDefaultFocus() override;
    View* getNextFocus(FocusDirection direction, View* currentView) override;
    void willAppear(bool resetState) override;
    void willDisappear(bool resetState) override;
    void onWindowSizeChanged() override;

    static View* createFromXMLElement(tinyxml2::XMLElement* element);

    /**
     * Adds a view to this Box
     */
    virtual void addView(View* view);

    /**
     * Sets the padding of the view, aka the internal space to give
     * between this view boundaries and its children.
     *
     * Only does one layout pass instead of four when using the four methods separately.
     */
    void setPadding(float top, float right, float bottom, float left);

    /**
     * Sets the padding of the view, aka the internal space to give
     * between this view boundaries and its children.
     */
    void setPaddingTop(float top);

    /**
     * Sets the padding of the view, aka the internal space to give
     * between this view boundaries and its children.
     */
    void setPaddingRight(float right);

    /**
     * Sets the padding of the view, aka the internal space to give
     * between this view boundaries and its children.
     */
    void setPaddingBottom(float bottom);

    /**
     * Sets the padding of the view, aka the internal space to give
     * between this view boundaries and its children.
     */
    void setPaddingLeft(float left);

    /**
     * Sets the children alignment along the Box axis.
     *
     * Default is FLEX_START.
     */
    void setJustifyContent(JustifyContent justify);

    /**
     * Sets the direction of the box, aka place the views
     * left to right or right to left (flips the children).
     *
     * Default is INHERIT.
     */
    void setDirection(Direction direction);

    void setAxis(Axis axis);

    std::vector<View*>& getChildren();

    /**
     * Returns the bounds used for culling children.
     */
    virtual void getCullingBounds(float* top, float* right, float* bottom, float* left);

  private:
    Axis axis;

    std::vector<View*> children;

    size_t defaultFocusedIndex = 0; // TODO: rewrite that to call a view instead of putting it in the ctor (addView(View*, defaultFocus))

    // TODO: rememberFocus

  protected:
    /**
     * Inflates the Box with the content of the given XML file.
     *
     * The root tag MUST be a brls::Box, corresponding to the inflated Box itself. Its
     * attributes will be applied to the Box.
     *
     * Each child node in the root brls::Box will be treated as a view and added
     * as a child of the Box.
     */
    void inflateFromXML(std::string xml);

    // TODO: inflate from file too

    /**
     * Handles a child XML tag.
     *
     * By default, calls createFromXMLElement() and adds the result
     * to the children of the Box.
     */
    void handleXMLTag(tinyxml2::XMLElement* element) override;
};

} // namespace brls
