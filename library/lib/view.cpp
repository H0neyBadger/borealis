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

#include <math.h>

#include <algorithm>
#include <borealis/animations.hpp>
#include <borealis/application.hpp>
#include <borealis/box.hpp>
#include <borealis/view.hpp>

namespace brls
{

static bool endsWith(const std::string& data, const std::string& suffix)
{
    return data.find(suffix, data.size() - suffix.size()) != std::string::npos;
}

static bool startsWith(const std::string& data, const std::string& prefix)
{
    return data.rfind(prefix, 0) == 0;
}

View::View()
{
    // Instantiate and prepare YGNode
    this->ygNode = YGNodeNew();

    YGNodeStyleSetWidthAuto(this->ygNode);
    YGNodeStyleSetHeightAuto(this->ygNode);

    // Register common XML attributes
    this->registerCommonAttributes();
}

const NVGcolor transparent = nvgRGBA(0, 0, 0, 0);

static int shakeAnimation(float t, float a) // a = amplitude
{
    // Damped sine wave
    float w = 0.8f; // period
    float c = 0.35f; // damp factor
    return roundf(a * exp(-(c * t)) * sin(w * t));
}

void View::shakeHighlight(FocusDirection direction)
{
    this->highlightShaking        = true;
    this->highlightShakeStart     = cpu_features_get_time_usec() / 1000;
    this->highlightShakeDirection = direction;
    this->highlightShakeAmplitude = std::rand() % 15 + 10;
}

float View::getAlpha(bool child)
{
    return this->alpha * (this->parent ? this->parent->getAlpha(true) : 1.0f);
}

NVGcolor View::a(NVGcolor color)
{
    NVGcolor newColor = color; // copy
    newColor.a *= this->getAlpha();
    return newColor;
}

NVGpaint View::a(NVGpaint paint)
{
    NVGpaint newPaint = paint; // copy
    newPaint.innerColor.a *= this->getAlpha();
    newPaint.outerColor.a *= this->getAlpha();
    return newPaint;
}

// TODO: Only draw views that are onscreen (w/ some margins)
void View::frame(FrameContext* ctx)
{
    if (this->visibility != Visibility::VISIBLE)
        return;

    Style style    = Application::getStyle();
    Theme oldTheme = ctx->theme;

    nvgSave(ctx->vg);

    // Theme override
    if (this->themeOverride)
        ctx->theme = *themeOverride;

    float x      = this->getX();
    float y      = this->getY();
    float width  = this->getWidth();
    float height = this->getHeight();

    if (this->alpha > 0.0f && this->collapseState != 0.0f)
    {
        // Draw background
        this->drawBackground(ctx->vg, ctx, style);

        // Draw highlight background
        if (this->highlightAlpha > 0.0f && this->isHighlightBackgroundEnabled())
            this->drawHighlight(ctx->vg, ctx->theme, this->highlightAlpha, style, true);

        // Collapse clipping
        if (this->collapseState < 1.0f)
        {
            nvgSave(ctx->vg);
            nvgIntersectScissor(ctx->vg, x, y, width, height * this->collapseState);
        }

        // Draw the view
        this->draw(ctx->vg, x, y, width, height, style, ctx);

        // Draw highlight
        if (this->highlightAlpha > 0.0f)
            this->drawHighlight(ctx->vg, ctx->theme, this->highlightAlpha, style, false);

#ifdef BRLS_WIREFRAME
        this->drawWireframe(ctx, x, y, width, height);
#endif

        this->drawBorder(ctx, x, y, width, height);

        //Reset clipping
        if (this->collapseState < 1.0f)
            nvgRestore(ctx->vg);
    }

    // Cleanup
    if (this->themeOverride)
        ctx->theme = oldTheme;

    nvgRestore(ctx->vg);
}

void View::drawBorder(FrameContext* ctx, float x, float y, float width, float height)
{
    // Don't setup and draw empty nvg path if there is no border to draw
    if (this->borderTop <= 0 && this->borderRight <= 0 && this->borderBottom <= 0 && this->borderLeft <= 0)
        return;

    nvgBeginPath(ctx->vg);
    nvgFillColor(ctx->vg, a(this->borderColor));

    if (this->borderTop > 0)
        nvgRect(ctx->vg, x, y, width, this->borderTop);

    if (this->borderRight > 0)
        nvgRect(ctx->vg, x + width, y, this->borderRight, height);

    if (this->borderBottom > 0)
        nvgRect(ctx->vg, x, y + height - this->borderBottom, width, this->borderBottom);

    if (this->borderLeft > 0)
        nvgRect(ctx->vg, x - this->borderLeft, y, this->borderLeft, height);

    nvgFill(ctx->vg);
}

#ifdef BRLS_WIREFRAME
void View::drawWireframe(FrameContext* ctx, float x, float y, float width, float height)
{
    nvgStrokeWidth(ctx->vg, 1);

    // Outline
    nvgBeginPath(ctx->vg);
    nvgStrokeColor(ctx->vg, nvgRGB(0, 0, 255));
    nvgRect(ctx->vg, x, y, width, height);
    nvgStroke(ctx->vg);

    if (this->hasParent())
    {
        // Diagonals
        nvgFillColor(ctx->vg, nvgRGB(0, 0, 255));

        nvgBeginPath(ctx->vg);
        nvgMoveTo(ctx->vg, x, y);
        nvgLineTo(ctx->vg, x + width, y + height);
        nvgFill(ctx->vg);

        nvgBeginPath(ctx->vg);
        nvgMoveTo(ctx->vg, x + width, y);
        nvgLineTo(ctx->vg, x, y + height);
        nvgFill(ctx->vg);
    }

    // Padding
    nvgBeginPath(ctx->vg);
    nvgStrokeColor(ctx->vg, nvgRGB(0, 255, 0));

    float paddingTop    = ntz(YGNodeLayoutGetPadding(this->ygNode, YGEdgeTop));
    float paddingLeft   = ntz(YGNodeLayoutGetPadding(this->ygNode, YGEdgeLeft));
    float paddingBottom = ntz(YGNodeLayoutGetPadding(this->ygNode, YGEdgeBottom));
    float paddingRight  = ntz(YGNodeLayoutGetPadding(this->ygNode, YGEdgeRight));

    // Top
    if (paddingTop > 0)
        nvgRect(ctx->vg, x, y, width, paddingTop);

    // Right
    if (paddingRight > 0)
        nvgRect(ctx->vg, x + width - paddingRight, y, paddingRight, height);

    // Bottom
    if (paddingBottom > 0)
        nvgRect(ctx->vg, x, y + height - paddingBottom, width, paddingBottom);

    // Left
    if (paddingLeft > 0)
        nvgRect(ctx->vg, x, y, paddingLeft, height);

    nvgStroke(ctx->vg);

    // Margins
    nvgBeginPath(ctx->vg);
    nvgStrokeColor(ctx->vg, nvgRGB(255, 0, 0));

    float marginTop    = ntz(YGNodeLayoutGetMargin(this->ygNode, YGEdgeTop));
    float marginLeft   = ntz(YGNodeLayoutGetMargin(this->ygNode, YGEdgeLeft));
    float marginBottom = ntz(YGNodeLayoutGetMargin(this->ygNode, YGEdgeBottom));
    float marginRight  = ntz(YGNodeLayoutGetMargin(this->ygNode, YGEdgeRight));

    // Top
    if (marginTop > 0)
        nvgRect(ctx->vg, x - marginLeft, y - marginTop, width + marginLeft + marginRight, marginTop);

    // Right
    if (marginRight > 0)
        nvgRect(ctx->vg, x + width, y - marginTop, marginRight, height + marginTop + marginBottom);

    // Bottom
    if (marginBottom > 0)
        nvgRect(ctx->vg, x - marginLeft, y + height, width + marginLeft + marginRight, marginBottom);

    // Left
    if (marginLeft > 0)
        nvgRect(ctx->vg, x - marginLeft, y - marginTop, marginLeft, height + marginTop + marginBottom);

    nvgStroke(ctx->vg);
}
#endif

void View::collapse(bool animated)
{
    menu_animation_ctx_tag tag = (uintptr_t) & this->collapseState;
    menu_animation_kill_by_tag(&tag);

    if (animated)
    {
        Style style = Application::getStyle();

        menu_animation_ctx_entry_t entry;

        entry.cb           = [](void* userdata) {};
        entry.duration     = style["brls/animations_durations/collapse"];
        entry.easing_enum  = EASING_OUT_QUAD;
        entry.subject      = &this->collapseState;
        entry.tag          = tag;
        entry.target_value = 0.0f;
        entry.tick         = [this](void* userdata) { if (this->hasParent()) this->getParent()->invalidate(); };
        entry.userdata     = nullptr;

        menu_animation_push(&entry);
    }
    else
    {
        this->collapseState = 0.0f;
    }
}

bool View::isCollapsed()
{
    return this->collapseState < 1.0f;
}

void View::expand(bool animated)
{
    menu_animation_ctx_tag tag = (uintptr_t) & this->collapseState;
    menu_animation_kill_by_tag(&tag);

    if (animated)
    {
        Style style = Application::getStyle();

        menu_animation_ctx_entry_t entry;

        entry.cb           = [](void* userdata) {};
        entry.duration     = style["brls/animations_durations/collapse"];
        entry.easing_enum  = EASING_OUT_QUAD;
        entry.subject      = &this->collapseState;
        entry.tag          = tag;
        entry.target_value = 1.0f;
        entry.tick         = [this](void* userdata) { if (this->hasParent()) this->getParent()->invalidate(); };
        entry.userdata     = nullptr;

        menu_animation_push(&entry);
    }
    else
    {
        this->collapseState = 1.0f;
    }
}

void View::setAlpha(float alpha)
{
    this->alpha = alpha;
}

// TODO: Slight glow all around
void View::drawHighlight(NVGcontext* vg, Theme theme, float alpha, Style style, bool background)
{
    nvgSave(vg);
    nvgResetScissor(vg);

    float insetTop, insetRight, insetBottom, insetLeft;
    this->getHighlightInsets(&insetTop, &insetRight, &insetBottom, &insetLeft);

    float cornerRadius;
    this->getHighlightMetrics(style, &cornerRadius);

    float strokeWidth = style["brls/highlight/stroke_width"];

    float x      = this->getX() - insetLeft - strokeWidth / 2;
    float y      = this->getY() - insetTop - strokeWidth / 2;
    float width  = this->getWidth() + insetLeft + insetRight + strokeWidth - 1;
    float height = this->getHeight() + insetTop + insetBottom + strokeWidth - 1;

    // Shake animation
    if (this->highlightShaking)
    {
        retro_time_t curTime = cpu_features_get_time_usec() / 1000;
        retro_time_t t       = (curTime - highlightShakeStart) / 10;

        if (t >= style["brls/animations_durations/shake"])
        {
            this->highlightShaking = false;
        }
        else
        {
            switch (this->highlightShakeDirection)
            {
                case FocusDirection::RIGHT:
                    x += shakeAnimation(t, this->highlightShakeAmplitude);
                    break;
                case FocusDirection::LEFT:
                    x -= shakeAnimation(t, this->highlightShakeAmplitude);
                    break;
                case FocusDirection::DOWN:
                    y += shakeAnimation(t, this->highlightShakeAmplitude);
                    break;
                case FocusDirection::UP:
                    y -= shakeAnimation(t, this->highlightShakeAmplitude);
                    break;
            }
        }
    }

    // Draw
    if (background)
    {
        // Background
        NVGcolor highlightBackgroundColor = theme["brls/highlight/background_color"];
        nvgFillColor(vg, RGBAf(highlightBackgroundColor.r, highlightBackgroundColor.g, highlightBackgroundColor.b, this->highlightAlpha));
        nvgBeginPath(vg);
        nvgRoundedRect(vg, x, y, width, height, cornerRadius);
        nvgFill(vg);
    }
    else
    {
        float shadowOffset = style["brls/highlight/shadow_offset"];

        // Shadow
        NVGpaint shadowPaint = nvgBoxGradient(vg,
            x, y + style["brls/highlight/shadow_width"],
            width, height,
            cornerRadius * 2, style["brls/highlight/shadow_feather"],
            RGBA(0, 0, 0, style["brls/highlight/shadow_opacity"] * alpha), transparent);

        nvgBeginPath(vg);
        nvgRect(vg, x - shadowOffset, y - shadowOffset,
            width + shadowOffset * 2, height + shadowOffset * 3);
        nvgRoundedRect(vg, x, y, width, height, cornerRadius);
        nvgPathWinding(vg, NVG_HOLE);
        nvgFillPaint(vg, shadowPaint);
        nvgFill(vg);

        // Border
        float gradientX, gradientY, color;
        menu_animation_get_highlight(&gradientX, &gradientY, &color);

        NVGcolor highlightColor1 = theme["brls/highlight/color1"];

        NVGcolor pulsationColor = RGBAf((color * highlightColor1.r) + (1 - color) * highlightColor1.r,
            (color * highlightColor1.g) + (1 - color) * highlightColor1.g,
            (color * highlightColor1.b) + (1 - color) * highlightColor1.b,
            alpha);

        NVGcolor borderColor = theme["brls/highlight/color2"];
        borderColor.a        = 0.5f * alpha * this->getAlpha();

        float strokeWidth = style["brls/highlight/stroke_width"];

        NVGpaint border1Paint = nvgRadialGradient(vg,
            x + gradientX * width, y + gradientY * height,
            strokeWidth * 10, strokeWidth * 40,
            borderColor, transparent);

        NVGpaint border2Paint = nvgRadialGradient(vg,
            x + (1 - gradientX) * width, y + (1 - gradientY) * height,
            strokeWidth * 10, strokeWidth * 40,
            borderColor, transparent);

        nvgBeginPath(vg);
        nvgStrokeColor(vg, pulsationColor);
        nvgStrokeWidth(vg, strokeWidth);
        nvgRoundedRect(vg, x, y, width, height, cornerRadius);
        nvgStroke(vg);

        nvgBeginPath(vg);
        nvgStrokePaint(vg, border1Paint);
        nvgStrokeWidth(vg, strokeWidth);
        nvgRoundedRect(vg, x, y, width, height, cornerRadius);
        nvgStroke(vg);

        nvgBeginPath(vg);
        nvgStrokePaint(vg, border2Paint);
        nvgStrokeWidth(vg, strokeWidth);
        nvgRoundedRect(vg, x, y, width, height, cornerRadius);
        nvgStroke(vg);
    }

    nvgRestore(vg);
}

void View::setBackground(ViewBackground background)
{
    this->background = background;
}

void View::drawBackground(NVGcontext* vg, FrameContext* ctx, Style style)
{
    float x      = this->getX();
    float y      = this->getY();
    float width  = this->getWidth();
    float height = this->getHeight();

    Theme theme = ctx->theme;

    switch (this->background)
    {
        case ViewBackground::SIDEBAR:
        {
            float backdropHeight  = style["brls/view/sidebar_border_height"];
            NVGcolor sidebarColor = theme["brls/view/sidebar_color"];

            // Solid color
            nvgBeginPath(vg);
            nvgFillColor(vg, a(sidebarColor));
            nvgRect(vg, x, y + backdropHeight, width, height - backdropHeight * 2);
            nvgFill(vg);

            //Borders gradient
            // Top
            NVGpaint topGradient = nvgLinearGradient(vg, x, y + backdropHeight, x, y, a(sidebarColor), transparent);
            nvgBeginPath(vg);
            nvgFillPaint(vg, topGradient);
            nvgRect(vg, x, y, width, backdropHeight);
            nvgFill(vg);

            // Bottom
            NVGpaint bottomGradient = nvgLinearGradient(vg, x, y + height - backdropHeight, x, y + height, a(sidebarColor), transparent);
            nvgBeginPath(vg);
            nvgFillPaint(vg, bottomGradient);
            nvgRect(vg, x, y + height - backdropHeight, width, backdropHeight);
            nvgFill(vg);
            break;
        }
        case ViewBackground::BACKDROP:
        {
            nvgFillColor(vg, a(theme["brls/view/backdrop_color"]));
            nvgBeginPath(vg);
            nvgRect(vg, x, y, width, height);
            nvgFill(vg);
        }
        case ViewBackground::NONE:
            break;
    }
}

void View::registerAction(std::string hintText, Key key, ActionListener actionListener, bool hidden)
{
    if (auto it = std::find(this->actions.begin(), this->actions.end(), key); it != this->actions.end())
        *it = { key, hintText, true, hidden, actionListener };
    else
        this->actions.push_back({ key, hintText, true, hidden, actionListener });
}

void View::updateActionHint(Key key, std::string hintText)
{
    if (auto it = std::find(this->actions.begin(), this->actions.end(), key); it != this->actions.end())
        it->hintText = hintText;

    Application::getGlobalHintsUpdateEvent()->fire();
}

void View::setActionAvailable(Key key, bool available)
{
    if (auto it = std::find(this->actions.begin(), this->actions.end(), key); it != this->actions.end())
        it->available = available;
}

void View::setParent(Box* parent, void* parentUserdata)
{
    this->parent         = parent;
    this->parentUserdata = parentUserdata;
}

void* View::getParentUserData()
{
    return this->parentUserdata;
}

bool View::isFocused()
{
    return this->focused;
}

Box* View::getParent()
{
    return this->parent;
}

bool View::hasParent()
{
    return this->parent;
}

void View::setDimensions(float width, float height)
{
    if (width == View::AUTO)
    {
        YGNodeStyleSetWidthAuto(this->ygNode);
        YGNodeStyleSetMinWidth(this->ygNode, YGUndefined);
    }
    else
    {
        YGNodeStyleSetWidth(this->ygNode, width);
        YGNodeStyleSetMinWidth(this->ygNode, width);
    }

    if (height == View::AUTO)
    {
        YGNodeStyleSetHeightAuto(this->ygNode);
        YGNodeStyleSetMinHeight(this->ygNode, YGUndefined);
    }
    else
    {
        YGNodeStyleSetHeight(this->ygNode, height);
        YGNodeStyleSetMinHeight(this->ygNode, height);
    }

    this->invalidate();
}

void View::setWidth(float width)
{
    if (width == View::AUTO)
    {
        YGNodeStyleSetWidthAuto(this->ygNode);
        YGNodeStyleSetMinWidth(this->ygNode, YGUndefined);
    }
    else
    {
        YGNodeStyleSetWidth(this->ygNode, width);
        YGNodeStyleSetMinWidth(this->ygNode, width);
    }

    this->invalidate();
}

void View::setHeight(float height)
{
    if (height == View::AUTO)
    {
        YGNodeStyleSetHeightAuto(this->ygNode);
        YGNodeStyleSetMinHeight(this->ygNode, YGUndefined);
    }
    else
    {
        YGNodeStyleSetHeight(this->ygNode, height);
        YGNodeStyleSetMinHeight(this->ygNode, height);
    }

    this->invalidate();
}

void View::setWidthPercentage(float percentage)
{
    YGNodeStyleSetWidthPercent(this->ygNode, percentage);
    this->invalidate();
}

void View::setHeightPercentage(float percentage)
{
    YGNodeStyleSetHeightPercent(this->ygNode, percentage);
    this->invalidate();
}

void View::setMarginTop(float top)
{
    if (top == brls::View::AUTO)
        YGNodeStyleSetMarginAuto(this->ygNode, YGEdgeTop);
    else
        YGNodeStyleSetMargin(this->ygNode, YGEdgeTop, top);

    this->invalidate();
}

void View::setMarginRight(float right)
{
    if (right == brls::View::AUTO)
        YGNodeStyleSetMarginAuto(this->ygNode, YGEdgeRight);
    else
        YGNodeStyleSetMargin(this->ygNode, YGEdgeRight, right);

    this->invalidate();
}

void View::setMarginBottom(float bottom)
{
    if (bottom == brls::View::AUTO)
        YGNodeStyleSetMarginAuto(this->ygNode, YGEdgeBottom);
    else
        YGNodeStyleSetMargin(this->ygNode, YGEdgeBottom, bottom);

    this->invalidate();
}

void View::setMarginLeft(float left)
{
    if (left == brls::View::AUTO)
        YGNodeStyleSetMarginAuto(this->ygNode, YGEdgeLeft);
    else
        YGNodeStyleSetMargin(this->ygNode, YGEdgeLeft, left);

    this->invalidate();
}

void View::setMargins(float top, float right, float bottom, float left)
{
    if (top == brls::View::AUTO)
        YGNodeStyleSetMarginAuto(this->ygNode, YGEdgeTop);
    else
        YGNodeStyleSetMargin(this->ygNode, YGEdgeTop, top);

    if (right == brls::View::AUTO)
        YGNodeStyleSetMarginAuto(this->ygNode, YGEdgeRight);
    else
        YGNodeStyleSetMargin(this->ygNode, YGEdgeRight, right);

    if (bottom == brls::View::AUTO)
        YGNodeStyleSetMarginAuto(this->ygNode, YGEdgeBottom);
    else
        YGNodeStyleSetMargin(this->ygNode, YGEdgeBottom, bottom);

    if (left == brls::View::AUTO)
        YGNodeStyleSetMarginAuto(this->ygNode, YGEdgeLeft);
    else
        YGNodeStyleSetMargin(this->ygNode, YGEdgeLeft, left);

    this->invalidate();
}

void View::setGrow(float grow)
{
    YGNodeStyleSetFlexGrow(this->ygNode, grow);
    this->invalidate();
}

void View::setShrink(float shrink)
{
    YGNodeStyleSetFlexShrink(this->ygNode, shrink);
    this->invalidate();
}

void View::invalidate()
{
    if (this->hasParent())
        this->getParent()->invalidate();
    else
        YGNodeCalculateLayout(this->ygNode, YGUndefined, YGUndefined, YGDirectionLTR);
}

float View::getX()
{
    if (this->hasParent())
        return this->getParent()->getX() + YGNodeLayoutGetLeft(this->ygNode);

    return YGNodeLayoutGetLeft(this->ygNode);
}

float View::getY()
{
    if (this->hasParent())
        return this->getParent()->getY() + YGNodeLayoutGetTop(this->ygNode);

    return YGNodeLayoutGetTop(this->ygNode);
}

float View::getHeight(bool includeCollapse)
{
    return YGNodeLayoutGetHeight(this->ygNode) * (includeCollapse ? this->collapseState : 1.0f);
}

float View::getWidth()
{
    return YGNodeLayoutGetWidth(this->ygNode);
}

void View::onFocusGained()
{
    this->focused = true;

    Style style = Application::getStyle();

    menu_animation_ctx_tag tag = (uintptr_t)this->highlightAlpha;

    menu_animation_ctx_entry_t entry;
    entry.cb           = [](void* userdata) {};
    entry.duration     = style["brls/animations_durations/highlight"];
    entry.easing_enum  = EASING_OUT_QUAD;
    entry.subject      = &this->highlightAlpha;
    entry.tag          = tag;
    entry.target_value = 1.0f;
    entry.tick         = [](void* userdata) {};
    entry.userdata     = nullptr;

    menu_animation_push(&entry);

    this->focusEvent.fire(this);

    if (this->hasParent())
        this->getParent()->onChildFocusGained(this, this);
}

GenericEvent* View::getFocusEvent()
{
    return &this->focusEvent;
}

/**
 * Fired when focus is lost
 */
void View::onFocusLost()
{
    this->focused = false;

    Style style = Application::getStyle();

    menu_animation_ctx_tag tag = (uintptr_t)this->highlightAlpha;

    menu_animation_ctx_entry_t entry;
    entry.cb           = [](void* userdata) {};
    entry.duration     = style["brls/animations_durations/highlight"];
    entry.easing_enum  = EASING_OUT_QUAD;
    entry.subject      = &this->highlightAlpha;
    entry.tag          = tag;
    entry.target_value = 0.0f;
    entry.tick         = [](void* userdata) {};
    entry.userdata     = nullptr;

    menu_animation_push(&entry);

    if (this->hasParent())
        this->getParent()->onChildFocusLost(this, this);
}

void View::setInFadeAnimation(bool inFadeAnimation)
{
    this->inFadeAnimation = inFadeAnimation;
}

bool View::isTranslucent()
{
    return this->fadeIn || this->inFadeAnimation;
}

void View::show(std::function<void(void)> cb)
{
    this->show(cb, true, this->getShowAnimationDuration(TransitionAnimation::FADE));
}

void View::show(std::function<void(void)> cb, bool animate, float animationDuration)
{
    brls::Logger::debug("Showing {}", this->describe());

    this->hidden = false;

    menu_animation_ctx_tag tag = (uintptr_t) & this->alpha;
    menu_animation_kill_by_tag(&tag);

    this->fadeIn = true;

    if (animate)
    {
        this->alpha = 0.0f;

        menu_animation_ctx_entry_t entry;
        entry.cb = [this, cb](void* userdata) {
            this->fadeIn = false;
            this->onShowAnimationEnd();
            cb();
        };
        entry.duration     = animationDuration;
        entry.easing_enum  = EASING_OUT_QUAD;
        entry.subject      = &this->alpha;
        entry.tag          = tag;
        entry.target_value = 1.0f;
        entry.tick         = [](void* userdata) {};
        entry.userdata     = nullptr;

        menu_animation_push(&entry);
    }
    else
    {
        this->alpha  = 1.0f;
        this->fadeIn = false;
        this->onShowAnimationEnd();
        cb();
    }
}

void View::hide(std::function<void(void)> cb)
{
    this->hide(cb, true, this->getShowAnimationDuration(TransitionAnimation::FADE));
}

void View::hide(std::function<void(void)> cb, bool animated, float animationDuration)
{
    brls::Logger::debug("Hiding {}", this->describe());

    this->hidden = true;
    this->fadeIn = false;

    menu_animation_ctx_tag tag = (uintptr_t) & this->alpha;
    menu_animation_kill_by_tag(&tag);

    if (animated)
    {
        this->alpha = 1.0f;

        menu_animation_ctx_entry_t entry;
        entry.cb           = [cb](void* userdata) { cb(); };
        entry.duration     = animationDuration;
        entry.easing_enum  = EASING_OUT_QUAD;
        entry.subject      = &this->alpha;
        entry.tag          = tag;
        entry.target_value = 0.0f;
        entry.tick         = [](void* userdata) {};
        entry.userdata     = nullptr;

        menu_animation_push(&entry);
    }
    else
    {
        this->alpha = 0.0f;
        cb();
    }
}

float View::getShowAnimationDuration(TransitionAnimation animation)
{
    Style style = Application::getStyle();

    if (animation == TransitionAnimation::SLIDE_LEFT || animation == TransitionAnimation::SLIDE_RIGHT)
        throw std::logic_error("Slide animation is not supported on views");

    return style["brls/animations_durations/show"];
}

bool View::isHidden()
{
    return this->hidden;
}

void View::overrideTheme(Theme* newTheme)
{
    this->themeOverride = newTheme;
}

void View::onChildFocusGained(View* directChild, View* focusedView)
{
    if (this->hasParent())
        this->getParent()->onChildFocusGained(this, focusedView);
}

void View::onChildFocusLost(View* directChild, View* focusedView)
{
    if (this->hasParent())
        this->getParent()->onChildFocusLost(this, focusedView);
}

View::~View()
{
    menu_animation_ctx_tag alphaTag = (uintptr_t) & this->alpha;
    menu_animation_kill_by_tag(&alphaTag);

    menu_animation_ctx_tag highlightTag = (uintptr_t) & this->highlightAlpha;
    menu_animation_kill_by_tag(&highlightTag);

    menu_animation_ctx_tag collapseTag = (uintptr_t) & this->collapseState;
    menu_animation_kill_by_tag(&collapseTag);

    // Parent userdata
    if (this->parentUserdata)
    {
        free(this->parentUserdata);
        this->parentUserdata = nullptr;
    }

    // Focus sanity check
    if (Application::getCurrentFocus() == this)
        Application::giveFocus(nullptr);
}

bool View::applyXMLAttribute(std::string name, std::string value)
{
    // String -> string
    if (this->stringAttributes.count(name) > 0)
    {
        // TODO: starts with @i18n -> string

        this->stringAttributes[name](value);
        return true;
    }

    // Auto -> auto
    if (value == "auto")
    {
        if (this->autoAttributes.count(name) > 0)
        {
            this->autoAttributes[name]();
            return true;
        }
        else
            return false;
    }

    // Ends with px -> float
    if (endsWith(value, "px"))
    {
        // Strip the px and parse the float value
        std::string newFloat = value.substr(0, value.length() - 2);
        try
        {
            float floatValue = std::stof(newFloat);
            if (this->floatAttributes.count(name) > 0)
            {
                this->floatAttributes[name](floatValue);
                return true;
            }
            else
                return false;
        }
        catch (const std::invalid_argument& exception)
        {
            return false;
        }
    }

    // Ends with % -> percentage
    if (endsWith(value, "%"))
    {
        // Strip the % and parse the float value
        std::string newFloat = value.substr(0, value.length() - 1);
        try
        {
            float floatValue = std::stof(newFloat);

            if (floatValue < 0 || floatValue > 100)
                return false;

            if (this->percentageAttributes.count(name) > 0)
            {
                this->percentageAttributes[name](floatValue);
                return true;
            }
            else
                return false;
        }
        catch (const std::invalid_argument& exception)
        {
            return false;
        }
    }
    // Starts with @style -> float
    else if (startsWith(value, "@style/"))
    {
        // Parse the style name
        std::string styleName = value.substr(7); // length of "@style/"
        float value           = Application::getStyle()[styleName]; // will throw logic_error if the metric doesn't exist

        if (this->floatAttributes.count(name) > 0)
        {
            this->floatAttributes[name](value);
            return true;
        }
        else
            return false;
    }
    // Starts with with # -> color
    else if (startsWith(value, "#"))
    {
        // Parse the color
        // #RRGGBB format
        if (value.size() == 7)
        {
            unsigned char r, g, b;
            int result = sscanf(value.c_str(), "#%02hhx%02hhx%02hhx", &r, &g, &b);

            if (result != 3)
                return false;
            else if (this->colorAttributes.count(name) > 0)
            {
                this->colorAttributes[name](nvgRGB(r, g, b));
                return true;
            }
            else
                return false;
        }
        // #RRGGBBAA format
        else if (value.size() == 9)
        {
            unsigned char r, g, b, a;
            int result = sscanf(value.c_str(), "#%02hhx%02hhx%02hhx%02hhx", &r, &g, &b, &a);

            if (result != 4)
                return false;
            else if (this->colorAttributes.count(name) > 0)
            {
                this->colorAttributes[name](nvgRGBA(r, g, b, a));
                return true;
            }
            else
                return false;
        }
        else
            return false;
    }
    // Starts with @theme -> color
    else if (startsWith(value, "@theme/"))
    {
        // Parse the color name
        std::string colorName = value.substr(7); // length of "@theme/"
        NVGcolor value        = Application::getTheme()[colorName]; // will throw logic_error if the color doesn't exist

        if (this->colorAttributes.count(name) > 0)
        {
            this->colorAttributes[name](value);
            return true;
        }
        else
            return false;
    }

    // Valid float -> float, otherwise unknown attribute
    try
    {
        float newValue = std::stof(value);
        if (this->floatAttributes.count(name) > 0)
        {
            this->floatAttributes[name](newValue);
            return true;
        }
        else
            return false;
    }
    catch (const std::invalid_argument& exception)
    {
        return false;
    }
}

void View::applyXMLAttributes(tinyxml2::XMLElement* element)
{
    if (!element)
        return;

    for (const tinyxml2::XMLAttribute* attribute = element->FirstAttribute(); attribute != nullptr; attribute = attribute->Next())
    {
        std::string name  = attribute->Name();
        std::string value = std::string(attribute->Value());

        if (!this->applyXMLAttribute(name, value))
            this->printXMLAttributeErrorMessage(element, name, value);
    }
}

View* View::createFromXMLResource(std::string name)
{
    return View::createFromXMLFile(std::string(BOREALIS_RESOURCES) + "xml/" + name); // TODO: platform agnostic separator here if anyone cares about Windows
}

View* View::createFromXMLFile(std::string path)
{
    tinyxml2::XMLDocument document;
    tinyxml2::XMLError error = document.LoadFile(path.c_str());

    if (error != tinyxml2::XMLError::XML_SUCCESS)
        throw std::logic_error("Unable to load XML file \"" + path + "\": error " + std::to_string(error));

    tinyxml2::XMLElement* element = document.RootElement();

    if (!element)
        throw std::logic_error("Unable to load XML file \"" + path + "\": no root element found, is the file empty?");

    return View::createFromXMLElement(element);
}

View* View::createFromXMLElement(tinyxml2::XMLElement* element)
{
    if (!element)
        return nullptr;

    std::string viewName = element->Name();

    if (!Application::XMLViewsRegisterContains(viewName))
        throw std::logic_error("Unknown XML tag \"" + viewName + "\"");

    View* view = Application::getXMLViewCreator(viewName)(element);
    view->applyXMLAttributes(element);

    for (tinyxml2::XMLElement* child = element->FirstChildElement(); child != nullptr; child = child->NextSiblingElement())
        view->handleXMLTag(child);

    return view;
}

void View::handleXMLTag(tinyxml2::XMLElement* element)
{
    throw std::logic_error("Raw views cannot have child XML tags");
}

void View::registerCommonAttributes()
{
    // Width
    this->registerAutoXMLAttribute("width", [this]() {
        this->setWidth(View::AUTO);
    });

    this->registerFloatXMLAttribute("width", [this](float value) {
        this->setWidth(value);
    });

    this->registerPercentageXMLAttribute("width", [this](float value) {
        this->setWidthPercentage(value);
    });

    // Height
    this->registerAutoXMLAttribute("height", [this]() {
        this->setHeight(View::AUTO);
    });

    this->registerFloatXMLAttribute("height", [this](float value) {
        this->setHeight(value);
    });

    this->registerPercentageXMLAttribute("height", [this](float value) {
        this->setHeightPercentage(value);
    });

    // Grow
    this->registerFloatXMLAttribute("grow", [this](float value) {
        this->setGrow(value);
    });

    // Shrink
    this->registerFloatXMLAttribute("shrink", [this](float value) {
        this->setShrink(value);
    });

    // Margin top
    this->registerFloatXMLAttribute("marginTop", [this](float value) {
        this->setMarginTop(value);
    });

    this->registerAutoXMLAttribute("marginTop", [this]() {
        this->setMarginTop(View::AUTO);
    });

    // Margin right
    this->registerFloatXMLAttribute("marginRight", [this](float value) {
        this->setMarginRight(value);
    });

    this->registerAutoXMLAttribute("marginRight", [this]() {
        this->setMarginRight(View::AUTO);
    });

    // Margin bottom
    this->registerFloatXMLAttribute("marginBottom", [this](float value) {
        this->setMarginBottom(value);
    });

    this->registerAutoXMLAttribute("marginBottom", [this]() {
        this->setMarginBottom(View::AUTO);
    });

    // Margin left
    this->registerFloatXMLAttribute("marginLeft", [this](float value) {
        this->setMarginLeft(value);
    });

    this->registerAutoXMLAttribute("marginLeft", [this]() {
        this->setMarginLeft(View::AUTO);
    });

    // Border
    this->registerColorXMLAttribute("borderColor", [this](NVGcolor color) {
        this->setBorderColor(color);
    });

    this->registerFloatXMLAttribute("borderTop", [this](float value) {
        this->setBorderTop(value);
    });

    this->registerFloatXMLAttribute("borderRight", [this](float value) {
        this->setBorderRight(value);
    });

    this->registerFloatXMLAttribute("borderBottom", [this](float value) {
        this->setBorderBottom(value);
    });

    this->registerFloatXMLAttribute("borderLeft", [this](float value) {
        this->setBorderLeft(value);
    });

    // Visiblity
    this->registerStringXMLAttribute("visibility", [this](std::string value) {
        if (value == "visible")
            this->setVisibility(Visibility::VISIBLE);
        else if (value == "invisible")
            this->setVisibility(Visibility::INVISIBLE);
        else if (value == "gone")
            this->setVisibility(Visibility::GONE);
        else
            throw std::logic_error("Illegal value \"" + value + "\" for XML attribute \"visibility\"");
    });
}

void View::setVisibility(Visibility visibility)
{
    this->visibility = visibility;

    if (visibility == Visibility::GONE)
        YGNodeStyleSetDisplay(this->ygNode, YGDisplayNone);
    else
        YGNodeStyleSetDisplay(this->ygNode, YGDisplayFlex);

    if (visibility == Visibility::VISIBLE)
        this->willAppear();
    else
        this->willDisappear();

    this->invalidate();
}

void View::printXMLAttributeErrorMessage(tinyxml2::XMLElement* element, std::string name, std::string value)
{
    if (this->knownAttributes.find(name) != this->knownAttributes.end())
        throw std::logic_error("Illegal value \"" + value + "\" for \"" + std::string(element->Name()) + "\" XML attribute \"" + name + "\"");
    else
        throw std::logic_error("Unknown XML attribute \"" + name + "\" for tag \"" + std::string(element->Name()) + "\" (with value \"" + value + "\")");
}

void View::registerFloatXMLAttribute(std::string name, FloatAttributeHandler handler)
{
    this->floatAttributes[name] = handler;
    this->knownAttributes.insert(name);
}

void View::registerPercentageXMLAttribute(std::string name, FloatAttributeHandler handler)
{
    this->percentageAttributes[name] = handler;
    this->knownAttributes.insert(name);
}

void View::registerAutoXMLAttribute(std::string name, AutoAttributeHandler handler)
{
    this->autoAttributes[name] = handler;
    this->knownAttributes.insert(name);
}

void View::registerStringXMLAttribute(std::string name, StringAttributeHandler handler)
{
    this->stringAttributes[name] = handler;
    this->knownAttributes.insert(name);
}

void View::registerColorXMLAttribute(std::string name, ColorAttributeHandler handler)
{
    this->colorAttributes[name] = handler;
    this->knownAttributes.insert(name);
}

float ntz(float value)
{
    return std::isnan(value) ? 0.0f : value;
}

} // namespace brls
