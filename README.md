# picopanion
picopanion is a music box creating musical accompaniment for a live band.
It uses a raspberry pico, a Novation Launchpad mini as UI (user interface) and [Pimironi's PICO AUDIO](https://shop.pimoroni.com/products/pico-audio-pack?variant=32369490853971) extension board.   
This project heavily relies on [rpicomidi](https://github.com/rppicomidi) work, and in particular on its [MIDI host driver for tinyUSB](https://github.com/rppicomidi/usb_midi_host) which allows raspberry pico to become a USB MIDI host.   

## The problem
If you music band is to small, you may want to have some harmonic support in the form of a piano or strings.   
However, if your band is too small, you may not be able to play these instruments, or you may be busy with something else (bass, drum, singing...)!   

## The solution
picopanion is a programmed music box (that is: following a note sheet) that plays a chord at a press of a button. At the next press, it plays the next note/chord of the song, etc.   

picopanion is made of a Raspberry PI Pico, Novation Launchpad Mini as UI (optional UI can be provided by GPIO switches). Pimironi's pico audio extension board is used to play the music from waveforms generated by the raspberry Pico.
