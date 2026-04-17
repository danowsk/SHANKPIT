# Changelog

## Recent changes

### 2026-04-16
- Added authoritative **CTFB** mode with shared flag state replication and bot objective handling for online capture-the-flag battles (`a7785a5`, merged in #182).
- Tuned first-person weapon viewmodel art direction and applied an additional global scale reduction pass for cleaner weapon framing (`41069c5`, `cfbcc8a`, merged in #176).
- Added and refined a new selectable **pink** skin into a samurai space-marine inspired presentation (`387a37f`, `549e301`, merged in #173).
- Added a standalone **Poo Poo Island** scene with a dedicated garage portal to broaden map rotation variety (`5f044fe`, merged in #178).
- Scaled down weapon model sizing and proportions by roughly ten percent for better visual balance (`f30cfb7`, merged in #180).
- Added random **TDMB** map selection with scene-specific team spawn support to improve match variety (`9e29d9f`, merged in #172).

### 2026-04-15
- Renamed the Battle Bots menu label to **TRAIN** for clearer mode intent (`bb37503`, merged in #170).
- Added **TDMO** online team deathmatch mode with server-side bot fill and team snapshot handling to expand online match options (`760798a`, merged in #169).
- Added bottom-right HUD ammo bars in the lobby/game HUD pass for clearer at-a-glance ammo tracking (`dba280d`, merged in #168).
- Polished the lobby menu into a two-column, launch-ready layout for improved readability and navigation (`5e38d39`, merged in #167).
- Converted TAB scoring for TDM/TDMB into a truly team-based scoreboard presentation (`ed071a4`, merged in #164).
- Added hard-coded **TDMB** mode flow and server-authoritative team rules while removing the earlier evolution-mode path (`deb1d91`, merged in #163).
- Fixed skin chooser behavior by pinning the BACK row for correct scrolling and corrected a malformed lobby click block that caused build failures (`60cb38b`, `ca83a38`, merged in #161).
- Polished TAB scoreboard readability with improved row layout and slightly increased row spacing for easier scanning during matches (`76f1945`, `33e783a`, merged in #159).

### 2026-04-14
- Expanded lobby customization with two new selectable character skins, **Genie** and **Wanderer** (`64cff5a`, merged in #158).
- Renamed the previous POC skin option to **BILL** for clearer in-game skin naming (`c0d2098`, merged in #156).

### 2026-04-13
- Added **pirate** and **ninja** character skins to the lobby skin lineup (`06300d0`, merged in #154).

### 2026-04-12
- Added the **CYBORG** player skin and integrated it into the skin selector (`ab38202`, merged in #135).
- Added a deterministic Voxworld bush foliage pass to improve scene dressing consistency (`127bdde`, merged in #136).

### 2026-04-12
- Added Voxworld helicopter gameplay support, including scene-aware network replication and rooftop spawn pads with stair access for better traversal and vehicle flow (`d26454f`, `a9f5a30`, merged in #132).
- Added a VS0 skin selector with Bat/Mayrice rendering options to expand in-game visual customization (`072e4ce`, #130).
- Added a new oil tanker scene with a garage portal, deathmatch rotation updates, and a tab scoreboard to improve map variety and match usability (`396b1cd`, #128).

### 2026-04-11
- Added a VS0 art-direction toggle and calmer world render pass for alternate visual presentation (`2daba9a`, #126).
- Added a dust-compound infantry map with a dedicated garage portal (`ba1915d`, #124).
- Reworked Voxworld into a large canyon vehicle battleground, then expanded it with a first-pass heightfield terrain system and direct garage portal (`789fc58`, `112fab7`, `0728f8e`, #121, #119).
- Fixed CI terrain-module compilation linkage while integrating terrain changes (`c650c74`).

### 2026-04-06
- Added a first-pass authoritative helicopter vehicle implementation (`3dcae0a`, #117).

### 2026-03-31 to 2026-03-30
- Added a katana weapon with blade-dash ability and fixed katana-model draw-box forward declaration issues (`0136375`, `110acb2`, #114).

### 2026-03-23
- Fixed Windows lobby build cross-link failures for `proc_tex` (`a47756b`, #112).

### 2026-03-01
- Refreshed buggy fixed-pipeline vehicle styling with procedural visual accents (`3e5ee0e`, #111).

### 2026-02-21 to 2026-02-20
- Added CI workflow support for Dragonfly Construct builds (`2c1d0f9`, #110).
- Improved multiplayer correctness and feel across multiple iterations: spawn synchronization, jitter diagnostics, parity harness hardening, unified simulation/movement paths, camera-relative movement, and stale-player cleanup (`2fddae0`, `4ff7fca`, `471cec3`, `4dd03e4`, `1376350`, `75b3429`, plus merged PRs #109, #107, #106, #105, #104, #103).
- Added procedural title-screen background texture work (`bf852d2`, #108).
- Added a definitive client-server netcode contract spec and follow-up yaw/input convention fixes (`852caf5`, `9d1af28`, #101, #102, #100).
