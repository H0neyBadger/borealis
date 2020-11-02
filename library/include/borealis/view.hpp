/*
    Borealis, a Nintendo Switch UI Library
    Copyright (C) 2019-2020  natinusala
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

#pragma once

#include <libretro-common/features/features_cpu.h>
#include <stdio.h>
#include <tinyxml2.h>
#include <yoga/YGNode.h>

#include <borealis/actions.hpp>
#include <borealis/event.hpp>
#include <borealis/frame_context.hpp>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace brls
{

// Focus direction when navigating
enum class FocusDirection
{
    UP,
    DOWN,
    LEFT,
    RIGHT
};

// View background
enum class ViewBackground
{
    NONE,
    SIDEBAR,
    BACKDROP
};

// View visibility
enum class Visibility
{
    VISIBLE,    // the view is visible
    INVISIBLE,  // the view is invisible but still takes some space
    GONE,       // the view is invisible and doesn't take any space
};

// The animation to play when
// pushing / popping an activity or
// showing / hiding a view.
enum class TransitionAnimation
{
    FADE, // the old activity fades away and the new one fades in
    SLIDE_LEFT, // the old activity slides out to the left and the new one slides in from the right
    SLIDE_RIGHT, // inverted SLIDE_LEFT
};

extern const NVGcolor transparent;

class View;
class Box;

typedef Event<View*> GenericEvent;
typedef Event<> VoidEvent;

typedef std::function<void(void)> AutoAttributeHandler;
typedef std::function<void(int)> IntAttributeHandler;
typedef std::function<void(float)> FloatAttributeHandler;
typedef std::function<void(std::string)> StringAttributeHandler;
typedef std::function<void(NVGcolor)> ColorAttributeHandler;

/**
 * Some YG values are NAN if not set, wrecking our
 * calculations if we use them as they are
 */
float ntz(float value);

// Superclass for all the other views
// Lifecycle of a view is :
//   new -> [willAppear -> willDisappear] -> delete
//
// Users have do to the new, the rest of the lifecycle is taken
// care of by the library
//
// willAppear and willDisappear can be called zero or multiple times
// before deletion (in case of a TabLayout for instance)
class View
{
  private:
    ViewBackground background = ViewBackground::NONE;

    void drawBackground(NVGcontext* vg, FrameContext* ctx, Style style);
    void drawHighlight(NVGcontext* vg, Theme theme, float alpha, Style style, bool background);

    float highlightAlpha = 0.0f;

    bool highlightShaking = false;
    retro_time_t highlightShakeStart;
    FocusDirection highlightShakeDirection;
    float highlightShakeAmplitude;

    bool fadeIn          = false; // is the fade in animation running?
    bool inFadeAnimation = false; // is any fade animation running?

    Theme* themeOverride = nullptr;

    bool hidden = false;

    std::vector<Action> actions;

    /**
     * Parent user data, typically the index of the view
     * in the internal layout structure
     */
    void* parentUserdata = nullptr;

    bool culled = true; // will be culled by the parent Box, if any

    std::map<std::string, AutoAttributeHandler> autoAttributes;
    std::map<std::string, FloatAttributeHandler> percentageAttributes;
    std::map<std::string, FloatAttributeHandler> floatAttributes;
    std::map<std::string, StringAttributeHandler> stringAttributes;
    std::map<std::string, ColorAttributeHandler> colorAttributes;

    std::set<std::string> knownAttributes;

    void registerCommonAttributes();
    void printXMLAttributeErrorMessage(tinyxml2::XMLElement* element, std::string name, std::string value);

    bool applyXMLAttribute(std::string name, std::string value);

    NVGcolor borderColor = transparent;
    float borderTop      = 0;
    float borderRight    = 0;
    float borderBottom   = 0;
    float borderLeft     = 0;

#ifdef BRLS_WIREFRAME
    void drawWireframe(FrameContext* ctx, float x, float y, float width, float height);
#endif

    void drawBorder(FrameContext* ctx, float x, float y, float width, float height);

    Visibility visibility = Visibility::VISIBLE;

  protected:
    float collapseState = 1.0f;

    bool focused = false;

    Box* parent = nullptr;

    GenericEvent focusEvent;

    YGNode* ygNode; // TODO: free in dtor

    virtual void getHighlightInsets(float* top, float* right, float* bottom, float* left)
    {
        *top    = 0;
        *right  = 0;
        *bottom = 0;
        *left   = 0;
    }

    virtual void getHighlightMetrics(Style style, float* cornerRadius)
    {
        *cornerRadius = style["brls/highlight/corner_radius"];
    }

    virtual bool isHighlightBackgroundEnabled()
    {
        return true;
    }

    // Helper functions to apply this view's alpha to a color
    NVGcolor a(NVGcolor color);
    NVGpaint a(NVGpaint paint);

    NVGcolor RGB(unsigned r, unsigned g, unsigned b)
    {
        return this->a(nvgRGB(r, g, b));
    }

    NVGcolor RGBA(unsigned r, unsigned g, unsigned b, unsigned a)
    {
        return this->a(nvgRGBA(r, g, b, a));
    }

    NVGcolor RGBf(float r, float g, float b)
    {
        return this->a(nvgRGBf(r, g, b));
    }

    NVGcolor RGBAf(float r, float g, float b, float a)
    {
        return this->a(nvgRGBAf(r, g, b, a));
    }

    /**
     * Should the hint alpha be animated when
     * pushing the view?
     */
    virtual bool animateHint()
    {
        return false;
    }

    /**
    * Triggers a layout of the whole view tree. Must be called
    * after a yoga node property is changed.
    *
    * Only methods that change yoga nodes properties should
    * call this method.
    */
    void invalidate();

  public:
    static constexpr float AUTO = NAN;

    View();
    virtual ~View();

    void setBackground(ViewBackground background);

    void shakeHighlight(FocusDirection direction);

    float getX();
    float getY();
    float getWidth();
    float getHeight(bool includeCollapse = true);

    // -----------------------------------------------------------
    // Flex layout properties
    // -----------------------------------------------------------

    /**
     * Sets the preferred width of the view. Use brls::View::AUTO
     * to have the layout automatically resize the view.
     *
     * If set to anything else than AUTO, the view is guaranteed
     * to never shrink below the given width.
     */
    void setWidth(float width);

    /**
     * Sets the preferred height of the view. Use brls::View::AUTO
     * to have the layout automatically resize the view.
     *
     * If set to anything else than AUTO, the view is guaranteed
     * to never shrink below the given height.
     */
    void setHeight(float height);

    /**
     * Shortcut to setWidth + setHeight.
     *
     * Only does one layout pass instead of two when using the two methods separately.
     */
    void setDimensions(float width, float height);

    /**
     * Sets the preferred width of the view in percentage of
     * the parent view width. Between 0.0f and 100.0f.
     */
    void setWidthPercentage(float percentage);

    /**
     * Sets the preferred height of the view in percentage of
     * the parent view height. Between 0.0f and 100.0f.
     */
    void setHeightPercentage(float percentage);

    /**
     * Sets the grow factor of the view, aka the percentage
     * of remaining space to give this view, in the containing box axis.
     * Default is 0.0f;
     */
    void setGrow(float grow);

    /**
     * Sets the shrink factor of the view, aka how much the view is
     * allowed to shrink to give more space to others where there is
     * not enough room in the containing box, in the containing box axis.
     * 0.0f means no shrink is allowed.
     * Default is 1.0f.
     */
    void setShrink(float shrink);

    /**
     * Sets the margin of the view, aka the space that separates
     * this view and the surrounding ones in all 4 directions.
     *
     * Use brls::View::AUTO to have the layout automatically select the
     * margin.
     *
     * Only works with views that have parents - top level views that are pushed
     * on the stack don't have parents.
     *
     * Only does one layout pass instead of four when using the four methods separately.
     */
    void setMargins(float top, float right, float bottom, float left);

    /**
     * Sets the top margin of the view, aka the space that separates
     * this view and the surrounding ones.
     *
     * Only works with views that have parents - top level views that are pushed
     * on the stack don't have parents.
     *
     * Use brls::View::AUTO to have the layout automatically select the
     * margin.
     */
    void setMarginTop(float top);

    /**
     * Sets the right margin of the view, aka the space that separates
     * this view and the surrounding ones.
     *
     * Only works with views that have parents - top level views that are pushed
     * on the stack don't have parents.
     *
     * Use brls::View::AUTO to have the layout automatically select the
     * margin.
     */
    void setMarginRight(float right);

    /**
     * Sets the bottom margin of the view, aka the space that separates
     * this view and the surrounding ones.
     *
     * Only works with views that have parents - top level views that are pushed
     * on the stack don't have parents.
     *
     * Use brls::View::AUTO to have the layout automatically select the
     * margin.
     */
    void setMarginBottom(float right);

    /**
     * Sets the right margin of the view, aka the space that separates
     * this view and the surrounding ones.
     *
     * Only works with views that have parents - top level views that are pushed
     * on the stack don't have parents.
     *
     * Use brls::View::AUTO to have the layout automatically select the
     * margin.
     */
    void setMarginLeft(float left);

    /**
     * Sets the visibility of the view.
     */
    void setVisibility(Visibility visibility);

    // -----------------------------------------------------------
    // Styling properties
    // -----------------------------------------------------------

    /**
     * Sets the border color for the view. To be used with setBorderTop(),
     * setBorderRight()...
     */
    inline void setBorderColor(NVGcolor color)
    {
        this->borderColor = color;
    }

    /**
     * Sets the top border thickness. Use setBorderColor()
     * to change the border color.
     */
    inline void setBorderTop(float thickness)
    {
        this->borderTop = thickness;
    }

    /**
     * Sets the right border thickness. Use setBorderColor()
     * to change the border color.
     */
    inline void setBorderRight(float thickness)
    {
        this->borderRight = thickness;
    }

    /**
     * Sets the bottom border thickness. Use setBorderColor()
     * to change the border color.
     */
    inline void setBorderBottom(float thickness)
    {
        this->borderBottom = thickness;
    }

    /**
     * Sets the left border thickness. Use setBorderColor()
     * to change the border color.
     */
    inline void setBorderLeft(float thickness)
    {
        this->borderLeft = thickness;
    }

    // -----------------------------------------------------------

    /**
     * Creates a view from the given XML file content.
     *
     * The method handleXMLTag() is executed for each child node in the XML.
     *
     * Uses the internal lookup table to instantiate the views.
     * Use registerXMLView() to add your own views to the table so that
     * you can use them in your own XML files.
     */
    static View* createFromXMLString(std::string xml);

    /**
     * Creates a view from the given XML element (node and attributes).
     *
     * The method handleXMLTag() is executed for each child node in the XML.
     *
     * Uses the internal lookup table to instantiate the views.
     * Use registerXMLView() to add your own views to the table so that
     * you can use them in your own XML files.
     */
    static View* createFromXMLElement(tinyxml2::XMLElement* element);

    /**
     * Creates a view from the given XML file path.
     *
     * The method handleXMLTag() is executed for each child node in the XML.
     *
     * Uses the internal lookup table to instantiate the views.
     * Use registerXMLView() to add your own views to the table so that
     * you can use them in your own XML files.
     */
    static View* createFromXMLFile(std::string path);

    /**
     * Creates a view from the given XML resource file name.
     *
     * The method handleXMLTag() is executed for each child node in the XML.
     *
     * Uses the internal lookup table to instantiate the views.
     * Use registerXMLView() to add your own views to the table so that
     * you can use them in your own XML files.
     */
    static View* createFromXMLResource(std::string name);

    /**
     * Handles a child XML tag.
     *
     * You can redefine this method to handle child XML like
     * as you want in your own views.
     *
     * If left unimplemented, will throw an exception because raw
     * views cannot handle child XML tags (Boxes can).
     */
    virtual void handleXMLTag(tinyxml2::XMLElement* element);

    /**
     * Applies the attributes of the given XML element to the view.
     *
     * You can add your own attributes to by calling registerXMLAttribute()
     * in the view constructor.
     */
    void applyXMLAttributes(tinyxml2::XMLElement* element);

    /**
     * Register a new XML attribute with the given name and handler
     * method. You can have multiple attributes registered with the same
     * name but different types / handlers, except if the type is "string".
     *
     * The method will be called if the attribute has the value "auto".
     */
    void registerAutoXMLAttribute(std::string name, AutoAttributeHandler handler);

    /**
     * Register a new XML attribute with the given name and handler
     * method. You can have multiple attributes registered with the same
     * name but different types / handlers, except if the type is "string".
     *
     * The method will be called if the attribute has a percentage value (an integer with "%" suffix).
     * The given float value is guaranteed to be between 0.0f and 1.0f.
     */
    void registerPercentageXMLAttribute(std::string name, FloatAttributeHandler handler);

    /**
     * Register a new XML attribute with the given name and handler
     * method. You can have multiple attributes registered with the same
     * name but different types / handlers, except if the type is "string".
     *
     * The method will be called if the attribute has an integer, float, @style or "px" value.
     */
    void registerFloatXMLAttribute(std::string name, FloatAttributeHandler handler);

    /**
     * Register a new XML attribute with the given name and handler
     * method. You can have multiple attributes registered with the same
     * name but different types / handlers, except if the type is "string".
     *
     * The method will be called if the attribute has a string or @i18n value.
     *
     * If you use string as a type, you can only have one handler for the attribute.
     */
    void registerStringXMLAttribute(std::string name, StringAttributeHandler handler);

    /**
     * Register a new XML attribute with the given name and handler
     * method. You can have multiple attributes registered with the same
     * name but different types / handlers, except if the type is "string".
     *
     * The method will be called if the attribute has a color value ("#XXXXXX" or "#XXXXXXXX").
     */
    void registerColorXMLAttribute(std::string name, ColorAttributeHandler handler);

    /**
     * If set to true, will force the view to be translucent.
     */
    void setInFadeAnimation(bool translucent);

    void setParent(Box* parent, void* parentUserdata = nullptr);
    Box* getParent();
    bool hasParent();

    void* getParentUserData();

    void registerAction(std::string hintText, Key key, ActionListener actionListener, bool hidden = false);
    void updateActionHint(Key key, std::string hintText);
    void setActionAvailable(Key key, bool available);

    std::string describe() const { return typeid(*this).name(); }

    YGNode* getYGNode()
    {
        return this->ygNode;
    }

    const std::vector<Action>& getActions()
    {
        return this->actions;
    }

    /**
      * Called each frame
      * Do not override it to draw your view,
      * override draw() instead
      */
    virtual void frame(FrameContext* ctx);

    /**
      * Called by frame() to draw the view onscreen.
      * Views should not draw outside of their bounds (they
      * may be clipped if they do so).
      */
    virtual void draw(NVGcontext* vg, float x, float y, float width, float height, Style style, FrameContext* ctx) = 0;

    /**
      * Called when the view will appear
      * on screen, before or after layout().
      *
      * Can be called if the view has
      * already appeared, so be careful.
      */
    virtual void willAppear(bool resetState = false)
    {
        // Nothing to do
    }

    /**
      * Called when the view will disappear
      * from the screen.
      *
      * Can be called if the view has
      * already disappeared, so be careful.
      */
    virtual void willDisappear(bool resetState = false)
    {
        // Nothing to do
    }

    /**
      * Called when the show() animation (fade in)
      * ends
      */
    virtual void onShowAnimationEnd() {};

    /**
      * Shows the view with a fade in animation.
      */
    virtual void show(std::function<void(void)> cb);

    /**
      * Shows the view with a fade in animation, or no animation at all.
      */
    virtual void show(std::function<void(void)> cb, bool animate, float animationDuration);

    /**
     * Returns the duration of the view show / hide animation.
     */
    virtual float getShowAnimationDuration(TransitionAnimation animation);

    /**
      * Hides the view in a collapse animation
      */
    void collapse(bool animated = true);

    bool isCollapsed();

    void setAlpha(float alpha);

    /**
      * Shows the view in a expand animation (opposite
      * of collapse)
      */
    void expand(bool animated = true);

    /**
      * Hides the view with a fade out animation.
      */
    virtual void hide(std::function<void(void)> cb);

    /**
      * Hides the view with a fade out animation, or no animation at all.
      */
    virtual void hide(std::function<void(void)> cb, bool animate, float animationDuration);

    bool isHidden();

    /**
      * Is this view translucent?
      *
      * If you override it please return
      * <value> || View::isTranslucent()
      * to keep the fadeIn transition
      */
    virtual bool isTranslucent();

    bool isFocused();

    /**
     * Returns the default view to focus when focusing this view
     * Typically the view itself or one of its children
     *
     * Returning nullptr means that the view is not focusable
     * (and neither are its children)
     *
     * When pressing a key, the flow is :
     *    1. starting from the currently focused view's parent, traverse the tree upwards and
     *       repeatidly call getNextFocus() on every view until we find a next view to focus or meet the end of the tree
     *    2. if a view is found, getNextFocus() will internally call getDefaultFocus() for the selected child
     *    3. give focus to the result, if it exists
     */
    virtual View* getDefaultFocus()
    {
        return nullptr;
    }

    /**
     * Returns the next view to focus given the requested direction
     * and the currently focused view (as parent user data)
     *
     * Returning nullptr means that there is no next view to focus
     * in that direction - getNextFocus will then be called on our
     * parent if any
     */
    virtual View* getNextFocus(FocusDirection direction, View* currentView)
    {
        return nullptr;
    }

    /**
      * Fired when focus is gained
      */
    virtual void onFocusGained();

    /**
      * Fired when focus is lost
      */
    virtual void onFocusLost();

    /**
     * Fired when focus is gained on one of this view's children, or one of the children
     * of the children...
     *
     * directChild is guaranteed to be one of your children. It may not be the view that has been
     * focused.
     *
     * If focusedView == directChild, then the child of yours has been focused.
     * Otherwise, focusedView is a child of directChild.
     */
    virtual void onChildFocusGained(View* directChild, View* focusedView);

    /**
     * Fired when focus is lost on one of this view's children. Works similarly to
     * onChildFocusGained().
     */
    virtual void onChildFocusLost(View* directChild, View* focusedView);

    /**
     * Fired when the window size changes, after updating
     * layout.
     */
    virtual void onWindowSizeChanged()
    {
        // Nothing by default
    }

    GenericEvent* getFocusEvent();

    float alpha = 1.0f;

    virtual float getAlpha(bool child = false);

    /**
      * Forces this view and its children to use
      * the specified theme.
      */
    void overrideTheme(Theme* newTheme);

    /**
     * Enables / disable culling for that view.
     *
     * To disable culling for all child views
     * of a Box use setCullingEnabled on the box.
     */
    void setCulled(bool culled)
    {
        this->culled = culled;
    }

    bool isCulled()
    {
        return this->culled;
    }
};

} // namespace brls
