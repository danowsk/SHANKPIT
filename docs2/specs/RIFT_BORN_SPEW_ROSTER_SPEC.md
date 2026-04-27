# SHANKPIT Spec: Rift-Born / The Spew (Portal Breach Enemy Set)

## Goal
Define a low-poly horror enemy faction for the post-boss portal breach sequence that is:

- Readable at distance.
- Implementable in staged milestones.
- Visually horrifying without requiring high-detail assets.

## Core Visual Language

### Material + color
- Matte-dark outer bodies.
- Glowing inner cracks using red, magenta, and sickly orange accents.

### Shape language
- Distorted humanoid + animal silhouettes.
- Silhouette-first readability (clear profile before texture detail).

### Face rules (pick one per archetype)
- No face at all.
- Too many glowing eyes.
- Vertical mouth slit that opens unnaturally wide.

### Animation "wrongness"
Apply subtle but consistent corruption cues:
- Micro-jitter while idling.
- Sudden head snaps.
- Slightly overextended limbs during attack frames.
- Mild hover/drag/twitch offsets in locomotion.

## Recommended V1 Roster (5 Units)
This is the suggested first-pass set for SHANKPIT.

## 1) Rift Hound
**Role:** Fast melee swarmer  
**Readability:** Immediate panic unit (“oh no” silhouette)

**Look**
- Low quadruped.
- Too-long forelegs.
- Spiked/sharp back line.
- Glowing jaw and/or chest cavity.
- Optional: no visible eyes, single light slit.

**Behavior**
- Spawns in packs.
- Zig-zag run toward player.
- Short leap attack.
- Low HP.

**Implementation notes**
- Simplest AI profile (rush + leap).
- Simple collision capsule.
- Strong portal-spew visual when spawned in bursts.

## 2) Shambler Trooper
**Role:** Mid-tier infantry filler  
**Readability:** Corrupted former soldier

**Look**
- Humanoid with one oversized/elongated arm.
- Helmet-like fused skull.
- Glowing torso cracks/rib seams.
- Broken-march locomotion.

**Behavior**
- Slow walk at range.
- Bursts into sprint near player.
- Melee slash or crude ranged throw.
- Spawns behind/after hounds.

**Lore flavor**
- Former humans, failed constructs, or dead soldiers altered by rupture-space.

## 3) Shrieker
**Role:** Ranged pressure/support  
**Readability:** Floating battlefield nuisance

**Look**
- Hovering upper torso.
- No legs; trailing smoke/torn mass.
- Oversized glowing mouth.
- Head tilt at unnatural angles.

**Behavior**
- Holds battlefield edges.
- Uses scream projectile / cone pulse / slow debuff.
- Stays behind frontline units.
- Fragile HP budget.

**Implementation notes**
- Audio + widened-mouth animation carries horror impact.
- Can be built with simple cone telegraph + projectile.

## 4) Gore Brute
**Role:** Tank / line breaker  
**Readability:** Escalation unit

**Look**
- Massive hunched torso.
- Asymmetrical shoulders.
- One oversized smash arm.
- Large glowing torso cracks.
- Optional fused portal-rock growth on back.

**Behavior**
- Slow advance.
- Shoulder charge or ground slam.
- Knockback/space control.
- High HP.

**Wave timing**
- Arrives shortly after first swarmer wave to signal escalation.

## 5) Portal Shepherd
**Role:** Support caster / wave manager  
**Readability:** Priority target controlling invasion flow

**Look**
- Tall, thin silhouette.
- Floating robe-like geometry.
- Head ring/halo/horns.
- Raised arm/staff gesture toward rupture.

**Behavior**
- Buffs nearby Rift-Born.
- Periodically summons adds.
- Can briefly shield the rupture/portal.

**Narrative function**
- Communicates that the invasion has intelligence and intent.

## Optional V2 Variant: Splitter
**Role:** Priority-disruption suicide unit

**Look**
- Swollen sac body.
- Unstable glowing core.
- Dragging limbs.

**Behavior**
- Slow approach.
- Explodes on death or proximity.
- Spawns 2–4 mites/larvae.

## Thematic Lanes
Pick one lane as the primary style guide.

### A) Hell Breach
Classic demonic horror readability.

### B) Broken Construct / Failed Experiment
Synthetic-biological corruption (stitched cyber-flesh, mannequin wrongness).

### C) Telecrystal Corruption (**recommended**)
Ties directly into SHANKPIT portal/teleport identity.
- Crystal growth through flesh.
- Phase flicker/blink behaviors.
- Jagged luminous seams.
- Telecrystal scream effects.

## Lore Framing (Portal Breach Event)
After a boss kill, local reality anchor stability collapses.

- A rupture opens in sky/terrain/arena wall.
- The rupture is treated as a wound, not a clean gate.
- Rift-Born entities emerge from the overlap of teleport-space, failed respawn-state, and corrupted matter.

## Staged Implementation Plan

### Stage 1 (MVP)
- Rift Hound + Shambler Trooper + Gore Brute.
- One shared emissive crack shader/material variant.
- One portal-spew spawn effect.

### Stage 2
- Add Shrieker (ranged pressure).
- Add minor animation corruption layer (jitter/head-snap blend events).

### Stage 3
- Add Portal Shepherd (buff/summon/portal shield window).
- Add breach encounter scripting for post-boss sequence pacing.

### Stage 4 (Optional)
- Add Splitter and larvae chain-spawn behavior.
- Add stronger telecrystal phase VFX.

## Readability Rules for Gameplay
- Every unit must be identifiable by silhouette at a distance.
- Behavior role must be obvious within first 2–3 seconds of contact.
- Emissive regions should indicate danger/attack origin (jaw, chest, arm, mouth).

## Audio Priorities
If only minimal audio budget is available:
1. Shrieker scream telegraph.
2. Gore Brute charge wind-up.
3. Rift Hound pack emergence cue.

These three cues carry most of the encounter readability and horror tone.
