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
#define NB_INSTRUMENTS 9

#define MIDI_CLOCK		0xF8
#define MIDI_PLAY		0xFA
#define MIDI_STOP		0xFC
#define MIDI_CONTINUE	0xFB
#define MIDI_PRG_CHANGE	0xCF	// 0xC0 is program change, 0x0F is midi channel

#define LOAD		0x08
#define RESET_POS	0x18

#define LED_GPIO	25	// onboard led
#define LED2_GPIO	255	// 2nd led
const uint NO_LED_GPIO = 255;
const uint NO_LED2_GPIO = 255;

// type definition
struct songstep {
	int step_number;					// current step number
	int number_of_steps;				// number of steps in the song
	int number_of_channels;				// number of channels in the song
	uint16_t notes [CHANNEL_COUNT];		// channel data (ie. notes) for that step
	uint8_t pad_number;					// pad number on the MIDI control surface
	uint8_t pad_color;					// pad color on the MIDI control surface
};

struct midisend {						// this struct is used to send midi data
	uint8_t midilength;					// length of midi data to be sent
	uint8_t mididata [3];				// midi data; max 3 bytes
};

// globals
static uint8_t midi_dev_addr = 0;
static bool connected = false;
static int song_num = 0;			// by default, song number is 000
static int number_of_songs;			// number of song in song.h
static struct songstep cur_step;	// contains data for currently played step of the song
static struct songstep next_step;	// contains data for next step that should be played in the song
static int next_step_number;	// number of next step in the song

static bool load_pressed = false;			// used for loading functionality
static bool load_unpressed = false;			// used for loading functionality
static bool load = false;					// used for loading functionality


// midi buffers
#define RX_LG	500						// 500 bytes to receive
#define TX_LG	200						// 200 midi events to send
static uint8_t midi_rx [RX_LG];			// large midi buffer to receive data
static struct midisend midi_tx [TX_LG];	// large midi buffer to send data
static int index_tx = 0;				// number of events to be sent


// channels definition
using namespace synth;
synth::AudioChannel synth::channels[CHANNEL_COUNT];

// 0: melody
// 1: rhythm
// 2: drum
// 3: hi-hat
// 4: bass
// 5: piano
// 6: reed
// 7: pluckedguitar
// 8: violin

// waveform, attack in ms, decay in ms, sustain volume, sustain in ms, release in ms, channel volume
const uint16_t instruments[NB_INSTRUMENTS][7] = {
	Waveform::TRIANGLE | Waveform::SQUARE, 16, 168, 0xafff, 10000, 168, 10000,
	Waveform::SINE | Waveform::SQUARE, 38, 300, 0, 0, 0, 12000,
	Waveform::NOISE, 5, 10, 16000, 10000, 100, 18000,
	Waveform::NOISE, 5, 5, 8000, 10000, 40, 8000,
	Waveform::SQUARE, 10, 100, 0, 0, 500, 12000,
	Waveform::PIANO, 20, 20, 0xafff, 3000, 500, 10000,
//	Waveform::PIANO, 16, 168, 0xafff, 10000, 168, 10000,
	Waveform::REED, 16, 168, 0xafff, 100, 3000, 10000,
	Waveform::PLUCKEDGUITAR, 16, 168, 0xafff, 10000, 168, 10000,
	Waveform::VIOLIN, 16, 168, 0xafff, 10000, 168, 10000	
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


// write midi events stored in midi_tx buffer to midi out
// returns true if data was actually sent, false if not
bool send_midi (void)
{
	uint32_t nwritten, lg;
	uint8_t buffer [20];		// small buffer, we should not write more than 3 x 3 = 9 bytes at a time
	int i, number;
	int midilimit = 3;

	// set a limit of sending 3 midi events at a time, to avoid blocking launchpad
	number = index_tx;
	if (number > midilimit) number = midilimit;

	// build buffer to be sent, from midi data
	lg = 0;
	for (i = 0; i < number; i++) {
		memcpy (&buffer [lg], midi_tx [i].mididata, midi_tx [i].midilength);
		lg += midi_tx [i].midilength;
	}

	// for debug only
	// printf ("send : %d, num evts : %d, data : %02X %02X %02X\n", lg, index_tx, buffer[0], buffer[1], buffer[2]);

	if (connected && tuh_midih_get_num_tx_cables(midi_dev_addr) >= 1)
	{
		nwritten = tuh_midi_stream_write(midi_dev_addr, 0, buffer, lg);
		if (nwritten != lg) {
			TU_LOG1("Warning: Dropped %ld bytes\r\n", (lg-nwritten));
		}

		// data has been sent somehow: remove midi events that have been sent
		memmove (midi_tx, &(midi_tx [number]), TX_LG - number);
		// update number of events still to be sent
		index_tx -= number;
		return true;
	}
	return false;
}


// reset launchpad leds
void reset_launchpad ()
{
	// basically we just add the right midi command to outgoing midi flow
	midi_tx [index_tx].mididata [0] = 0xB0;
	midi_tx [index_tx].mididata [1] = 0x00;
	midi_tx [index_tx].mididata [2] = 0x00;
	midi_tx [index_tx++].midilength = 3;			// 3 bytes to send
}


// set launchpad led to its original color for a given step 
void set_led (struct songstep* step)
{
	// basically we just add the right midi command to outgoing midi flow
	// this is NOTE ON (pad number) (pad color)
	midi_tx [index_tx].mididata [0] = 0x90;
	midi_tx [index_tx].mididata [1] = step->pad_number;
	midi_tx [index_tx].mididata [2] = step->pad_color;
	midi_tx [index_tx++].midilength = 3;			// 3 bytes to send
}


// set launchpad led to a nice green color for a given step 
void set_green_led (struct songstep* step)
{
	// basically we just add the right midi command to outgoing midi flow
	// this is NOTE ON (pad number) (pad color)
	midi_tx [index_tx].mididata [0] = 0x90;
	midi_tx [index_tx].mididata [1] = step->pad_number;
	midi_tx [index_tx].mididata [2] = 0x3C;
	midi_tx [index_tx++].midilength = 3;			// 3 bytes to send
}


// set green leds for load button and reset position button
void set_function_leds (void)
{
struct songstep temp; 					// temporary structure to store step data for the song

	temp.pad_number = LOAD;		// load button
	set_green_led (&temp);
	temp.pad_number = RESET_POS;	// reset position button
	set_green_led (&temp);
}


// reset all launchpad leds 
void reset_leds ()
{
//int i, j;

	// reset launchpad basically resets leds; midi-wise, it is better than switching off each individual led
	reset_launchpad ();
	// set function leds in green
	set_function_leds ();

//	for (i = 0; i < 0x80; i+= 0x10) {
//		for (j = 0; j < 0x08 ; j++) {
//			// basically we just add the right midi command to outgoing midi flow
//			// this is NOTE ON (pad number) (pad color)
//			midi_tx [index_tx++] = 0x90;
//			midi_tx [index_tx++] = (i + j);
//			midi_tx [index_tx++] = 0x0C;	// led off
//		}
//	}
}


// get step data from the song and fill the step structure accordingly
// num is the song number, position is the step number
// returns true if step data is OK, false otherwise
bool get_step (int num, int position, struct songstep* step)
{
int pointer;
int i;


	// check if song exists by checking that song number is lower than number of songs
	if (num >= song [0]) return false;

	// get pointer to song data
	pointer = song [1+num];

	// get number of channels and steps
	step->number_of_channels = song [pointer++];
	step->number_of_steps = song [pointer++];

	// no need to load instruments of the song to the channels, this is done when loading a song
	// this could be done only once at song loading
	pointer += step->number_of_channels;

	// determine if we are still within the song; if not, return false
	if (position >= step->number_of_steps) return false;

	// update pointer so it points to step data
	pointer = pointer + (position * (step->number_of_channels + 2));	// nb_chan + 2 as we need to skip pad number and color

	// get song data (at position) and load structure with it
	// make sure we are not at the end of the song
	for (i = 0; i < step->number_of_channels; i++) {
		step->notes [i] = song [pointer];
		if (step->notes [i] == 0xFFFF) return false;
		pointer++;							// next position in song file
	}

	// fill pad number and pad color, and obviously step number
	step->pad_number = song [pointer++];
	step->pad_color = song [pointer];
	step->step_number = position;

	return true;
}


// get step data from a pad number pressed and fill the step structure accordingly
// num is the song number, start_from is the step number from which we want to start checking the data
// pad is the pressed pad number, for which we want the data
// returns true if step data is OK, false otherwise (for exemple : no data for this pad number)
bool get_step_from_pad_number (int num, int start_from, int pad, struct songstep* step)
{
struct songstep temp; 					// temporary structure to store step data for the song
int i;
int nbstep;

	// get step 0 data for the song to get total number of steps in the song
	if (get_step (num, 0, &temp) == false) return false;
	nbstep = temp.number_of_steps;			// total number of steps in the song

	// we start searching at (start_from) position then do a ring lookup.
	// This allows to have step information that is the closest to the step actually being played (otherwise, the found found pad would be returned)
	if (start_from > nbstep) return false;						// start_from out of boundaries
	
	
	// we now have the total number of steps in the song; go through each to determine if one of the steps corresponds to the pressed pad
	// from start_from to end of the song
	for (i = start_from; i < nbstep; i++) {
		if (get_step (num, i, &temp) == false) return false;	// in case we reached the end of the song or song file is inconsistent
		if (pad == temp.pad_number) {							// we have found the step corresponding to the pressed pad
			memcpy (step, &temp, sizeof (struct songstep));		// copy step data into destination passed as argument
			return true;
		}
	}

	// we now have the total number of steps in the song; go through each to determine if one of the steps corresponds to the pressed pad
	// from beginning to start_from
	for (i = 0; i < start_from; i++) {
		if (get_step (num, i, &temp) == false) return false;	// in case we reached the end of the song or song file is inconsistent
		if (pad == temp.pad_number) {							// we have found the step corresponding to the pressed pad
			memcpy (step, &temp, sizeof (struct songstep));		// copy step data into destination passed as argument
			return true;
		}
	}

	return false;	// we went through all the song, we didn't find any step corresponding to the pressed pad
}


// plays the notes contained in songstep structure
void update_playback (struct songstep* step) {

int i;

	// get notes data from the structure, and pass it to synthetizer
	for (i = 0; i < step->number_of_channels; i++) {
		channels[i].frequency = step->notes [i];
		channels[i].trigger_attack();
	}
}


// release all active channels for a song
void stop_playback () {

  // we must update the playback with release on all channels

	for (uint8_t i = 0; i < CHANNEL_COUNT; i++) {
    	// if channel is in OFF state, then do nothing
    	// if channel is already in release state, then do nothing
    	// if channel is in another state, then go to release state
    	if ((channels[i].adsr_phase != ADSRPhase::OFF) && (channels[i].adsr_phase != ADSRPhase::RELEASE)) {
			channels[i].trigger_release();
		}
	}
}


// shut down all the channels
void reset_playback () {

	// we must stop all channels
	for (uint8_t i = 0; i < CHANNEL_COUNT; i++) {
		channels[i].off ();
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
	channels[chan].sustain_ms  = instruments [instr][4];
	channels[chan].release_ms  = instruments [instr][5];
	channels[chan].volume      = instruments [instr][6];
  
	return true;
}


// reset position in the song to step 0 (start)
// returns false if an issue has occured
bool reset_position (bool is_next_step)
{
	// in case "next_step" exists already, color of "next step" pad shall be set back to normal
	if (is_next_step) set_led (&next_step);

	// initialize next step that should be played in the song to 0
	// set corresponding led in green
	// copy also to current step for safety
	next_step_number = 0;
	if (get_step (song_num, 0, &next_step) == false) return false;
	memcpy (&cur_step, &next_step, sizeof (struct songstep));
	set_green_led (&next_step);		// set next step pad led in green
	return true;
}


// load song with number NUM
bool load_song (int num)
{
int pointer;
int nb_chan;
int nb_step;
int i;
struct songstep temp_step;

	// check if song exists by checking that song number is lower than number of songs
	if (num >= song [0]) return false;

	// make sure all channels are off
	reset_playback ();
	// clear launchpad leds, set function leds on
	reset_leds ();

	// get pointer to song data
	pointer = song [1+num];

	// get number of channels and steps
	nb_chan = song [pointer++];
	// get number of steps
	nb_step = song [pointer++];

	// load instruments of the song to the corresponding channel
	for (i = 0; i < nb_chan; i++) {
		if (load_instrument (song [pointer++], i) == false) return false;
	}

	// go through each step and light the corresponding pad
	for (i = 0; i < nb_step; i++) {
		if (get_step (num, i, &temp_step) == false) return false;
		set_led (&temp_step);
	}

	// set starting position to 0, and set next_step accordingly
	if (reset_position (false) == false) return false;

	return true;
}


// in case of error, reset the playback so no sound is made
void error()
{
	printf("Error in song data.\n");
	reset_playback ();
}



int main() {
// MISSING:
// X-LOAD FCT
// X-SET POSITION TO 0 IN THE SONG
// X-RAZ Launchpad
// X-test du retour des appels à load song, get_step : si false, ne pas jouer de son...
// X-ring buffer pour envoi des événements MIDI
// X-faire une durée sur le sustain?
// -rajouter une commande à 2 doigts
// -travail sur les sons : regler l'enveloppe des sons, rajouter des sons
// X-tester les sons: tous les sons + polyphonie, monophonie
// X-rajout de sons : - 256 positions, plus de positions
// X-rajout de sons: vrai son piano, etc
// X stop previous sound : do we need to do this? need to check:
// X1- whether playing new sound stops previous sound --> oui si sur le même channel
// X2- whether having 0 as sound frequency stops sound in the current channel
// 


	int i, j, k;
	struct songstep temp_step;


	stdio_init_all();
	board_init();
	printf("Picopadnion\r\n");
	sleep_ms (2000);	// 2 sec wait to make sure launchpad is awake properly

	// configure USB host
	tusb_init();

	// configure audio
	struct audio_buffer_pool *ap = init_audio(synth::sample_rate, PICO_AUDIO_PACK_I2S_DATA, PICO_AUDIO_PACK_I2S_BCLK);


	// load song 000 by default, and set green leds for load button and reset position button
	// set all the leds, load next step, set next step to #0
	number_of_songs = song [0];
	if (!load_song (song_num)) error ();


	// main loop
	while (true) {

		tuh_task();
		// check connection to USB slave
		connected = midi_dev_addr != 0 && tuh_midi_configured(midi_dev_addr);


		// load functionality
		if (load_pressed) {			// load pad has just been pressed
			reset_leds ();			// clear leds and light function leds
			// set leds on , according to the number of songs
			k = 0;
			while (k < number_of_songs) {		// load mode : set exisiting songs as green buttons (green pads)

				i = k / 8;
				j = k % 8;
				temp_step.pad_number = (i * 0x10) + j;		// set song button to green
				set_green_led (&temp_step);
				
				k++;
			}
			load_pressed = false;
			load = true;
		}
		if (load_unpressed) {							// load pad has just been unpressed
			load = false;
			load_unpressed = false;
			if (!load_song (song_num)) error ();		// load new song according to song_num
		}


		// in case some MIDI data is to be sent, then send it
		if (index_tx) {
			if (!send_midi ()) printf ("Could not send midi out, %d midi events in buffer\n", index_tx);
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
	uint8_t j, k, l;
	uint32_t i;
	uint32_t bytes_read;
	struct songstep temp_step;

	// set midi_rx as buffer
	buffer = midi_rx;
	
	if (midi_dev_addr == dev_addr)
	{
		if (num_packets != 0)
		{
			while (1) {
				bytes_read = tuh_midi_stream_read(dev_addr, &cable_num, buffer, RX_LG);
				if (bytes_read == 0) return;
				if (cable_num == 0) {
					i = 0;
					while (i < bytes_read) {
						// test values received from midi surface control via MIDI protocol
						switch (buffer [i] & 0xF0) {	// control only most significant nibble to increment index in buffer; event sorting is approximative, but should be enough
							case 0x90:	// note on
								// Would it be useful to put this in the main loop rather than in this callback ?

								// test velocity : if velocity is 0, then pad is released on the launchpad
								if (buffer[i+2] == 0) {
									// if pad release is the same number as the last pad that was pressed, then sound off
									if (buffer [i+1] == cur_step.pad_number) stop_playback ();
									// if load button is unpressed, then set relevant state
									if (buffer [i+1] == LOAD) load_unpressed = true;

									i+=3;
									break;
								}
								
								// else velocity is 0x7F: pad is pressed on launchpad
								// reset position functionality
								if (buffer[i+1] == RESET_POS) {
									reset_position (true);
									reset_playback ();
									i+=3;
									break;
								}

								// load functionality
								if (buffer[i+1] == LOAD) {
									load_pressed = true;
									reset_playback ();
									i+=3;
									break;
								}
								if (load) {								// we are in load functionality
									j = buffer [i+1] & 0x0F;			// determine which song has been pressed according to pad number
									k = (buffer [i+1] >> 8) & 0x0F;
									if ((j < 8) && (k < 8)) {			// test boundaries of selected pad, to see if this corresponds to an existing song
										l = (k * 8) + j;
										if (l < number_of_songs) song_num = l;	// set song_number with the new value (ie. pad number of pad which has been pressed)
									}
									i+=3;
									break;
								}

								// determine if the pressed pad is assigned to a step, and which step; we start checking the pads from next_step_number
								// to make sure we don't miss the next step
								if (!get_step_from_pad_number (song_num, next_step_number, buffer [i+1], &temp_step)) error ();
								else {
									// we have pressed a pad that is assigned to a step
									if (temp_step.pad_number == next_step.pad_number) {
										// we have pressed the "next step" pad; for safety, copy "next step" values in "temp" 
										memcpy (&temp_step, &next_step, sizeof (struct songstep));
										// color of "next step" pad shall be set back to normal
										set_led (&temp_step);
										// determine next step in the song
										if (++next_step_number >= temp_step.number_of_steps) next_step_number = 0;
										// load new next step structure
										if (!get_step (song_num, next_step_number, &next_step)) error ();
										// set led color of next step in a nice green
										set_green_led (&next_step);
									}
									// no need to stop previous sound as:
									// 1- whenever playing new sound stops previous sound on the same channel
									// 2- whether having 0 as new sound frequency stops sound in the same channel

									// pressed pad is becoming the current pad: fill pad structure
									memcpy (&cur_step, &temp_step, sizeof (struct songstep));
									// play new sound
									update_playback (&cur_step);
								}
								i+=3;
								break;
							case 0x80:	//note off
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