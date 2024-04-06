# UEVR-outcast-new-beginning
Mod for 1st/3rd person toggle (and maybe other stuff) for UEVR for outcast new beginning

The mod does these things:

1) detects when you're in a conversation and puts you back into 3rd person view so your character isn't invisible
   
2) When you hit X for melee, you are in 3rd person for 2 seconds then automatically go back into 1st person. This is so you can button mash and combo with X easily. It works much better. This is configurable via the config file.

3) Detects when you're in a cinematic and switches back to 3rd person view so that the cinematic cameras are correct.
   
4) There's an option in the config file to put you into third person when gliding now. This is much easier to complete some of the trials and such.

5) There's an option in the config file to swap LT and RB buttons so that you aim by squeezing the controller in right hand. Menus still use RB and LB the same way.

If at any point you get out of sync with the 1st / 3rd person, (hopefully this never happens), you can just go into the UEVR menu and toggle UObject hook back on. (The mod turns it off for 3rd person sequences).

Note: the mod requires you to have bound F4 to the UObject enable / disable key in UEVR menu. It sends F4 to change first and third person views.
This line in config.txt
UObjectHook_ToggleUObjectHookKey=115

This takes a config file called "PersonToggle.txt" that goes in the same folder with the plugin.
These are the fields in the config file. Lack of a config file will just use defaults.

`#` Options are 1 or 0. 1 for on, 0 for off

`#` Pressing X for melee toggles 3rd person for 2 seconds. Better Melee
XButtonThirdPerson=1

`#` Right stick up on toggles first and third person
RightStickUpToggleThirdPerson=0

`#` Right Stick Down sends B button for dodge
RightStickDownB=0

`#` LB / RB (gliding) in 3rd person
ThirdPersonGlide=1

`#` Swap LT and RB for single hand squeeze aiming.
SwapLTRB=1
