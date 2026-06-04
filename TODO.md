# TODO

> Open / untested work is up top. Completed items are in **Done** at the bottom.

---

# OPEN

## Gameplay

- [ ] Adjust the death flow (DESIGN DECISION pending) — currently death keeps all progression and
      reloads the same dungeon, no stakes. Decide: respawn in the dungeon vs. back in town; possibly
      require a consumable "life scroll" to respawn (else lose the run / drop loot). Ties into run-stakes
      + the town-portal difficulty/risk-reward idea in the backlog.
- [x] Mage weapon — added `Wand` ("Apprentice Wand") + `Staff` ("Oak Staff") items with `EEquipKind::Staff`;
      equipping one sets the **Mage** style (spell bolts cost mana). Held-mesh swap-in point at
      `/Game/Weapons/Staff/SK_Staff` (graybox/hidden until art lands). They roll in the loot pool.
- [ ] Trophy cases — placeable displays (e.g. in town) where players can show off items earned from the collection log

## Boss — next

### Boss animations needed (hermit crab)
Hooked up in code via the anim paths on `ABossMonster` (idle/walk/attack auto-load; the rest need a
code hook once the clip exists). **Currently in-engine: Idle, Walk, Attack only.** While the missing
ones are authored the boss runs in anim-test mode (`bAbilitiesEnabled = false`: phase 1, no specials).
- [x] **Idle** — `A_Hermit_Crab_Boss_Idle`
- [x] **Walk** — `A_Hermit_Crab_Boss_Walk` (used for scuttle + lunge movement)
- [x] **Attack** — `A_Hermit_Crab_Boss_Attack` (plays on the standard melee swing)
- [ ] **Spawn / intro** — rise + roar when the encounter starts (replaces the graybox scale-up "morph").
- [ ] **Death** — collapse/flip when killed (currently just the DeathPoof + dissolve).
- [ ] **Hit-react / flinch** — quick recoil when damaged (currently the cube hit-react pop).
- [ ] (specials, only once `bAbilitiesEnabled` is turned back on)
  - [ ] **Lunge / charge** wind-up + dash tell (telegraph before the dash).
  - [ ] **Ground slam** raise + slam.
  - [ ] **Projectile volley** cast.
  - [ ] **Summon adds** cast.
  - [ ] **Shell-retreat** tuck-in + emerge (phase 3).
  - [ ] **Enrage** roar (phase 3).

- [ ] **Remove the temporary `bAbilitiesEnabled` flag** (`ABossMonster`) once the anims above are in —
      restore the full fight unconditionally and delete the flag + its `Tick` guard + README note.
- [ ] VERIFY IN PIE: intro camera framing, hold length, doors line up + actually block, input hands
      back cleanly, boss scuttles/lunges without jitter at the LoS hand-off, double-height room (stacked
      wall courses, no clipping), red attack telegraph + dodge-only zone feel.
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
- [ ] Pincer-sweep cone telegraph (replace/augment the radius-only slam).
- [ ] Swap graybox: hermit-crab model, real gate/portcullis door mesh, portal mesh.
- [ ] Support multiple boss types (only one boss id today).
- [ ] (later) Burrow & resurface — boss digs under, re-emerges near the player (not for this boss).

## Flow / UX

- [x] Insufficient-resource feedback — denied actions (dash/melee/ranged with no stamina, mage with no
      mana) flash the matching HUD bar red. Still TODO (optional polish): a denial *sound* + on-screen
      "Not enough X" text + flashing the Q-ability cost too.
- [ ] Rebindable controls — let the player change key bindings from the settings menu (click a row,
      press a new key); persist the remaps in the save profile and apply to the Enhanced Input mappings.
- [ ] (later) Dedicated main-menu level — needs an empty .umap authored in-editor; then point the
      GameMode/GameDefaultMap at it instead of overlaying the boot map.

## RPG depth (genre features not yet implemented)

- [ ] Equipment set bonuses (wearing matching pieces grants extra effects)
- [x] Character classes / starting archetypes — Dark-Souls-style **starting templates** only (no lock-in):
      the fresh-character menu offers Warrior / Ranger / Mage, each just seeding starting attributes +
      a starting weapon (sword/crossbow/wand). All skill branches + weapons stay open; you level freely.
      (`CharacterClass.h` loadouts; `ApplyClassLoadout`; menu class buttons; returning saves just continue.)
- [ ] Elemental damage types + resistances/weaknesses
- [ ] Dodge / block / parry defensive options
- [ ] Biomes / themed floors (visual + enemy-set variety per depth)
- [ ] Keys & locked doors, secret rooms, destructible walls
- [ ] Crafting / enchanting / item upgrades (sinks for materials + gold)
- [ ] More consumables beyond potions (scrolls, food, buff items)

## Backlog (bigger systems, not scheduled yet)

- [ ] Enemy behavior archetypes — distinct AI beyond chase+melee: ranged shooter, charger, exploder,
      shielded/flanker (so encounters are about positioning + threat, not just trading hits).
- [ ] Status effects (poison, burn, freeze, stun, bleed) on weapons/abilities/enemies
- [ ] Loot rarity tiers + procedural affixes (rolled item stats: +damage, +crit, resistances, etc.)
  - Scaffolding done: `EDamageType`, `FItemAffix` + `AffixDatabase` (starter table incl. "of the Inferno"),
    `ComposeItemName`/`ComposeItemBonuses`, per-instance `FInventorySlot::Affixes`, elemental fields on
    `FItemBonuses`. **Still to wire:** roll affixes onto drops (in `AItemPickup::Configure` / boss loot),
    don't stack instances with differing affixes (`UInventoryComponent::AddItem`), apply
    `FlatBonusDamage`/`BonusDamageType` in combat, and show the composed name + affixes in tooltips/UI.
- [ ] Multiple dungeon floors with descending, scaling difficulty (floor 1 → N)
- [ ] FTUE for controls — first-run onboarding surfacing movement/attack/interact/abilities/inventory.
- [ ] Fishing mechanic in the town — a relaxing side activity (cast/reel minigame, catch table, maybe sell/cook the catch).
- [ ] Dungeon-select menu on the town portal — lists dungeons/bosses you've cleared; replay at the same
      or harder difficulty tiers for greater reward potential, with the risk of losing everything in the
      run (high-stakes mode). Pairs with run-stakes / meta-progression.
- [ ] Destructible props — smash barrels/crates/pots for coins/loot + a debris burst (classic crawler feel).
- [ ] Mimics — a chest variant that's secretly a monster; snaps at you when opened.
- [ ] Shrines / altars — interact for a random blessing (or a gamble: buff vs. curse). Pairs with run-stakes.
- [x] Rest / campfire at Rest rooms — `ABonfire` (graybox stone base + flickering flame/light); interact
      to **fully heal + refill mana/stamina + checkpoint-save** (Dark-Souls bonfire). Reusable. (Later:
      use it as the death-respawn point + a brief buff.)
- [~] Aggro + stealth backstab — **backstab done**: hits landing from behind a regular monster deal
      1.8× (shown as an orange crit number). Still TODO: line-of-sight/noise aggro + an unaware/stealth bonus.
- [ ] Summon / companion — a temporary graybox ally (skill or item) that fights alongside you.
- [ ] Bestiary — monster compendium that fills in as you kill each type (sibling to the collection log).
- [ ] Item durability + repair (gold sink), and/or gem socketing / runes for gear customization.
- [x] Loot beams — every dropped item emits a rarity-colored pillar + glow (taller/brighter for rarer);
      the boss drops a handful of rolled items on death. (Still TODO: rarity feedback on chest loot + auto-loot.)
- [ ] Some sort of gambling. Blackjack?

## Aesthetics / VFX (code-driven, no imported art needed)

Done (batches 1-2): impact spark bursts on hits, ambient drifting dust motes, screen damage flash,
gold level-up burst, sprint FOV kick, low-HP chromatic aberration + desaturation.

Remaining (batch 3):
- [x] Footstep dust (puff every ~175cm). Still TODO: richer ability-cast bursts (Whirlwind ring / Nova shockwave vs. the current poof).
- [ ] More ambient atmosphere — embers off torches, fog wisps, occasional ceiling drips.
- [x] Pickup sparkle (rarity-colored on collect) + chest open rarity-pop. Still TODO: boss phase-transition shockwave + screen flash.
- [x] Trap telegraph glow before spikes pop; [x] enemy death dissolve (sink+shrink). Still TODO: enemy *spawn* dissolve (boss has a spawn VFX; regular mobs don't).

### Niagara VFX upgrade (art pass — replaces the code-driven graybox effects)
The current effects spawn plain meshes + lights from C++ and animate them by hand (cheap, asset-free,
diff-friendly, but boxy). Once we're doing the art pass, author proper **Niagara systems** (editor assets)
and swap them in where these code effects fire — soft textured sprites, GPU particle counts, ribbons, etc.
Keep the C++ spawn points; just point them at Niagara systems (or gate code-vs-Niagara behind a flag).
- [ ] Niagara replacements for: impact spark bursts (`AImpactBurst`), ambient dust (`AAmbientDust`),
      death poof (`ADeathPoof`), level-up burst, loot beams (`AItemPickup`), **boss spawn-in**
      (`ABossSpawnVFX` — pairs with the dissolve shader option B above), and the batch-3 effects above.
- [ ] Add the Niagara plugin/module dependency and a small helper to spawn a system at a transform with
      a tint param, so each call site is a one-liner.

## Engine / Settings

- [ ] Resolve the "Missing Project Settings" editor warning — enable Shader Model 6 (SM6) and/or set
      up Nanite in Project Settings (editor shows a toast about Nanite assets needing SM6)

## Art / VFX

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

  models to make later:
  - sacks
  - broken pottery shards
  - hay piles
  - candelabra/floor candlesticks
  - lantern
  - chains
  - cobwebs
  - rugs/carpets
  - statues
  - gravestones
  - cage (idk what happened here it wouldnt import, blend has been started)
  - ladder
  - coin/treasure piles

## UI

- [ ] Better item icons — current render-to-texture silhouettes are recognizable but rough
  (flat shading, no fill/depth, weapons-only). Options: tune the icon studio lighting/material for
  nicer 3D thumbnails, add proper icon meshes for all items (potions, gems, etc.), or hand-authored
  2D icon textures per item.

## Audio

- [ ] Ambient music (dungeon background track; later boss-fight / town variants)

## Monster ideas
- eel
- manta ray / sting ray
- ant eater boss
- goat
- bear
- dog
- monkey
- snake
- squirrel

---

# DONE

## Gameplay / combat
- [x] **Dash/dodge** replaces sprint on Shift — short committed stamina-costed burst (Dark-Souls feel).
- [x] **Boss combat pass** (Dark-Souls direction): attack lands on anim **frame 20** (dodgeable by
      repositioning), melee reach measured from the boss's body edge (so the big crab can actually hit),
      hits for a brutal flat **135** (~75% of starting HP), closes fast (MoveSpeed 430), and runs in
      anim-test mode (`bAbilitiesEnabled=false`) with no specials while anims are finalized.
- [x] **Boss telegraph + dodge feel** — a red floor disc (`AAttackTelegraph`) marks the exact danger
      radius and flashes on impact; the zone is wide enough that you must DASH out (walk-back won't clear
      it); attack cooldown 2.8s (no spam); player dash ~400cm; player melee lands ~45% through the swing.
- [x] **Boss bug fixes** — entrance doors actually seal now (AlwaysSpawn); loot drops at floor level (not
      the ceiling); intro camera+shake plays every encounter (not once-per-save) and frames the boss center.
- [x] **Feedback polish** — hit sparks are a dark blood spurt (not a bright pop), damage numbers
      bigger/brighter, skill tree panel enlarged + padded, banners lie flat on the wall.
- [x] **Boss loot + spawn VFX** — drops 3-5 rolled items at the kill site (rarity loot beams); a code-
      driven spawn-in effect (`ABossSpawnVFX`: ground flare + energy pillar + shard swirl + debris); the
      return portal activates at the room corner farthest from the death spot.
- [x] **Quick-win feel batch** — insufficient-stamina/mana bar flash, enemy death dissolve (sink+shrink),
      footstep dust, item pickup sparkle + chest open rarity-pop, trap telegraph warning glow.
- [x] Boss encounter sequence — spawn-on-entry opposite the player; entrance doors seal (open on death);
      first-ever kill plays a camera-focus + screen-shake + intro cinematic (persisted per boss id).
- [x] Boss fight depth — scuttle + telegraphed lunge, hybrid navmesh, projectile volley (3-spread p2+),
      phase-1 back weak point, phase-2/3 bubble pools, shell-retreat (p3), 3 phases + specials.
- [x] Boss health bar — dedicated top-center red bar, labelled from the boss id, removed on death.
- [x] Boss switched to the hermit-crab skeletal mesh (idle/walk hooked; attack/spawn anims pending) with
      a static/graybox fallback; boss room is a normal (big) room; floor-snap so it spawns on the ground;
      no traps in the boss arena; intro camera straighter/lower. Projectile speed tuned; adds get a real type.
- [x] Scenery feels deliberate — per-room décor themes, furniture lined against walls (coffins flat),
      corner stacks, dining sets; scenery avoids traps and never overlaps other scenery.
- [x] Dev menu also has Teleport to Boss; arena cleans up its boss/doors/portal on regenerate.
- [x] Floating damage numbers — world-space 3D text; weak-point hits bigger/orange; big & bright.
- [x] Improve enemy pathing (runtime navmesh + MoveTo).
- [x] Dungeon traps — spike floors, pressure plates, dart shooters (ADungeonTrap).
- [x] Room-type variety — Treasure / Ambush / Rest / Elite / Normal, each with a colored marker light.
- [x] Weapon usage costs stamina (melee now spends stamina, gated when empty).
- [x] Game juice — hit-stop, camera kick, enemy knockback, low-health vignette.
- [x] VFX pass (batches 1-2) — impact spark bursts, ambient dust motes, screen damage flash, gold
      level-up burst, sprint FOV kick, low-HP chromatic aberration + desaturation. All pooled/cheap.
- [x] Dev menu (Esc → Dev Menu) — No Clip, God Mode, Reveal Map, Teleport to Boss, Kill Player, Teleport Home.
- [x] Potions pick up on E press (interact-to-collect).

## UI / UX
- [x] Inventory shows gold; shop and inventory no longer overlap (mutually exclusive).
- [x] Skill tree decluttered — one clean line per node, full description in a hover tooltip.
- [x] FPS counter (top-right, color-coded); pause menu widened + auto-sizes (no squish).

## Flow / UX
- [x] Start screen — graybox main menu (Start/Quit) over the boot map, once per session.
- [x] Scene transitions — black camera fades on every level travel + fade-in on arrival.
- [x] Controls in settings — full scrollable controls reference in the pause→Settings panel.

## Generation / art
- [x] Bigger rooms + shorter (nearest-neighbor) hallways + denser props/chests + wall props no longer clip.
- [x] Dungeon fog (atmospheric volumetric/height fog).
- [x] In generation: barrel, pots, bucket, rocks, bones, anvil, coffin, mushrooms as clutter;
      weapon racks against walls; banners flanking doorways.
