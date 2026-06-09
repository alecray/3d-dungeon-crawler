# Art TODO

> Art-asset-creation tasks moved out of `TODO.md` — models, meshes, textures, icons, materials, and
> animations that need to be authored (Blender / in-editor). The **code/gameplay hooks** that consume these
> assets stay in `TODO.md`; this file is just the "make the art" side.
>
> Brainstorm / models-to-make ideas live in `ideas.md` (untouched). Audio asset creation stays in `TODO.md`.

## Models / meshes

- [ ] Fix the weapon-rack asset (SM_Weapon_Rack) — it spawns tilted + sunk into the floor; set it upright
      with its pivot/origin at the base. Then audit all other props' pivots (origin at base, upright) so
      nothing floats or clips into the floor when placed.
- [ ] Chest model and opening animation
- [ ] Vendor / shop model (currently graybox) — shopkeeper stall / counter
- [ ] Portal model (currently a code-built procedural "energy gate") — proper portal mesh/frame
- [ ] Scenery props — clutter scattered on the floor and on top of tables/surfaces:
  - Tabletop: candles & candlesticks, plates/bowls, mugs/tankards, bottles & flasks,
    books/scrolls, quill + inkpot, food (bread, cheese, apple), coin stacks, gems,
    potion vials, maps/parchment, dice, knives, small pouches
  - (more models to make are listed in `ideas.md`)
- [ ] Swap graybox: hermit-crab model, real gate/portcullis door mesh, portal mesh.

## Boss animations (hermit crab)

Hooked up in code via the anim paths on `ABossMonster` (idle/walk/attack auto-load; the rest need a
code hook once the clip exists). The `bAbilitiesEnabled` debug gate has been **removed** — tier 0 now phases for real (2 lives, heal-to-full
on 0 HP) with the ground slam live in phase 2; other specials are reserved for tier 1+ (deferred). Any
special whose anim clip isn't authored yet simply plays no clip (the gameplay still fires).
- [x] **Idle** — `A_Hermit_Crab_Boss_Idle`
- [x] **Walk** — `A_Hermit_Crab_Boss_Walk` (used for scuttle + lunge movement)
- [x] **Attack** — `A_Hermit_Crab_Boss_Attack` (plays on the standard melee swing)
- [x] **Spawn / intro** — rise + roar when the encounter starts (replaces the graybox scale-up "morph").
- [x] **Death** — collapse/flip when killed (currently just the DeathPoof + dissolve).
- [x] **Hit-react / flinch** — quick recoil when damaged (currently the cube hit-react pop).
- [ ] (special *animations* — the ground-slam gameplay is already live in tier-0 phase 2; these clips
      still need authoring + a play hook. `A_Hermit_Crab_Boss_Charge` / `_Smash` assets exist but aren't wired yet)
  - [ ] **Lunge / charge** wind-up + dash tell (telegraph before the dash).
  - [ ] **Ground slam** raise + slam (gameplay done; anim clip not yet hooked).
  - [ ] **Projectile volley** cast.
  - [ ] **Summon adds** cast.
  - [ ] **Shell-retreat** tuck-in + emerge (phase 3).
  - [ ] **Enrage** roar (phase 3).

## Materials / shaders

- [ ] Boss spawn-in **dissolve/materialize shader** (option B — the richer version of the code VFX that's
      already in via `ABossSpawnVFX`). Needs a material asset (can't be done in pure C++), then a small
      C++ hook to drive it. Steps:
  1. Author a master material `M_BossDissolve` (or a Material Instance over the crab's base): add a
     **`Dissolve`** scalar param (0..1) and an **`EdgeColor`** vector param (emissive). Sample a tiling
     noise texture; `if (Noise < Dissolve) discard` via **Opacity Mask** (set Blend Mode = Masked). Drive
     the emissive: a thin band where `Noise` is just above `Dissolve` glows `EdgeColor` (e.g.
     `EdgeGlow = saturate(1 - abs(Noise - Dissolve)/EdgeWidth) * EdgeColor`), so the dissolve edge burns.
  2. Assign it to `SK_Hermit_Crab_Boss` (all material slots).
  3. In C++: in `ABossMonster::PlayIntro`/`UpdateIntro`, grab the mesh's material as a Dynamic Material
     Instance (`GetMesh()->CreateDynamicMaterialInstance(slot)`) and animate `Dissolve` from 0→1 over the
     intro (`SetScalarParameterValue("Dissolve", A)`), set `EdgeColor` to the summon tint. Boss "forms
     into existence" with a glowing dissolve edge. Keep `ABossSpawnVFX` running alongside for the pillar/shards.
  4. (optional) Reverse it on death for a dissolve-out.

## Icons

- [ ] Item icons + potions — the 3D thumbnails need a lot of improvement.
- [ ] Better item icons — current render-to-texture silhouettes are recognizable but rough
  (flat shading, no fill/depth, weapons-only). Options: tune the icon studio lighting/material for
  nicer 3D thumbnails, add proper icon meshes for all items (potions, gems, etc.), or hand-authored
  2D icon textures per item.

## Niagara VFX (art pass — replaces the code-driven graybox effects)

The current effects spawn plain meshes + lights from C++ and animate them by hand (cheap, asset-free,
diff-friendly, but boxy). Once we're doing the art pass, author proper **Niagara systems** (editor assets)
and swap them in where these code effects fire — soft textured sprites, GPU particle counts, ribbons, etc.
Keep the C++ spawn points; just point them at Niagara systems (or gate code-vs-Niagara behind a flag).
- [ ] Niagara replacements for: impact spark bursts (`AImpactBurst`), ambient dust (`AAmbientDust`),
      death poof (`ADeathPoof`), level-up burst, loot beams (`AItemPickup`), **boss spawn-in**
      (`ABossSpawnVFX` — pairs with the dissolve shader option B above), and the batch-3 effects above.
- [~] **Mage fireball art (`NS_Fireball` + `NS_FireballImpact`)** — the mage spell bolt (`AProjectile`)
      currently shows a **Tier 0** code-driven look: an emissive orange orb + flickering point light (no
      assets). The C++ is already scaffolded for the art (`AProjectile::EnableFireballVisual`): author a
      Niagara system at **`/Game/VFX/NS_Fireball`** (in-flight: glowing core + additive fire material +
      ember/spark trail + ribbon) and a hit burst at **`/Game/VFX/NS_FireballImpact`**. They auto-load by
      path — once they exist the orb is hidden, the system plays in-flight, and the burst spawns on hit;
      **no code change needed**. (Research notes: stylized = simple additive panner/SubUV, not fluid sim;
      Infinity Blade: Effects has free fire textures. Optional later: charge-up VFX over cast frames 0-9.)
- [ ] Niagara plugin/module dependency — **done** (`Niagara` added to `DungeonCrawler.Build.cs` for the
      fireball). Still TODO: a small helper to spawn a system at a transform with a tint param so each
      call site is a one-liner. — partial: `Niagara` is confirmed in `PublicDependencyModuleNames` (Build.cs); `NiagaraFunctionLibrary::SpawnSystemAtLocation` is called in `Projectile.cpp`; the one-liner helper is not yet written.
