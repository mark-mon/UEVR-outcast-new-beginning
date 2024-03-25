# UEVR-outcast-new-beginning
Mod for 1st/3rd person toggle (and maybe other stuff) for UEVR for outcast new beginning

The mod does 2 things:

1) detects when you're in a conversation and puts you back into 3rd person view so your character isn't invisible
   
2) When you hit X for melee, you are in 3rd person for 2 seconds then automatically go back into 1st person. This is so you can button mash and combo with X easily. It works much better.
The 6 dof stuff was done by @legaiaflame . 
I also added some user_script and cvar values so you should be able to play in native stereo not sequential.
Also, make sure you load the game in DX12 mode not DX11 mode. Not only is DX12 mode better for performance, but also it helps eliminate some weird right eye only effects (like the bird in the title screen on the right vanishing).

If at any point you get out of sync with the 1st / 3rd person, (hopefully this never happens), you can just go into the UEVR menu and toggle UObject hook back on. (The mod turns it off for 3rd person sequences).

Note: the mod requires you to have bound F4 to the UObject enable / disable key in UEVR menu. It sends F4 to change first and third person views.
This line in config.txt
UObjectHook_ToggleUObjectHookKey=115
