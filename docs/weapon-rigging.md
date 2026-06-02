# Weapon Rigging & Hookup — Blender → Unreal (UE 5.7)

Convention: pivot at the **grip**, blade along **+X** (UE forward), **+Z** up. Keep it consistent
across all weapons.

## A. Rig the weapon (one-bone rig is enough for a rigid weapon)
1. Origin at the grip; blade pointing +X. Then `Object → Apply → All Transforms` (Ctrl+A).
2. `Add → Armature`. In Edit Mode, place the bone running down the blade (root at grip, tip at blade
   tip). Rename it e.g. `Weapon`.
3. Bind the mesh: select **mesh** first, then **shift-select armature**, `Ctrl+P → With Empty Groups`.
4. Weight the mesh to the bone: select the mesh, Weight Paint (or Edit Mode → select all → in the
   Object Data "Vertex Groups", assign all verts to the bone with weight **1.0**).
5. Verify in Pose Mode: rotating the bone moves the whole weapon.

## B. Author the swing animation
1. Animation workspace, Pose Mode on the armature.
2. Frame 1: pose at **rest/idle**, select bone, `I → Rotation` to keyframe.
3. ~Frame 8: **wind-up** pose, keyframe.
4. ~Frame 16: **slash** pose, keyframe.
5. ~Frame 30: return to the **exact rest pose**, keyframe. (Must end at rest — the played anim holds
   its last frame.)
6. In Dope Sheet → Action Editor, name the action (e.g. `Weapon_Swing`). Keep it short
   (~15–30 frames @ 30 fps).
7. **Keep the action so it exports** (an action with no "user" is purged on save/reload — the usual
   cause of a "disappearing" animation):
   - Click the **shield icon** 🛡 next to the action name (Fake User), **and/or**
   - **Push it down to an NLA strip:** in the Action Editor, open the **Action menu dropdown** (the
     dropdown in the editor header) → **Push Down**. This stores the action as a strip on its own NLA
     track. (Note: the **NLA Editor** is its own editor pane — like the 3D Viewport or Dope Sheet, you
     pick it from the **editor-type dropdown** in the corner of any pane — where you can see and manage
     the pushed-down strips.)
   - For a single animation, keeping it the **active action** + Bake Animation ON (Section C) is enough.
     If you have **multiple** actions, push each down and tick **"NLA Strips"** / **"All Actions"** in the
     FBX export options.

### How to add a keyframe
You must be in **Pose Mode** with the **bone** selected (keying the mesh object won't animate the
deform).

- **Manual (recommended):**
  1. Set the playhead to the target frame — click that frame on the **Timeline** (or type it in the
     frame field).
  2. Pose the bone (`R` to rotate into position).
  3. **Hover over the 3D viewport** and press **`I`** → choose **Rotation** (or **LocRotScale** if you
     also moved/scaled). A keyframe diamond appears on the Timeline / Dope Sheet.
- **Auto-key (faster):** click the **record button** (⏺ circle) on the Timeline; transforming the bone
  at any frame now inserts keys automatically. **Turn it off** when done to avoid stray keys.

Tips:
- Keyframes show as diamonds in the **Timeline** and **Dope Sheet**; drag to retime, `X` to delete a
  selected one.
- Right-click a key → set **interpolation** (Bezier = smooth ease, Linear = constant speed) to tune
  the swing's feel.
- If `I` does nothing or is greyed out, you're not in Pose Mode or no bone is selected.

## C. Export FBX
1. Select **both** the armature and the mesh.
2. `File → Export → FBX`.
3. Settings:
   - Path Mode: Copy (toggle the "embed textures" icon if desired)
   - Limit to: **Selected Objects**
   - Object Types: **Armature + Mesh** only
   - Armature: untick **Add Leaf Bones**
   - Bake Animation: **ON**; untick "All Actions" (export just this action); "Force Start/End Keying" ON
   - Keep the same Scale/axis settings you used for static meshes (UE = cm)
4. Export to your project's `blends/<category>/` folder.

## D. Import into Unreal
1. Drag the FBX into the right Content folder (e.g. `Content/Weapons/`).
2. Import dialog: **Skeletal Mesh = ON**, **Import Mesh = ON**, **Import Animations = ON**. Import.
3. You get a **Skeletal Mesh**, a **Skeleton**, and an **Animation Sequence**.
4. Rename to match your code paths (prefixes): `SK_<Name>`, `SKEL_<Name>`, `A_<Name>_Swing`.
5. Assign the weapon material to the skeletal mesh's material slot.

## E. Hook up in code (this project's pattern)
- The weapon is a `USkeletalMeshComponent` on the character, parented to the camera, positioned with
  `SwordBase` + a component rotation.
- It's loaded by path in `BeginPlay` (e.g. `/Game/Weapons/SK_Sword`, `/Game/Weapons/A_Sword_Swing`)
  — or assign the `UPROPERTY` slots in a derived BP.
- `SetAnimationMode(AnimationSingleNode)` in the constructor; on attack:
  `SwordMesh->PlayAnimation(SwingAnim, /*loop*/ false)`.
- Residual orientation from the Blender→UE axis swap is corrected with the component transform in
  code, not in Blender.

## Don't-forget checklist
- [ ] Origin at grip, transforms applied
- [ ] All verts weighted 1.0 to the bone
- [ ] Animation starts AND ends at rest pose
- [ ] Export: Selected Objects, Armature+Mesh, Bake Animation ON, no Leaf Bones
- [ ] Import: Skeletal Mesh + Animations ON
- [ ] Renamed to `SK_` / `A_` and material assigned
