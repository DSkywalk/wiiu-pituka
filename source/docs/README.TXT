************************************

	PituKa V1.0D
	SOURCE CODE

************************************


 I would like to have in this 1.0
==================================
 I spend some months trying change the sound code to have better support but I cant find
 the correct mode, or the sound gliches or directly the emulation turns slooow... anyway
 i want public the code for the scene adds his optimizations ;)

 I dont think that we have better sound (and more quickly) even you dont change the Z80 core,
 if you want avance the PituKa proyect you can start with this... in the forums of GP32spain.com
 the people is coment that we can answer to Reesy that if he want apply his core to Pituka (he
 send me his code but i dont know to apply them ¿any is encourage to coment him?
 
 Some easy Tasks to implement:
	- In explorer: order ROMS by name.
	- In explorer: directory support.
	- Key repeating in vkeyboard.

 COMPILE OPTIONS USED
======================
 opts=-march=armv4t -mtune=arm9tdmi -marm -mno-thumb-interwork -ffast-math -fshort-enums -fno-common 
 -ffreestanding -fno-builtin -fno-exceptions -mstructure-size-boundary=32 -fomit-frame-pointer -Wall -falign-functions 
 -falign-jumps -falign-loops -falign-labels -finline-limit=500

Link: cap32.o crtc.o fdc.o tape.o psg.o z80.o rlyeh/sound/gpsoundbuf.o rlyeh/snaps/snapshot.o port/pituka.o port/GPio.o port/GPprintf.o port/files/explorer.o chatboard-mirko/gp_chatboard.o chatboard-mirko/gp_irq.o

 TODO:
=======
0.	Apply ASM/ARM9 Reesy z80 Core
1. 	Better PSG CPC emulation (digital effects)
2. 	Dsk Save.
3. 	ZIP Support
4. 	CDT Support

See you
	D_Skywalk

http://gp32.dantoine.org

PituKa its based on Caprice 4.1.0

English Translation: D_Skywalk (sorry shakespeare LOL).

Thanks to:  	Rlyeh, kotah, Franxis, Una-i
		Damizean, Pelayo, vnomis, Wonder_Boy
		K-teto, Fagotero, Hexdump, A600
		to my mumm, girlfried and her poof bear: PituKa
		& the pp of #gp32dev on efnet & Irc-Hispano :D

Email: dantoine [@] gmail [dot] com
