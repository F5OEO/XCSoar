// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device/Driver/Softrf.hpp"
#include "Device/Driver.hpp"
#include "Device/Util/NMEAWriter.hpp"
#include "NMEA/Checksum.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/InputLine.hpp"

using std::string_view_literals::operator""sv;

class SoftrfDevice : public AbstractDevice {
  Port &port;

public:
  SoftrfDevice(Port &_port):port(_port) {}

  /* virtual methods from class Device */
  bool EnableNMEA(OperationEnvironment &env) override;
  bool ParseNMEA(const char *line, struct NMEAInfo &info) override;
};

bool
SoftrfDevice::EnableNMEA(OperationEnvironment &env)
{
  /* this command initiates NMEA mode according to the "Flymaster F1
     Commands" document */
  PortWriteNMEA(port, "$PFMNAV,", env);
  return true;
}

static bool
VARIOLK(NMEAInputLine &line, NMEAInfo &info)
{
  /*
  LK8000 EXTERNAL INSTRUMENT SERIES 1 - NMEA SENTENCE: LK8EX1
VERSION A, 110217

LK8EX1,pressure,altitude,vario,temperature,battery,*checksum

Field 0, raw pressure in hPascal:
	hPA*100 (example for 1013.25 becomes  101325) 
	no padding (987.25 becomes 98725, NOT 098725)
	If no pressure available, send 999999 (6 times 9)
	If pressure is available, field 1 altitude will be ignored

Field 1, altitude in meters, relative to QNH 1013.25
	If raw pressure is available, this value will be IGNORED (you can set it to 99999
	but not really needed)!
	(if you want to use this value, set raw pressure to 999999)
	This value is relative to sea level (QNE). We are assuming that
	currently at 0m altitude pressure is standard 1013.25.
	If you cannot send raw altitude, then send what you have but then
	you must NOT adjust it from Basic Setting in LK.
	Altitude can be negative
	If altitude not available, and Pressure not available, set Altitude
	to 99999  (5 times 9)
	LK will say "Baro altitude available" if one of fields 0 and 1 is available.

Field 2, vario in cm/s
	If vario not available, send 9999  (4 times 9)
	Value can also be negative

Field 3, temperature in C , can be also negative
	If not available, send 99

Field 4, battery voltage or charge percentage
	Cannot be negative
	If not available, send 999 (3 times 9)
	Voltage is sent as float value like: 0.1 1.4 2.3  11.2 
	To send percentage, add 1000. Example 0% = 1000
	14% = 1014 .  Do not send float values for percentages.
	Percentage should be 0 to 100, with no decimals, added by 1000!
*/
  double value;
  if (line.ReadChecked(value)&&(value!=999999)) // Raw Pressure
    info.ProvideStaticPressure(AtmosphericPressure::HectoPascal(value));

  if (line.ReadChecked(value)) // Altitude in meters
  {}

  if (line.ReadChecked(value)&&(value!=9999)) //Vario
    info.ProvideTotalEnergyVario(value / 100);

  if (line.ReadChecked(value)&&(value!=99)) //Temperature
  {
    info.temperature = Temperature::FromCelsius(value);
    info.temperature_available = true;
  }

  if (line.ReadChecked(value)&&(value!=999))
  {
    info.voltage = value;
    info.voltage_available.Update(info.clock);
  }

  

  return true;
}

bool
SoftrfDevice::ParseNMEA(const char *String, NMEAInfo &info)
{
  if (!VerifyNMEAChecksum(String))
    return false;

  NMEAInputLine line(String);

  const auto type = line.ReadView();
  if (type == "$LK8EX1"sv)
    return VARIOLK(line, info);
  else
    return false;
}

static Device *
SoftrfDeviceCreateOnPort([[maybe_unused]] const DeviceConfig &config, Port &port)
{
  return new SoftrfDevice(port);
}

const struct DeviceRegister softrf_driver = {
  _T("SoftRF"),
  _T("SoftRF "),
  0,
  SoftrfDeviceCreateOnPort,
};
