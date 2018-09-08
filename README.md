This is a simple parser for the files that are produced by the Kaos Pad 3+.

I needed to extract the audio stored in a slot from the data saved directly by
the KP3+. Luckily it was pretty straight forward! The file extension for this
file is P3A. It is stored in 512 byte pieces, likely for the sector size of
the SD card. If a chunk is less than the sector/block size, it'll be padded
with 0xFF.

The example program reads the P3A file and then writes out the 4 samples as
.WAV files. They are stored at 48000KHz 16 bit, Stereo LE format in the .WAV
file, swapped from the BE format used on the KP3+.

I haven't yet figured out the rest of the settings, but I know that the
PROG fields correspond to the 8 stored FX buttons and that the GLOB field
contains the rest of the settings information. For example, I think the 
tempo is a 16 bit BE field at offset 19 from the start of the GLOB chunk, but
I'd have to do more work to map out all of the options. A task for another day!

Hope you find this useful!
