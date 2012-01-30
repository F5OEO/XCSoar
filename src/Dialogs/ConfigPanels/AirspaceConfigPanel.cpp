/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "AirspaceConfigPanel.hpp"
#include "ConfigPanel.hpp"
#include "DataField/Enum.hpp"
#include "DataField/Boolean.hpp"
#include "Form/Button.hpp"
#include "Form/RowFormWidget.hpp"
#include "Dialogs/Airspace.hpp"
#include "Profile/ProfileKeys.hpp"
#include "Language/Language.hpp"
#include "Airspace/AirspaceComputerSettings.hpp"
#include "Renderer/AirspaceRendererSettings.hpp"
#include "Interface.hpp"
#include "UIGlobals.hpp"

enum ControlIndex {
  AirspaceDisplay,
  ClipAltitude,
  AltWarningMargin,
  AirspaceWarnings,
  WarningTime,
  AcknowledgeTime,
  UseBlackOutline,
  AirspaceFillMode,
  AirspaceTransparency
};

static const StaticEnumChoice  as_display_list[] = {
  { ALLON, N_("All on"),
    N_("All airspaces are displayed.") },
  { CLIP, N_("Clip"),
    N_("Display airspaces below the clip altitude.") },
  { AUTO, N_("Auto"),
    N_("Display airspaces within a margin of the glider.") },
  { ALLBELOW, N_("All below"),
    N_("Display airspaces below the glider or within a margin.") },
  { 0 }
};

static const StaticEnumChoice  as_fill_mode_list[] = {
  { AirspaceRendererSettings::AS_FILL_DEFAULT, N_("Default"),
    _T("") },
  { AirspaceRendererSettings::AS_FILL_ALL, N_("Fill all"),
    _T("") },
  { AirspaceRendererSettings::AS_FILL_PADDING, N_("Fill padding"),
    _T("") },
  { 0 }
};

class AirspaceConfigPanel : public RowFormWidget {
private:
  WndButton *buttonColors, *buttonMode;

public:
  AirspaceConfigPanel()
    :RowFormWidget(UIGlobals::GetDialogLook()) {}

  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc);
  virtual bool Save(bool &changed, bool &require_restart);
  virtual void Show(const PixelRect &rc);
  virtual void Hide();
  void ShowDisplayControls(AirspaceDisplayMode_t mode);
  void ShowWarningControls(bool visible);
  void SetButtonsVisible(bool active);
};


/** XXX this hack is needed because the form callbacks don't get a
    context pointer - please refactor! */
static AirspaceConfigPanel *instance;

static void
OnAirspaceColoursClicked(gcc_unused WndButton &button)
{
  dlgAirspaceShowModal(true);
}

static void
OnAirspaceModeClicked(gcc_unused WndButton &button)
{
  dlgAirspaceShowModal(false);
}

void
AirspaceConfigPanel::ShowDisplayControls(AirspaceDisplayMode_t mode)
{
  SetRowVisible(ClipAltitude, mode == CLIP);
  SetRowVisible(AltWarningMargin, mode == AUTO || mode == ALLBELOW);
}

void
AirspaceConfigPanel::ShowWarningControls(bool visible)
{
  SetRowVisible(WarningTime, visible);
  SetRowVisible(AcknowledgeTime, visible);
}

void
AirspaceConfigPanel::SetButtonsVisible(bool active)
{
  if (buttonColors != NULL)
    buttonColors->set_visible(active);

  if (buttonMode != NULL)
    buttonMode->set_visible(active);
}

void
AirspaceConfigPanel::Show(const PixelRect &rc)
{
  if (buttonColors != NULL) {
    buttonColors->set_text(_("Colours"));
    buttonColors->SetOnClickNotify(OnAirspaceColoursClicked);
  }

  if (buttonMode != NULL) {
    buttonMode->set_text(_("Filter"));
    buttonMode->SetOnClickNotify(OnAirspaceModeClicked);
  }

  SetButtonsVisible(true);
  RowFormWidget::Show(rc);
}

void
AirspaceConfigPanel::Hide()
{
  RowFormWidget::Hide();
  SetButtonsVisible(false);
}

static void
OnAirspaceDisplay(DataField *Sender,
                  DataField::DataAccessKind_t Mode)
{
  const DataFieldEnum &df = *(const DataFieldEnum *)Sender;
  AirspaceDisplayMode_t mode = (AirspaceDisplayMode_t)df.GetAsInteger();
  instance->ShowDisplayControls(mode);
}

static void
OnAirspaceWarning(DataField *Sender,
                  DataField::DataAccessKind_t Mode)
{
  const DataFieldBoolean &df = *(const DataFieldBoolean *)Sender;
  instance->ShowWarningControls(df.GetAsBoolean());
}

void
AirspaceConfigPanel::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  const AirspaceComputerSettings &computer =
    CommonInterface::GetComputerSettings().airspace;
  const AirspaceRendererSettings &renderer =
    CommonInterface::GetMapSettings().airspace;

  instance = this;

  RowFormWidget::Prepare(parent, rc);

  AddEnum(_("Airspace display"),
          _("Controls filtering of airspace for display and warnings.  The airspace filter button also allows filtering of display and warnings independently for each airspace class."),
          as_display_list ,renderer.altitude_mode, OnAirspaceDisplay);

  AddFloat(_("Clip altitude"),
           _("For clip airspace mode, this is the altitude below which airspace is displayed."),
           _T("%.0f %s"), _T("%.0f"), fixed(0), fixed(20000), fixed(100), false, UnitGroup::ALTITUDE, fixed(renderer.clip_altitude));

  AddFloat(_("Margin"),
           _("For auto and all below airspace mode, this is the altitude above/below which airspace is included."),
           _T("%.0f %s"), _T("%.0f"), fixed(0), fixed(10000), fixed(100), false, UnitGroup::ALTITUDE, fixed(computer.warnings.AltWarningMargin));

  AddBoolean(_("Warnings"), _("Enable/disable all airspace warnings."), computer.enable_warnings, OnAirspaceWarning);

  AddTime(_("Warning time"),
          _("This is the time before an airspace incursion is estimated at which the system will warn the pilot."),
          10, 1000, 5, computer.warnings.WarningTime);
  SetExpertRow(WarningTime);

  AddTime(_("Acknowledge time"),
          _("This is the time period in which an acknowledged airspace warning will not be repeated."),
          10, 1000, 5, computer.warnings.AcknowledgementTime);
  SetExpertRow(AcknowledgeTime);

  AddBoolean(_("Use black outline"),
             _("Draw a black outline around each airspace rather than the airspace color."),
             renderer.black_outline);
  SetExpertRow(UseBlackOutline);

  AddEnum(_("Airspace fill mode"),
          _("Specifies the mode for filling the airspace area."),
          as_fill_mode_list, renderer.fill_mode);
  SetExpertRow(AirspaceFillMode);

#if !defined(ENABLE_OPENGL) && defined(HAVE_ALPHA_BLEND)
  if (AlphaBlendAvailable()) {
    AddBoolean(_("Airspace transparency"), _("If enabled, then airspaces are filled transparently."),
               renderer.transparency);
    SetExpertRow(AirspaceTransparency);
  }
#endif

  buttonColors = ConfigPanel::GetExtraButton(1);
  assert(buttonColors != NULL);

  buttonMode = ConfigPanel::GetExtraButton(2);
  assert(buttonMode != NULL);

  ShowDisplayControls(renderer.altitude_mode); // TODO make this work the first time
  ShowWarningControls(computer.enable_warnings);
}


bool
AirspaceConfigPanel::Save(bool &_changed, bool &require_restart)
{
  bool changed = false;

  AirspaceComputerSettings &computer =
    CommonInterface::SetComputerSettings().airspace;
  AirspaceRendererSettings &renderer =
    CommonInterface::SetMapSettings().airspace;

  changed |= SaveValueEnum(AirspaceDisplay, szProfileAltMode, renderer.altitude_mode);

  changed |= SaveValue(ClipAltitude, UnitGroup::ALTITUDE, szProfileClipAlt, renderer.clip_altitude);

  changed |= SaveValue(AltWarningMargin, UnitGroup::ALTITUDE, szProfileAltMargin, computer.warnings.AltWarningMargin);

  changed |= SaveValue(AirspaceWarnings, szProfileAirspaceWarning, computer.enable_warnings);

  if (SaveValue(WarningTime, szProfileWarningTime, computer.warnings.WarningTime)) {
    changed = true;
    require_restart = true;
  }

  if (SaveValue(AcknowledgeTime, szProfileAcknowledgementTime,
                computer.warnings.AcknowledgementTime)) {
    changed = true;
    require_restart = true;
  }

  changed |= SaveValue(UseBlackOutline, szProfileAirspaceBlackOutline, renderer.black_outline);

  changed |= SaveValueEnum(AirspaceFillMode, szProfileAirspaceFillMode, renderer.fill_mode);

#ifndef ENABLE_OPENGL
#ifdef HAVE_ALPHA_BLEND
  if (AlphaBlendAvailable())
    changed |= SaveValue(AirspaceTransparency, szProfileAirspaceTransparency,
                         renderer.transparency);
#endif
#endif /* !OpenGL */

  _changed |= changed;

  return true;
}


Widget *
CreateAirspaceConfigPanel()
{
  return new AirspaceConfigPanel();
}

