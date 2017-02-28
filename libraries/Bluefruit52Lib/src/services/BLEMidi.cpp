/**************************************************************************/
/*!
    @file     BLEMidi.cpp
    @author   hathach

    @section LICENSE

    Software License Agreement (BSD License)

    Copyright (c) 2017, Adafruit Industries (adafruit.com)
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
    1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
    3. Neither the name of the copyright holders nor the
    names of its contributors may be used to endorse or promote products
    derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
/**************************************************************************/

#include "bluefruit.h"

/* MIDI Service: 03B80E5A-EDE8-4B33-A751-6CE34EC4C700
 * MIDI I/O    : 7772E5DB-3868-4112-A1A9-F2669D106BF3
 */

const uint8_t BLEMIDI_UUID_SERVICE[] =
{
    0x00, 0xC7, 0xC4, 0x4E, 0xE3, 0x6C, 0x51, 0xA7,
    0x33, 0x4B, 0xE8, 0xED, 0x5A, 0x0E, 0xB8, 0x03
};

const uint8_t BLEMIDI_UUID_CHR_IO[] =
{
    0xF3, 0x6B, 0x10, 0x9D, 0x66, 0xF2, 0xA9, 0xA1,
    0x12, 0x41, 0x68, 0x38, 0xDB, 0xE5, 0x72, 0x77
};

/*------------------------------------------------------------------*/
/* MIDI Data Type
 *------------------------------------------------------------------*/
typedef union ATTR_PACKED
{
  struct {
    uint8_t timestamp_hi : 6;
    uint8_t              : 1;
    uint8_t start_bit    : 1;
  };

  uint8_t byte;
} midi_header_t;

VERIFY_STATIC ( sizeof(midi_header_t) == 1 );

typedef union ATTR_PACKED
{
  struct {
    uint8_t timestamp_low : 7;
    uint8_t start_bit : 1;
  };

  uint8_t byte;
} midi_timestamp_t;

VERIFY_STATIC ( sizeof(midi_timestamp_t) == 1 );

typedef union ATTR_PACKED
{
  struct {
    uint8_t channel : 4;
    uint8_t type    : 4;
  };

  uint8_t byte;
} midi_status_t;

VERIFY_STATIC ( sizeof(midi_status_t) == 1 );

typedef struct ATTR_PACKED
{
  midi_header_t header;
  midi_timestamp_t timestamp;
  uint8_t data[3];
} midi_event_packet_t;

VERIFY_STATIC ( sizeof(midi_event_packet_t) == 5 );

typedef struct ATTR_PACKED
{
  midi_header_t header;
  midi_timestamp_t timestamp;
  midi_status_t status;
  uint8_t data[16];
} midi_n_event_packet_t;

VERIFY_STATIC ( sizeof(midi_n_event_packet_t) == 19 );

/*------------------------------------------------------------------*/
/* IMPLEMENTATION
 *------------------------------------------------------------------*/
BLEMidi::BLEMidi(void)
  : BLEService(BLEMIDI_UUID_SERVICE), _io(BLEMIDI_UUID_CHR_IO)
{

}

bool BLEMidi::configured()
{
  return Bluefruit.connBonded() && _io.notifyEnabled();
}

void BLEMidi::setWriteCallback(BLECharacteristic::write_cb_t fp)
{
  _io.setWriteCallback(fp);
}

err_t BLEMidi::start(void)
{
  VERIFY_STATUS( this->addToGatt() );

  // IO characteristic
  _io.setProperties(CHR_PROPS_READ | CHR_PROPS_WRITE | CHR_PROPS_WRITE_WO_RESP | CHR_PROPS_NOTIFY);
  _io.setPermission(SECMODE_ENC_NO_MITM, SECMODE_ENC_NO_MITM);

  VERIFY_STATUS( _io.start() );

  // Attempt to change the connection interval to 11.25-15 ms when starting HID
  Bluefruit.setConnInterval(9, 12);

  return ERROR_NONE;
}

/*------------------------------------------------------------------*/
/* Send Event
 *------------------------------------------------------------------*/
 err_t BLEMidi::send(uint8_t data[])
{
  uint32_t tstamp = millis();

  midi_event_packet_t event =
  {
      .header = {{
          .timestamp_hi = (uint8_t) ((tstamp & 0x1F80UL) >> 7),
          .start_bit    = 1
      }},

      .timestamp = {{
          .timestamp_low = (uint8_t) (tstamp & 0x7FUL),
          .start_bit     = 1
      }}
  };

  memcpy(event.data, data, 3);

  VERIFY_STATUS( _io.notify(&event, sizeof(event)) );

  return ERROR_NONE;
}

err_t BLEMidi::send(uint8_t status, uint8_t byte1, uint8_t byte2)
{
  uint8_t data[] = { status, byte1, byte2 };
  return send(data);
}
