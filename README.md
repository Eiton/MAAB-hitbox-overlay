# MAAB-hitbox-overlay

This is a tool to viualize the hitbox in the PC version of the game Million Arthur Arcana Blood, inspired by the <a href="https://www.reddit.com/r/Guiltygear/comments/42hql7/guilty_gear_xrd_pc_hitbox_overlay/" target="_blank">GGXRD version </a>

Most of the values and addresses are hardcoded. Therefore, the tool is not guarenteed to work on every machine.

## How to use
* Build the project. 
* Make sure you have downloaded the DirectX sdk, Microsoft Research Detours Package and added the paths to the Library Directories.
* You can download the built dll file <a href="https://drive.google.com/file/d/1KVRaZvgeML4hd1DrnxtmbYhUjvHjDHD9/view?usp=sharing">here</a>

* Launch the game
* Use any dll injecter to inject the dll


## Screenshot
<img src="https://i.imgur.com/MAjtpG3.png"/>

##Known Issues
* The hitbox drawn on each frame actually uses the data of the last frame.
* Hitbox of throws is not drawn
* Hitbox is not drawn when either character is in evasive dash/roll, where the character is hit invincible and can pass through the opponent.
