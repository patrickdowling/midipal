// Copyright 2011 Olivier Gillet.
//
// Author: Olivier Gillet (ol.gillet@gmail.com)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// -----------------------------------------------------------------------------
//
// MIDI clock monitor app.

#include "midipal/apps/bpm_meter.h"

#include "avrlib/string.h"

#include "midipal/clock.h"
#include "midipal/display.h"
#include "midipal/ui.h"

namespace midipal { namespace apps {

using namespace avrlib;

/* static */
uint32_t BpmMeter::num_ticks_;

/* static */
uint32_t BpmMeter::clock_;

/* static */
uint8_t BpmMeter::active_page_;

/* static */
uint8_t BpmMeter::refresh_bpm_;

/* static */
const prog_AppInfo BpmMeter::app_info_ PROGMEM = {
  &OnInit, // void (*OnInit)();
  NULL, // void (*OnNoteOn)(uint8_t, uint8_t, uint8_t);
  NULL, // void (*OnNoteOff)(uint8_t, uint8_t, uint8_t);
  NULL, // void (*OnNoteAftertouch)(uint8_t, uint8_t, uint8_t);
  NULL, // void (*OnAftertouch)(uint8_t, uint8_t);
  NULL, // void (*OnControlChange)(uint8_t, uint8_t, uint8_t);
  NULL, // void (*OnProgramChange)(uint8_t, uint8_t);
  NULL, // void (*OnPitchBend)(uint8_t, uint16_t);
  NULL, // void (*OnAllSoundOff)(uint8_t);
  NULL, // void (*OnResetAllControllers)(uint8_t);
  NULL, // void (*OnLocalControl)(uint8_t, uint8_t);
  NULL, // void (*OnAllNotesOff)(uint8_t);
  NULL, // void (*OnOmniModeOff)(uint8_t);
  NULL, // void (*OnOmniModeOn)(uint8_t);
  NULL, // void (*OnMonoModeOn)(uint8_t, uint8_t);
  NULL, // void (*OnPolyModeOn)(uint8_t);
  NULL, // void (*OnSysExStart)();
  NULL, // void (*OnSysExByte)(uint8_t);
  NULL, // void (*OnSysExEnd)();
  &OnClock, // void (*OnClock)();
  &OnStart, // void (*OnStart)();
  &OnContinue, // void (*OnContinue)();
  &OnStop, // void (*OnStop)();
  NULL, // void (*OnActiveSensing)();
  &OnReset, // void (*OnReset)();
  NULL, // uint8_t (*CheckChannel)(uint8_t);
  &OnRawByte, // void (*OnRawByte)(uint8_t);
  NULL, // void (*OnRawMidiData)(uint8_t, uint8_t*, uint8_t, uint8_t);
  NULL, // void (*OnInternalClockTick)();
  NULL, // void (*OnInternalClockStep)();
  &OnIncrement, // uint8_t (*OnIncrement)(int8_t);
  &OnClick, // uint8_t (*OnClick)();
  NULL, // uint8_t (*OnPot)(uint8_t, uint8_t);
  &OnRedraw, // uint8_t (*OnRedraw)();
  NULL, // void (*OnIdle)();
  NULL, // void (*SetParameter)(uint8_t, uint8_t);
  NULL, // uint8_t (*GetParameter)(uint8_t);
  NULL, // uint8_t (*CheckPageStatus)(uint8_t);
  0, // settings_size
  0, // settings_offset
  NULL, // settings_data
  0, // factory_data
  STR_RES_BPM_CNTR, // app_name
};

/* static */
void BpmMeter::OnInit() {
  active_page_ = 0;
  refresh_bpm_ = 1;
  ui.RefreshScreen();
}

/* static */
void BpmMeter::Reset() {
  num_ticks_ = 0;
  clock_ = 0;
  clock.Reset();
}

/* static */
void BpmMeter::OnClock() {
  if (num_ticks_ == 0) {
    clock.Reset();
  }
  clock_ = clock.value();
  ++num_ticks_;
}

/* static */
void BpmMeter::OnStart() {
  Reset();
}

/* static */
void BpmMeter::OnStop() {
  Reset();
}

/* static */
void BpmMeter::OnContinue() {
  Reset();
}

/* static */
void BpmMeter::OnReset() {
  Reset();
}

/* static */
void BpmMeter::OnRawByte(uint8_t byte) {
  app.SendNow(byte);
}

/* static */
void BpmMeter::PrintBpm() {
  line_buffer[0] = active_page_ == 1 ? 'B' : 'b';
  
  uint32_t num_ticks = num_ticks_;
  if (num_ticks) {
    --num_ticks;
  }
  uint32_t num = (78125 * 2 * 60 * 2 / 24 / 5) * num_ticks;
  uint32_t den = (2 * clock_ / 25);
  if (den == 0) {
    den = 1;
  }
  UnsafeItoa(num / den, 6, &line_buffer[2]);
  AlignRight(&line_buffer[2], 6);
  // A dirty hack to get the decimal display.
  if (line_buffer[6] == ' ') {
    line_buffer[6] = '0';
  }
  for (uint8_t i = 0; i < 5; ++i) {
    line_buffer[i + 1] = line_buffer[i + 2];
  }
  line_buffer[6] = '.';
}

/* static */
uint8_t BpmMeter::OnRedraw() {
  if (active_page_ < 2) {
    if (refresh_bpm_ || active_page_ == 1) {
      PrintBpm();
      ui.RefreshScreen();
      if (active_page_ == 0) {
        Reset();
      }
    }
    refresh_bpm_ = \
        (active_page_ == 0 && clock.value() > 30000 && num_ticks_ > 48);
  } else {
    line_buffer[0] = 't';
    UnsafeItoa(num_ticks_, 7, &line_buffer[1]);
    AlignRight(&line_buffer[1], 7);
    ui.RefreshScreen();
  }
  return 1;
}

/* static */
uint8_t BpmMeter::OnIncrement(int8_t increment) {
  if (increment) {
    active_page_ = active_page_ + increment;
    if (active_page_ > 128) {
      active_page_ = 0;
    } else if (active_page_ > 2) {
      active_page_ = 2;
    }
  }
  refresh_bpm_ = 1;
  ui.RefreshScreen();
  return 1;
}

/* static */
uint8_t BpmMeter::OnClick() {
  Reset();
  return 1;
}

} }  // namespace midipal::apps
