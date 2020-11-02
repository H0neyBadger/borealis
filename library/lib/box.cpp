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

#include <tinyxml2.h>

#include <borealis/application.hpp>
#include <borealis/box.hpp>
#include <cmath>

namespace brls
{

static YGFlexDirection getYGFlexDirection(Axis axis)
{
    switch (axis)
    {
        default:
        case Axis::ROW:
            return YGFlexDirectionRow;
        case Axis::COLUMN:
            return YGFlexDirectionColumn;
    }
}

Box::Box(Axis axis)
    : axis(axis)
{
    YGNodeStyleSetFlexDirection(this->ygNode, getYGFlexDirection(axis));

    // no need to invalidate if the box is empty and is not attached to any parent

    // Register XML attributes
    this->registerStringXMLAttribute("axis", [this](std::string value) {
        if (value == "row")
            this->setAxis(Axis::ROW);
        else if (value == "column")
            this->setAxis(Axis::COLUMN);
        else
            throw std::logic_error("Illegal value \"" + value + "\" for \"brls::Box\" attribute \"axis\"");
    });

    this->registerStringXMLAttribute("direction", [this](std::string value) {
        if (value == "inherit")
            this->setDirection(Direction::INHERIT);
        else if (value == "leftToRight")
            this->setDirection(Direction::LEFT_TO_RIGHT);
        else if (value == "rightToLeft")
            this->setDirection(Direction::RIGHT_TO_LEFT);
        else
            throw std::logic_error("Illegal value \"" + value + "\" for \"brls::Box\" attribute \"direction\"");
    });

    this->registerStringXMLAttribute("justifyContent", [this](std::string value) {
        if (value == "flexStart")
            this->setJustifyContent(JustifyContent::FLEX_START);
        else if (value == "center")
            this->setJustifyContent(JustifyContent::CENTER);
        else if (value == "flexEnd")
            this->setJustifyContent(JustifyContent::FLEX_END);
        else if (value == "spaceBetween")
            this->setJustifyContent(JustifyContent::SPACE_BETWEEN);
        else if (value == "spaceAround")
            this->setJustifyContent(JustifyContent::SPACE_AROUND);
        else if (value == "spaceEvenly")
            this->setJustifyContent(JustifyContent::SPACE_EVENLY);
        else
            throw std::logic_error("Illegal value \"" + value + "\" for \"brls::Box\" attribute \"justifyContent\"");
    });

    // Padding
    this->registerFloatXMLAttribute("paddingTop", [this](float value) {
        this->setPaddingTop(value);
    });

    this->registerFloatXMLAttribute("paddingRight", [this](float value) {
        this->setPaddingRight(value);
    });

    this->registerFloatXMLAttribute("paddingBottom", [this](float value) {
        this->setPaddingBottom(value);
    });

    this->registerFloatXMLAttribute("paddingLeft", [this](float value) {
        this->setPaddingLeft(value);
    });
}

Box::Box()
    : Box(Axis::ROW)
{
    // Empty ctor for XML
}

void Box::getCullingBounds(float* top, float* right, float* bottom, float* left)
{
    *top    = this->getY();
    *left   = this->getX();
    *right  = *left + this->getWidth();
    *bottom = *top + this->getHeight();
}

void Box::draw(NVGcontext* vg, float x, float y, float width, float height, Style style, FrameContext* ctx)
{
    for (View* child : this->children)
    {
        // Ensure that the child is in bounds before drawing it
        if (child->isCulled())
        {
            float childTop    = child->getY();
            float childLeft   = child->getX();
            float childRight  = childLeft + child->getWidth();
            float childBottom = childTop + child->getHeight();

            float top, right, bottom, left;
            this->getCullingBounds(&top, &right, &bottom, &left);

            if (
                childBottom < top || // too high
                childRight < left || // too far left
                childLeft > right || // too far right
                childTop > bottom // too low
            )
                continue;
        }

        child->frame(ctx);
    }
}

void Box::addView(View* view)
{
    // Add the view to our children and YGNode
    size_t position = YGNodeGetChildCount(this->ygNode);

    this->children.push_back(view);
    YGNodeInsertChild(this->ygNode, view->getYGNode(), position);

    // Allocate and set parent userdata
    size_t* userdata = (size_t*)malloc(sizeof(size_t));
    *userdata        = position;

    view->setParent(this, userdata);

    // Layout and events
    this->invalidate();
    view->willAppear();
}

void Box::setPadding(float top, float right, float bottom, float left)
{
    YGNodeStyleSetPadding(this->ygNode, YGEdgeTop, top);
    YGNodeStyleSetPadding(this->ygNode, YGEdgeRight, right);
    YGNodeStyleSetPadding(this->ygNode, YGEdgeBottom, bottom);
    YGNodeStyleSetPadding(this->ygNode, YGEdgeLeft, left);

    this->invalidate();
}

void Box::setPaddingTop(float top)
{
    YGNodeStyleSetPadding(this->ygNode, YGEdgeTop, top);
    this->invalidate();
}

void Box::setPaddingRight(float right)
{
    YGNodeStyleSetPadding(this->ygNode, YGEdgeRight, right);
    this->invalidate();
}

void Box::setPaddingBottom(float bottom)
{
    YGNodeStyleSetPadding(this->ygNode, YGEdgeBottom, bottom);
    this->invalidate();
}

void Box::setPaddingLeft(float left)
{
    YGNodeStyleSetPadding(this->ygNode, YGEdgeLeft, left);
    this->invalidate();
}

View* Box::getDefaultFocus()
{
    // Focus default focus first
    if (this->defaultFocusedIndex < this->children.size())
    {
        View* newFocus = this->children[this->defaultFocusedIndex]->getDefaultFocus();

        if (newFocus)
            return newFocus;
    }

    // Fallback to finding the first focusable view
    for (size_t i = 0; i < this->children.size(); i++)
    {
        View* newFocus = this->children[i]->getDefaultFocus();

        if (newFocus)
            return newFocus;
    }

    return nullptr;
}

View* Box::getNextFocus(FocusDirection direction, View* currentView)
{
    void* parentUserData = currentView->getParentUserData();

    // Return nullptr immediately if focus direction mismatches the box axis (clang-format refuses to split it in multiple lines...)
    if ((this->axis == Axis::ROW && direction != FocusDirection::LEFT && direction != FocusDirection::RIGHT) || (this->axis == Axis::COLUMN && direction != FocusDirection::UP && direction != FocusDirection::DOWN))
    {
        return nullptr;
    }

    // Traverse the children
    size_t offset = 1; // which way we are going in the children list

    if ((this->axis == Axis::ROW && direction == FocusDirection::LEFT) || (this->axis == Axis::COLUMN && direction == FocusDirection::UP))
    {
        offset = -1;
    }

    size_t currentFocusIndex = *((size_t*)parentUserData) + offset;
    View* currentFocus       = nullptr;

    while (!currentFocus && currentFocusIndex >= 0 && currentFocusIndex < this->children.size())
    {
        currentFocus = this->children[currentFocusIndex]->getDefaultFocus();
        currentFocusIndex += offset;
    }

    return currentFocus;
}

void Box::willAppear(bool resetState)
{
    for (View* child : this->children)
        child->willAppear(resetState);
}

void Box::willDisappear(bool resetState)
{
    for (View* child : this->children)
        child->willDisappear(resetState);
}

void Box::onWindowSizeChanged()
{
    for (View* child : this->children)
        child->onWindowSizeChanged();
}

std::vector<View*>& Box::getChildren()
{
    return this->children;
}

void Box::inflateFromXML(std::string xml)
{
    // Load XML
    tinyxml2::XMLDocument document;
    tinyxml2::XMLError error = document.Parse(xml.c_str());

    if (error != tinyxml2::XMLError::XML_SUCCESS)
        throw std::logic_error("Invalid XML: error " + std::to_string(error));

    // Ensure first element is a Box
    tinyxml2::XMLElement* root = document.RootElement();

    if (!root)
        throw std::logic_error("Invalid XML: no element found");

    if (std::string(root->Name()) != "brls::Box")
        throw std::logic_error("First XML element is " + std::string(root->Name()) + ", expected Box");

    // Apply attributes
    this->applyXMLAttributes(root);

    // Handle children
    for (tinyxml2::XMLElement* child = root->FirstChildElement(); child != nullptr; child = child->NextSiblingElement())
        Box::addView(View::createFromXMLElement(child)); // don't call handleXMLTag because this method is for user XMLs, don't call this->addView because it can be reimplemented
}

void Box::handleXMLTag(tinyxml2::XMLElement* element)
{
    this->addView(View::createFromXMLElement(element));
}

void Box::setAxis(Axis axis)
{
    YGNodeStyleSetFlexDirection(this->ygNode, getYGFlexDirection(axis));
    this->axis = axis;
    this->invalidate();
}

void Box::setDirection(Direction direction)
{
    // TODO: direction must flip focus too!

    switch (direction)
    {
        case Direction::INHERIT:
            YGNodeStyleSetDirection(this->ygNode, YGDirectionInherit);
            break;
        case Direction::LEFT_TO_RIGHT:
            YGNodeStyleSetDirection(this->ygNode, YGDirectionLTR);
            break;
        case Direction::RIGHT_TO_LEFT:
            YGNodeStyleSetDirection(this->ygNode, YGDirectionRTL);
            break;
    }

    this->invalidate();
}

void Box::setJustifyContent(JustifyContent justify)
{
    switch (justify)
    {
        case JustifyContent::FLEX_START:
            YGNodeStyleSetJustifyContent(this->ygNode, YGJustifyFlexStart);
            break;
        case JustifyContent::CENTER:
            YGNodeStyleSetJustifyContent(this->ygNode, YGJustifyCenter);
            break;
        case JustifyContent::FLEX_END:
            YGNodeStyleSetJustifyContent(this->ygNode, YGJustifyFlexEnd);
            break;
        case JustifyContent::SPACE_BETWEEN:
            YGNodeStyleSetJustifyContent(this->ygNode, YGJustifySpaceBetween);
            break;
        case JustifyContent::SPACE_AROUND:
            YGNodeStyleSetJustifyContent(this->ygNode, YGJustifySpaceAround);
            break;
        case JustifyContent::SPACE_EVENLY:
            YGNodeStyleSetJustifyContent(this->ygNode, YGJustifySpaceEvenly);
            break;
    }

    this->invalidate();
}

View* Box::createFromXMLElement(tinyxml2::XMLElement* element)
{
    return new Box();
}

} // namespace brls
