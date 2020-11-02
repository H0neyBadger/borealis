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

#include <borealis/applet_frame.hpp>

namespace brls
{

const std::string appletFrameXML = R"xml(
    <brls::Box
        width="auto"
        height="auto"
        axis="column"
        justifyContent="spaceBetween"
        paddingLeft="@style/brls/applet_frame/padding_sides"
        paddingRight="@style/brls/applet_frame/padding_sides">

        <!-- Header -->
        <brls::Box
            width="auto"
            height="@style/brls/applet_frame/header_height"
            axis="row"
            paddingTop="@style/brls/applet_frame/header_padding_top_bottom"
            paddingBottom="@style/brls/applet_frame/header_padding_top_bottom"
            paddingLeft="@style/brls/applet_frame/header_padding_sides"
            paddingRight="@style/brls/applet_frame/header_padding_sides"
            borderColor="@theme/brls/applet_frame/separator"
            borderBottom="1px">

            <brls::Rectangle
                width="50px"
                height="auto"
                marginRight="@style/brls/applet_frame/header_image_title_spacing"
                color="#0000FF" />

            <brls::Rectangle
                width="225px"
                height="auto"
                color="#FF00FF" />

        </brls::Box>

        <!-- Content will be injected here with grow="1.0" -->

        <!--
            Footer
            Direction inverted so that the bottom left text can be
            set to visibility="gone" without affecting the hint
        -->
        <brls::Box
            width="auto"
            height="@style/brls/applet_frame/footer_height"
            axis="row"
            direction="rightToLeft"
            paddingLeft="@style/brls/applet_frame/footer_padding_sides"
            paddingRight="@style/brls/applet_frame/footer_padding_sides"
            paddingTop="@style/brls/applet_frame/footer_padding_top_bottom"
            paddingBottom="@style/brls/applet_frame/footer_padding_top_bottom"
            borderColor="@theme/brls/applet_frame/separator"
            justifyContent="spaceBetween"
            borderTop="1px" >

            <brls::Rectangle
                width="272px"
                height="auto"
                color="#FF0000" />

            <brls::Rectangle
                width="75px"
                height="auto"
                color="#FF00FF" />

        </brls::Box>

    </brls::Box>
)xml";

AppletFrame::AppletFrame()
{
    this->inflateFromXML(appletFrameXML);
}

void AppletFrame::addView(View* view)
{
    // TODO: inject the view in second, between header and footer - replace if a view already exists
}

View* AppletFrame::createFromXMLElement(tinyxml2::XMLElement* element)
{
    return new AppletFrame();
}

} // namespace brls
