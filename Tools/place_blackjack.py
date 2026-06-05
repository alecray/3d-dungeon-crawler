"""Headless: place an editable ABlackjackTable actor into L_Town near the player start (replacing the old
code-spawn), then save the level so it can be repositioned in-editor.

Run AFTER building, with the editor closed:
  UnrealEditor-Cmd.exe <proj> -run=pythonscript -script="Tools/place_blackjack.py" -unattended -nopause -nosplash
"""
import unreal

ELL = unreal.EditorLevelLibrary
ELL.load_level("/Game/Maps/Town/L_Town")

# Find the player start to anchor the table near where the old code-spawn put it.
loc = unreal.Vector(0.0, 0.0, 0.0)
yaw = 0.0
for a in ELL.get_all_level_actors():
    if isinstance(a, unreal.PlayerStart):
        sl = a.get_actor_location()
        sr = a.get_actor_rotation()
        fwd = unreal.MathLibrary.get_forward_vector(sr)
        right = unreal.MathLibrary.get_right_vector(sr)
        loc = sl + fwd * 360.0 + right * (-250.0)
        loc.z = sl.z - 88.0  # player-start foot ~= floor; the table base sits at its actor origin now
        yaw = sr.yaw
        break

# Don't add a second one if it's already placed.
existing = [a for a in ELL.get_all_level_actors() if isinstance(a, unreal.BlackjackTable)]
if existing:
    unreal.log("Blackjack table already in L_Town ({} found) — leaving as is".format(len(existing)))
else:
    actor = ELL.spawn_actor_from_class(unreal.BlackjackTable, loc, unreal.Rotator(0.0, 0.0, yaw))
    if actor:
        actor.set_actor_label("BlackjackTable")
        unreal.log("Placed BlackjackTable at {}".format(loc))
    ELL.save_current_level()
    unreal.log("Saved L_Town")
