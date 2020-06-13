// Adafruit IO Temperature & Humidity Example
// Tutorial Link: https://learn.adafruit.com/adafruit-io-basics-temperature-and-humidity
//
// Adafruit invests time and resources providing this open source code.
// Please support Adafruit and open source hardware by purchasing
// products from Adafruit!
//
// Written by Todd Treece for Adafruit Industries
// Copyright (c) 2016-2017 Adafruit Industries
// Licensed under the MIT license.
//
// All text above must be included in any redistribution.

/************************** Configuration ***********************************/

// edit the config.h tab and enter your Adafruit IO credentials
// and any additional configuration needed for WiFi, cellular,
// or ethernet clients.
#include "config.h"

/************************ Example Starts Here *******************************/
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <array>
#include <cmath>

using namespace std;

// pin connected to DH22 data line
#define DATA_PIN 2

// create DHT22 instance
DHT_Unified dht(DATA_PIN, DHT22);

// set up the Adafruit IO feeds
AdafruitIO_Feed *temperature_feed = io.feed("temperature");
AdafruitIO_Feed *humidity_feed = io.feed("humidity");
AdafruitIO_Feed *heat_index_feed = io.feed("heat-index");

Adafruit_SSD1306 display = Adafruit_SSD1306(128, 32, &Wire);

/* Source: https://www.wpc.ncep.noaa.gov/html/heatindex_equation.shtml */
float get_heat_index(float t, float rh)
{
  const array<float, 9> K = {
      -42.379,
      2.04901523,
      10.14333127,
      -0.22475541,
      -0.00683783,
      -0.05481717,
      0.00122874,
      0.00085282,
      -0.00000199};

  const array<float, 9> x = {
      1,
      t,
      rh,
      t * rh,
      t * t,
      rh * rh,
      t * t * rh,
      t * rh * rh,
      t * t * rh * rh};

  // Use simple formula to decide how to proceed
  const float hi_simple = 0.5 * (t + 61.0 + ((t - 68.0) * 1.2) + (rh * 9.4e-2));
  // Average with the temperature
  const float hi_simple_avg = (hi_simple + t) / 2;
  // For temperatures below 80 F, the simple formula is more appropriate, so
  // return here
  if (hi_simple_avg < 80.0)
  {
    return hi_simple;
  }
  // Otherwise, compute the full heat index regression
  float hi_full_regression = 0;
  for (int i = 0; i < x.size(); i++)
  {
    hi_full_regression += K[i] * x[i];
  }
  // There are two cases where adjustments need to be added to the heat index
  // for an accurate result:
  float adjustment = 0;
  if ((rh < 13.0) && (t < 112.0))
  {
    adjustment = -(((13 - rh) / 4) * sqrtf((17 - abs(t - 95)) / 17));
  }
  else if ((rh > 85.0) && (t < 87.0))
  {
    adjustment = ((rh - 85) / 10) * ((87 - t) / 5);
  }
  hi_full_regression += adjustment;

  return hi_full_regression;
}

void setup()
{

  // start the serial connection
  Serial.begin(115200);

  // wait for serial monitor to open
  while (!Serial)
    ;

  // inititalize the display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // Address 0x3C for 128x32
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // initialize dht22
  dht.begin();

  // connect to io.adafruit.com
  Serial.print("Connecting to Adafruit IO");
  io.connect();

  // wait for a connection
  while (io.status() < AIO_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }

  // we are connected
  Serial.println();
  Serial.println(io.statusText());
}

void loop()
{

  // io.run(); is required for all sketches.
  // it should always be present at the top of your loop
  // function. it keeps the client connected to
  // io.adafruit.com, and processes any incoming data.
  io.run();

  sensors_event_t event;
  /* ---- Temperature ---- */
  dht.temperature().getEvent(&event);

  float temperature_f = (event.temperature * 1.8) + 32;

  Serial.print("Temperature: ");
  Serial.print(temperature_f);
  Serial.println(" F");

  // save fahrenheit (or celsius) to Adafruit IO
  temperature_feed->save(temperature_f);

  /* ---- Humidity ---- */
  dht.humidity().getEvent(&event);

  float humidity = event.relative_humidity;

  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println("%");

  // save humidity to Adafruit IO
  humidity_feed->save(humidity);

  /* ---- Heat Index ---- */
  float heat_index = get_heat_index(temperature_f, humidity);

  Serial.print("Heat Index: ");
  Serial.print(heat_index);
  Serial.println(" F");

  heat_index_feed->save(heat_index);

  /* ---- Display ----- */
  // put all on display
  display.clearDisplay();
  display.setCursor(0, 0);

  display.print("Heat Index: ");
  display.print(heat_index);
  display.println(" F");

  display.print("Temperature: ");
  display.print(temperature_f);
  display.println(" F");

  display.print("Humidity: ");
  display.print(humidity);
  display.println("%");

  display.display();

  // wait 10s to prevent data throttle
  delay(10000);
}
