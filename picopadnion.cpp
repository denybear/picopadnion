/**
 * @file picovation.c
 * @brief A pico board acting as USB Host and sending session signals to external groovebox (Novation Circuit) at press of a button
 * 
 * MIT License

 * Copyright (c) 2022 denybear, rppicomidi

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 */


#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "bsp/board.h"
#include "tusb.h"

#include "synth.hpp"
#include "audio.hpp"
#include "song.h"

// constants
#define PICO_AUDIO_PACK_I2S_DATA 9
#define PICO_AUDIO_PACK_I2S_BCLK 10
#define NB_INSTRUMENTS 5

#define MIDI_CLOCK		0xF8
#define MIDI_PLAY		0xFA
#define MIDI_STOP		0xFC
#define MIDI_CONTINUE	0xFB
#define MIDI_PRG_CHANGE	0xCF	// 0xC0 is program change, 0x0F is midi channel

#define LED_GPIO	25	// onboard led
#define LED2_GPIO	255	// 2nd led
const uint NO_LED_GPIO = 255;
const uint NO_LED2_GPIO = 255;

// globals
static uint8_t midi_dev_addr = 0;
static bool connected = false;

// midi buffers
#define MIDI_BUF_SIZE	500
static uint8_t midi_rx [MIDI_BUF_SIZE];		// large midi buffer to avoid override when receiving midi
static uint8_t midi_tx [MIDI_BUF_SIZE];		// large midi buffer to avoid override when receiving midi
static int index_tx = 0;					// number of bytes to be sent


using namespace synth;

synth::AudioChannel synth::channels[CHANNEL_COUNT];


// 0: melody
// 1: rhythm
// 2: drum
// 3: hi-hat
// 4: bass

const uint16_t instruments[NB_INSTRUMENTS][6] = {
	Waveform::TRIANGLE | Waveform::SQUARE, 16, 168, 0xafff, 168, 10000,
	Waveform::SINE | Waveform::SQUARE, 38, 300, 0, 0, 12000,
	Waveform::NOISE, 5, 10, 16000, 100, 18000,
	Waveform::NOISE, 5, 5, 8000, 40, 8000,
	Waveform::SQUARE, 10, 100, 0, 500, 12000
};


// poll USB receive and process received bytes accordingly
void poll_usb_rx ()
{
	// device must be attached and have at least one endpoint ready to receive a message
	if (!connected || tuh_midih_get_num_rx_cables(midi_dev_addr) < 1)
	{
		return;
	}
	tuh_midi_read_poll(midi_dev_addr);
}


// write lg bytes stored in buffer to midi out
void send_midi (uint8_t * buffer, uint32_t lg)
{
	uint32_t nwritten;

	if (connected && tuh_midih_get_num_tx_cables(midi_dev_addr) >= 1)
	{
		nwritten = tuh_midi_stream_write(midi_dev_addr, 0, buffer, lg);
		if (nwritten != lg) {
			TU_LOG1("Warning: Dropped %ld byte\r\n", (lg-nwritten));
		}
	}
}


// load an instrument of a song into a channel
bool load_instrument(int instr, int chan) {

	// check boundaries
	if ((instr <0) || (instr >= NB_INSTRUMENTS)) return false;
	if ((chan <0) || (chan >= CHANNEL_COUNT)) return false;

	// assign instrument parameters to the channel
	channels[chan].waveforms   = instruments [instr][0];
	channels[chan].attack_ms   = instruments [instr][1];
	channels[chan].decay_ms    = instruments [instr][2];
	channels[chan].sustain     = instruments [instr][3];
	channels[chan].release_ms  = instruments [instr][4];
	channels[chan].volume      = instruments [instr][5];
  
	return true;
}


// get new data from the song, and plays the notes
// num is the song number, position is the step number
bool update_playback(int num, int position) {

int pointer;
int nb_chan;
int nb_step;
int i;
uint16_t freq;

	// check if song exists by checking that song number is lower than number of songs
	if (num >= song [0]) return false;

	// get pointer to song data
	pointer = song [1+num];

	// get number of channels and steps
	nb_chan = song [pointer++];
	nb_step = song [pointer++];

	// no need to load instruments of the song to the channels, this is done when loading a song
	// this could be done only once at song loading
	pointer += nb_chan;

	// determine if we are still within the song; if not, return false
	if (position >= nb_step) return false;
	// get song data (at position) and load corresponding channels with it
	// make sure we are not at the end of the song
	for (i=0; i<nb_chan; i++) {
		freq = song [pointer + (position * (nb_chan + 2)) + i];		// nb_chan + 2 as we need to skip pad number and color
		if (freq == 0xFFFF) return false;
		channels[i].frequency = freq;
		channels[i].trigger_attack();
	}
	
	return true;
}


// release all active channels for a song
void stop_playback(void) {

  // we must update the playback with release on all channels

	for(uint8_t i = 0; i < CHANNEL_COUNT; i++) {
    	// if channel is in OFF state, then do nothing
    	// if channel is already in release state, then do nothing
    	// if channel is in another state, then go to release state
    	if ((channels[i].adsr_phase != ADSRPhase::OFF) && (channels[i].adsr_phase != ADSRPhase::RELEASE)) {
			channels[i].trigger_release();
		}
	}
}


// shut down all the channels
void reset_playback(void) {

	// we must stop all channels
	for(uint8_t i = 0; i < CHANNEL_COUNT; i++) {
		channels[i].off();
	}
}


// load song with number NUM
bool load_song (int num) {

int pointer;
int nb_chan;
int i;

	// check if song exists by checking that song number is lower than number of songs
	if (num >= song [0]) return false;

	// make sure all channels are off
	reset_playback ();

	// get pointer to song data
	pointer = song [1+num];

	// get number of channels and steps
	nb_chan = song [pointer++];
	// skip number of steps
	pointer++;

	// load instruments of the song to the corresponding channel
	for (i=0; i<nb_chan; i++) {
		if (load_instrument (song [pointer++], i) == false) return false;
	}

	return true;
}



int main() {

	int position = 0;
	int song_num = 0;	// by default, song number is 000
	int number_of_songs;


	stdio_init_all();
	board_init();
	printf("Picopadnion\r\n");

	// configure USB host
	tusb_init();

	// configure audio
	struct audio_buffer_pool *ap = init_audio(synth::sample_rate, PICO_AUDIO_PACK_I2S_DATA, PICO_AUDIO_PACK_I2S_BCLK);


	// main loop
	while (1) {

		tuh_task();
		// check connection to USB slave
		connected = midi_dev_addr != 0 && tuh_midi_configured(midi_dev_addr);

		if (index_tx) {
			send_midi (midi_tx, index_tx);
			index_tx = 0;
		}

		// read MIDI events coming from groovebox and manage accordingly
		if (connected) tuh_midi_stream_flush(midi_dev_addr);
		poll_usb_rx ();

    	// update audio buffer : make sure we do this regularly (in while loop)
	   	update_buffer(ap, get_audio_frame);
	}
}


//--------------------------------------------------------------------+
// TinyUSB Callbacks
//--------------------------------------------------------------------+

// Invoked when device with hid interface is mounted
// Report descriptor is also available for use. tuh_hid_parse_report_descriptor()
// can be used to parse common/simple enough descriptor.
// Note: if report descriptor length > CFG_TUH_ENUMERATION_BUFSIZE, it will be skipped
// therefore report_desc = NULL, desc_len = 0
void tuh_midi_mount_cb(uint8_t dev_addr, uint8_t in_ep, uint8_t out_ep, uint8_t num_cables_rx, uint16_t num_cables_tx)
{
	printf("MIDI device address = %u, IN endpoint %u has %u cables, OUT endpoint %u has %u cables\r\n",
		dev_addr, in_ep & 0xf, num_cables_rx, out_ep & 0xf, num_cables_tx);

	if (midi_dev_addr == 0) {
		// then no MIDI device is currently connected
		midi_dev_addr = dev_addr;
	}

	else {
		printf("A different USB MIDI Device is already connected.\r\nOnly one device at a time is supported in this program\r\nDevice is disabled\r\n");
	}
}

// Invoked when device with hid interface is un-mounted
void tuh_midi_umount_cb(uint8_t dev_addr, uint8_t instance)
{
	if (dev_addr == midi_dev_addr) {
		midi_dev_addr = 0;
		printf("MIDI device address = %d, instance = %d is unmounted\r\n", dev_addr, instance);
	}
	else {
		printf("Unused MIDI device address = %d, instance = %d is unmounted\r\n", dev_addr, instance);
	}
}

// invoked when receiving some MIDI data
void tuh_midi_rx_cb(uint8_t dev_addr, uint32_t num_packets)
{
	uint8_t cable_num;
	uint8_t *buffer;
	uint32_t i;
	uint32_t bytes_read;

	// set midi_rx as buffer
	buffer = midi_rx;
	
	if (midi_dev_addr == dev_addr)
	{
		if (num_packets != 0)
		{
			while (1) {
				bytes_read = tuh_midi_stream_read(dev_addr, &cable_num, buffer, MIDI_BUF_SIZE);
				if (bytes_read == 0) return;
				if (cable_num == 0) {
					i = 0;
					while (i < bytes_read) {
						// test values received from groovebox via MIDI
						switch (buffer [i]) {
// This part is not needed as we don't receive MIDI CLOCK signals (R-PICO cannot cope with the speed)
//							case MIDI_CLOCK:
//								midi_tx [index_tx++] = MIDI_CLOCK;
//								break;
							case MIDI_CONTINUE:
//								pause = true;
								break;
							case MIDI_PLAY:
//								play = true;
								break;
							case MIDI_STOP:
//								play = false;
//								pause = false;
								break;
							case MIDI_PRG_CHANGE:
//								if (buffer [i+1] <= 31) song = buffer [i+1];		// make sure song number is inside boudaries (0 to 31)
								break;
						}
						switch (buffer [i] & 0xF0) {	// control only most significant nibble to increment index in buffer; event sorting is approximative, but should be enough
							case 0x80:
							case 0x90:
							case 0xA0:
							case 0xB0:
							case 0xE0:
								i+=3;
								break;
							case 0xC0:
							case 0xD0:
								i+=2;
								break;
							case 0xF0:
								i+=1;
								break;
							default:
								i+=1;
								break;
						}
					}
				}
			}
		}
	}

	return;
}

// invoked when sending some MIDI data
void tuh_midi_tx_cb(uint8_t dev_addr)
{
	(void)dev_addr;
}