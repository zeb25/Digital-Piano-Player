# Final-Project 
Code Written Zoe Bell & Purva Shirolkar

## Summary
A piano, C++, graphics program that uses an audio library from PortAudio to generate sound corresponding to keys of a single piano octave.
The program opens up to the start screen which offers two options for the user: press 'p' to practice a song or press 's' to freely
play the piano. The freePlay mode allows the user to play the piano using their laptop keyboard. The bottom two rows of the letter keys
correspond to the keys of the piano. The gamePlay mode goes through the keys to play "Mary Had a Little Lamb" on the piano once before leaving
the screen open for the user to practice.

## OS
This program can run on any operating system including Windows and MacOS.

## Concepts
We used the graphics code from the confetti button project from Module 4 as a base because we could implement some of the same ideas of 
generating buttons and having the user interact with those buttons, or in this case keys, to produce a reaction (sound playing). We also used
several other graphics concepts from module 4 to generate the vector of piano keys.

## How to run
This program runs straight on your OS in a pop-up graphical window. 

## Installations

## Citations
* 'playSine.h' source code:
  * * PortAudio Portable Audio Library.
  * * For more information see: http://www.portaudio.com/
  * * Copyright (c) 1999-2000 Ross Bencina and Phil Burk

## Grading structure
The project is out of 200 points. From where it is in its current state it is probably worth around 160 points. While the
piano key sounds don't change pitch, they work correctly to correlate to each individual key and still play when the user presses
the correct key on their keyboard. The piano graphics work correctly and part of the game logic we hoped to implement is also working. 

| Points Possible | Description of requirement                                               |
|-----------------|--------------------------------------------------------------------------|
| 100 pts         | Learning and implementing of the audio library chosen IDE                |
| 40 pts          | Integrating graphics (screens and piano)                                 |
| 50 pts          | Game component: implementing the logic of the song practice mode         |
| 10 pts          | Collaboration: all code committed to Github correctly, ReadMe filled out |
