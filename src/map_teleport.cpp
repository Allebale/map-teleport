// Map Teleport: a map of Isle Delfino, opened in a stage (M, or a controller button), with every
// level the save file has reached on it; choose one (and its episode) and Mario is taken there, as
// a warp pipe would.
//
// Which levels: the ones this save file has been to since the mod was on (kept with the save file,
// files->* scope SMS_FILES_CURRENT, so they are saved with it), and in Super Mario Sunshine also
// every world the file has a Shine of. A world's episodes are the ones its own episode select
// offers (the Shines had, and the next). Super Mario Eclipse numbers its stages its own way (no
// api->world there), so its levels are the ones visited, by the episodes they were visited in.
//
// The map is the mod's own: Isle Delfino drawn from circles, each level's place on it found by its
// name (game->area_name); a level whose name is not known goes in a list at the map's side.
#include <sms_mod.hpp>

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace {

// Where each level is on the map (the place of its name, a part of it in lower case), in the map's
// own picture: 1460 x 1095, from its top left
struct Spot {
	const char* key;
	float x, y;
};
const Spot kSpots[] = {
    {"airstrip", 1165, 530},    {"plaza", 1015, 632},      {"bianco", 848, 497},     {"ricco", 795, 610},
    {"gelato", 678, 483},       {"pinna", 503, 783},       {"sirena", 468, 560},     {"pianta", 780, 362},
    {"noki", 532, 362},         {"corona", 947, 442},      {"lacrima", 710, 289},    {"rugivosa", 797, 271},
    {"warship", 882, 298},      {"arido", 995, 353},       {"erto", 1044, 463},      {"bollente", 912, 537},
    {"fire shrine", 985, 553},  {"giovane", 740, 522},     {"sonnolento", 642, 347}, {"arcobaleno", 454, 443},
    {"marea", 638, 655},        {"lighthouse", 657, 747},  {"daisy", 925, 872},      {"peach", 1250, 640},
};
const float kPicW = 1460.0f, kPicH = 1095.0f;

// The island, as circles (x, y, radius in the picture's units): the dolphin's body, its fin, its tail
// and the small islands
const float kLand[][3] = {
    // the back, tail to head
    {425, 705, 30},  {420, 640, 40},  {440, 560, 50},  {480, 480, 58},  {545, 410, 64},  {625, 360, 68},
    {715, 330, 70},  {800, 320, 70},  {880, 335, 66},  {955, 370, 62},  {1010, 420, 58}, {1045, 480, 54},
    {1055, 545, 48}, {1040, 605, 40}, {1015, 640, 30}, {950, 445, 60},
    // the fin
    {745, 260, 40},  {720, 215, 30},  {705, 180, 18},
    // the belly
    {520, 560, 56},  {600, 520, 62},  {690, 500, 66},  {780, 520, 68},  {870, 520, 66},  {950, 520, 58},
    {600, 610, 48},  {700, 600, 56},  {790, 600, 60},  {880, 600, 56},  {960, 610, 48},  {520, 640, 40},
    {750, 660, 40},  {830, 655, 40},  {990, 640, 36},
    // the tail's fluke and Pinna Park
    {455, 790, 40},  {505, 805, 46},  {555, 790, 34},  {470, 845, 28},
    // Lighthouse Island, Marea Reef, the Airstrip
    {657, 747, 22},  {638, 655, 14},  {1165, 530, 30}, {1200, 560, 18},
    // the cruiser's harbour and Peach's Castle
    {900, 870, 16},  {925, 872, 18},  {950, 875, 16},  {1250, 640, 22},
};

// The game's choice of episode as it would (the plaza, Hotel Delfino, Pinna Park's park, Casino
// Delfino: their episode follows the story)
const int kStoryEpisode = 255;

// Super Mario Sunshine's own tables (the decomp's System/StageUtil.cpp and GC2D/ShineTable.hpp), kept
// here: the game's api->world answers only when no code mod is on, and this works with one on too.
// An area's world (0 the Airstrip, 1 the plaza, 2 to 8 the seven worlds, 9 Corona Mountain).
const unsigned char kShineStage[61] = {
    0, 1, 2, 3, 4, 5, 6, 6, 7, 8, 9, 1, 1, 5, 6, 1, 8, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 3,
    8, 4, 4, 5, 5, 5, 5, 5, 5, 6, 5, 7, 7, 8, 8, 2, 2, 3, 3, 5, 6, 9, 1, 1, 2, 6, 8, 5, 3, 9,
};
const int kWorldArea[10] = {0, 1, 2, 3, 4, 5, 6, 8, 9, 52}; // the area each world is entered by
// each world's eight episodes' Shines (worlds 2 to 8)
const unsigned char kEpisodeShines[7][8] = {
    {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07}, // Bianco Hills
    {0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11}, // Ricco Harbor
    {0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B}, // Gelato Beach
    {0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25}, // Pinna Park
    {0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F}, // Sirena Beach
    {0x3C, 0x41, 0x3E, 0x3D, 0x40, 0x3F, 0x42, 0x43}, // Pianta Village
    {0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39}, // Noki Bay
};
int shineStage(int area) { return area >= 0 && area < 61 ? kShineStage[area] : -1; }
// SMS_AREA_*, as api->world's area_kind
int areaKind(int area)
{
	const int w = shineStage(area);
	if (w < 0)
		return SMS_AREA_NONE;
	if (area == 15)
		return SMS_AREA_TITLE;
	if (w == 0)
		return SMS_AREA_AIRSTRIP;
	if (w == 9)
		return SMS_AREA_FINAL;
	if (area > 0x14 && area < 0x35)
		return SMS_AREA_SECRET;
	if (w == 1)
		return SMS_AREA_PLAZA;
	const bool entry = area == 2 || area == 3 || area == 4 || area == 5 || area == 6 || area == 8 || area == 9;
	return entry ? SMS_AREA_WORLD : SMS_AREA_WORLD_PART;
}
int worldOf(int area)
{
	const int w = shineStage(area);
	return w >= 0 && w <= 9 && area != 15 ? kWorldArea[w] : -1;
}
// a world's episode Shines (by the area it is entered by), or null
const unsigned char* episodeShines(int area)
{
	const int w = shineStage(area);
	return w >= 2 && w <= 8 && area != 15 ? kEpisodeShines[w - 2] : nullptr;
}

std::string lower(const std::string& s)
{
	std::string o = s;
	for (size_t i = 0; i < o.size(); i++)
		o[i] = (char)tolower((unsigned char)o[i]);
	return o;
}

struct Entry {
	std::string label;
	int area, episode;
};

struct Place {
	std::string name;
	std::vector<int> areas; // the main one first
	int world = -1;         // Super Mario Sunshine's world (api->world), or -1
	float x = 0, y = 0;     // on the map, 0..1
	bool onMap = false;     // its name is known (else in the side list)
	bool open  = false;     // reached
	std::vector<Entry> entries;
};

} // namespace

struct MapTeleport : sms::Mod {
	using sms::Mod::Mod;

	int openKey = -1;
	int kUp = -1, kDown = -1, kLeft = -1, kRight = -1, kEnter = -1, kSpace = -1, kEscape = -1, kBack = -1;

	bool shown   = false; // the map is up
	bool leaving = false; // a teleport asked: the controls kept until the stage changes
	double leftAt = 0;
	bool inList  = false; // choosing a level's episode
	int sel = 0, item = 0;
	int lastDir = 0;
	double nextRepeat = 0;
	uint16_t prevHeld = 0;
	std::vector<Place> places;
	std::set<std::pair<int, int> > visited; // (area, episode) this save file has been to
	int fileLoaded = -2;

	// ---------------------------------------------------------- the game

	const SmsWorldApi* world() const
	{
		const SmsApi* a = &api();
		return SMS_API_HAS(a, world) ? a->world : nullptr;
	}
	const SmsFilesApi* files() const
	{
		const SmsApi* a = &api();
		return SMS_API_HAS(a, files) ? a->files : nullptr;
	}
	// Super Mario Sunshine's map of its worlds (none in Super Mario Eclipse)
	bool sunshineWorlds() const
	{
		return !state().eclipse;
	}
	std::string areaName(int area) const
	{
		const char* n = game().area_name ? game().area_name(area) : "";
		return n ? n : "";
	}

	// ---------------------------------------------------------- the save file's levels

	void loadVisited(int file)
	{
		visited.clear();
		fileLoaded = file;
		const SmsFilesApi* f = files();
		if (!f || file < 0)
			return;
		static char buf[64 * 1024];
		int n = f->read(handle(), file, "visited.txt", buf, sizeof buf - 1);
		if (n <= 0)
			return;
		if (n > (int)sizeof buf - 1)
			n = sizeof buf - 1;
		buf[n] = 0;
		const char* p = buf;
		int a, e, used;
		while (sscanf(p, "%d %d%n", &a, &e, &used) == 2) {
			if (a >= 0 && a < 255 && e >= 0 && e < 256)
				visited.insert(std::make_pair(a, e));
			p += used;
		}
	}
	void saveVisited()
	{
		const SmsFilesApi* f = files();
		if (!f)
			return;
		std::string s;
		char line[32];
		for (std::set<std::pair<int, int> >::const_iterator i = visited.begin(); i != visited.end(); ++i) {
			snprintf(line, sizeof line, "%d %d\n", i->first, i->second);
			s += line;
		}
		f->write(handle(), SMS_FILES_CURRENT, "visited.txt", s.data(), (uint32_t)s.size());
	}
	bool areaVisited(int area) const
	{
		std::set<std::pair<int, int> >::const_iterator i = visited.lower_bound(std::make_pair(area, 0));
		return i != visited.end() && i->first == area;
	}

	// ---------------------------------------------------------- the places on the map

	void placeOnMap(Place& p) const
	{
		const std::string n = lower(p.name);
		for (size_t i = 0; i < sizeof kSpots / sizeof *kSpots; i++)
			if (n.find(kSpots[i].key) != std::string::npos) {
				p.x = kSpots[i].x / kPicW, p.y = kSpots[i].y / kPicH;
				p.onMap = true;
				return;
			}
	}

	std::string episodeName(int area, int episode) const
	{
		const SmsWorldApi* w = world();
		char name[96] = "";
		if (w && SMS_API_HAS(w, episode_name) && w->episode_name(area, episode, name, sizeof name) > 0) {
			char out[128];
			snprintf(out, sizeof out, "%d  %s", episode + 1, name);
			return out;
		}
		char out[32];
		snprintf(out, sizeof out, "Episode %d", episode + 1);
		return out;
	}

	// a Sunshine area that is not a world's own episodes: its name in the list
	std::string partName(int area, int kind) const
	{
		switch (area) {
		case 7: return "Hotel Delfino";
		case 13: return "The park";
		case 58: return "Casino Delfino";
		}
		if (kind == SMS_AREA_SECRET)
			return "Secret course";
		if (kind == SMS_AREA_PLAZA)
			return areaName(area);
		return "Another part";
	}

	void buildSunshine(std::map<std::string, Place>& by)
	{
		const SmsGameApi& g  = game();
		const int shines     = g.shines ? g.shines() : 0;
		for (int a = 0; a < 255; a++) {
			const int kind = areaKind(a);
			if (kind == SMS_AREA_NONE || kind == SMS_AREA_TITLE || areaName(a).empty())
				continue;
			const int wo = worldOf(a);
			const int id = wo >= 0 ? wo : a;
			char key[16];
			snprintf(key, sizeof key, "%03d", id);
			Place& p = by[key];
			if (p.areas.empty()) {
				p.world = id;
				p.name  = areaName(id).empty() ? areaName(a) : areaName(id);
			}
			p.areas.push_back(a);
		}
		for (std::map<std::string, Place>::iterator it = by.begin(); it != by.end(); ++it) {
			Place& p = it->second;
			std::sort(p.areas.begin(), p.areas.end());
			const int wid = p.world;
			// reached: been there, or a Shine of its episodes had (the plaza and the Airstrip: any Shine)
			for (size_t i = 0; i < p.areas.size(); i++)
				p.open = p.open || areaVisited(p.areas[i]);
			const unsigned char* shines8 = episodeShines(wid);
			const int episodes = shines8 ? 8 : 0;
			for (int e = 0; e < episodes && !p.open; e++)
				p.open = g.has_shine && g.has_shine(shines8[e]);
			if (wid == 1 && shines > 0)
				p.open = true;
			if (!p.open)
				continue;
			// a world's episodes, as its episode select offers them
			if (episodes > 0) {
				// the one after the newest whose Shine is had (TSelectMenu::initData)
				int next = 0;
				for (int e = 0; e < 8; e++)
					if (g.has_shine && g.has_shine(shines8[e]))
						next = e + 1;
				next = next > 7 ? 7 : next;
				for (int e = 0; e <= next; e++) {
					Entry en = {episodeName(wid, e), wid, e};
					p.entries.push_back(en);
				}
			}
			for (size_t i = 0; i < p.areas.size(); i++) {
				const int a    = p.areas[i];
				const int kind = areaKind(a);
				if (a == wid && episodes > 0)
					continue; // its episodes, above
				if (a == 1 || a == 7 || a == 13 || a == 58) { // the game picks its episode
					if (a == 1 || areaVisited(a)) {
						Entry en = {a == 1 ? p.name : partName(a, kind), a, kStoryEpisode};
						p.entries.push_back(en);
					}
					continue;
				}
				for (std::set<std::pair<int, int> >::const_iterator v = visited.lower_bound(std::make_pair(a, 0));
				     v != visited.end() && v->first == a; ++v) {
					std::string label = a == wid ? p.name : partName(a, kind);
					char ep[24];
					snprintf(ep, sizeof ep, " (episode %d)", v->second + 1);
					Entry en = {label + ep, a, v->second};
					p.entries.push_back(en);
				}
			}
			p.open = !p.entries.empty();
		}
	}

	// Super Mario Eclipse (or a build without api->world): the levels by name, as visited
	void buildByName(std::map<std::string, Place>& by)
	{
		for (int a = 0; a < 255; a++) {
			const std::string n = areaName(a);
			if (n.empty())
				continue;
			Place& p = by[lower(n)];
			if (p.areas.empty())
				p.name = n;
			p.areas.push_back(a);
		}
		for (std::map<std::string, Place>::iterator it = by.begin(); it != by.end(); ++it) {
			Place& p = it->second;
			int part = 0;
			for (size_t i = 0; i < p.areas.size(); i++) {
				const int a = p.areas[i];
				if (!areaVisited(a))
					continue;
				part++;
				for (std::set<std::pair<int, int> >::const_iterator v = visited.lower_bound(std::make_pair(a, 0));
				     v != visited.end() && v->first == a; ++v) {
					std::string label = episodeName(a, v->second);
					if (p.areas.size() > 1) {
						char pre[24];
						snprintf(pre, sizeof pre, "Part %d, ", (int)i + 1);
						label = pre + label;
					}
					Entry en = {label, a, v->second};
					p.entries.push_back(en);
				}
			}
			p.open = part > 0;
		}
	}

	void build()
	{
		std::map<std::string, Place> by;
		if (sunshineWorlds())
			buildSunshine(by);
		else
			buildByName(by);
		places.clear();
		int side = 0;
		for (std::map<std::string, Place>::iterator it = by.begin(); it != by.end(); ++it) {
			Place p = it->second;
			placeOnMap(p);
			if (!p.onMap) {
				if (!p.open)
					continue; // an unknown name not reached: left out
				p.x = 0.05f, p.y = 0.12f + 0.07f * (float)side++;
			}
			places.push_back(p);
		}
		int reached = 0;
		for (size_t i = 0; i < places.size(); i++)
			reached += places[i].open ? 1 : 0;
		info("map: %s, %d places, %d reached, %d Shines", sunshineWorlds() ? "Super Mario Sunshine" : "Super Mario Eclipse",
		     (int)places.size(), reached, game().shines ? game().shines() : 0);
		// start on where Mario is
		sel = 0;
		const int here = state().area;
		for (size_t i = 0; i < places.size(); i++)
			for (size_t k = 0; k < places[i].areas.size(); k++)
				if (places[i].areas[k] == here)
					sel = (int)i;
		if (places.empty())
			sel = -1;
	}

	// ---------------------------------------------------------- opening and closing

	bool canOpen(const SmsGameState& st) const
	{
		if (st.mode != SMS_MODE_STAGE || !st.stage_ready || st.paused || st.save_file < 0)
			return false;
		const SmsWorldApi* w = world();
		if (w && SMS_API_HAS(w, cutscene_playing) && w->cutscene_playing())
			return false;
		if (sunshineWorlds() && areaKind(st.area) == SMS_AREA_TITLE)
			return false;
		const char* by = input().captured_by ? input().captured_by() : nullptr;
		return !by;
	}

	void sound(uint32_t id) const
	{
		if (api().audio && api().audio->play_system_sound)
			audio().play_system_sound(id);
	}

	void openMap()
	{
		build();
		if (!input().capture(handle(), 1))
			return;
		shown   = true;
		inList  = false;
		item    = 0;
		lastDir = 0;
		prevHeld = 0xFFFF; // the button that opened it: let go first
		sound(SMS_SOUND_DECIDE);
	}
	void closeMap(bool quiet = false)
	{
		if (shown && !leaving)
			input().capture(handle(), 0);
		shown = false;
		if (!quiet)
			sound(SMS_SOUND_CANCEL);
	}
	void teleport(const Entry& e)
	{
		if (!game().warp || !game().warp(e.area, e.episode)) {
			sound(SMS_SOUND_DAMAGE);
			return;
		}
		info("teleport to area %d, episode %d (%s)", e.area, e.episode, e.label.c_str());
		sound(SMS_SOUND_DECIDE);
		shown   = false;
		leaving = true; // the controls stay ours until the stage changes (no jump from the A held)
		leftAt  = core().time();
	}
	void endLeaving()
	{
		if (leaving)
			input().capture(handle(), 0);
		leaving = false;
	}

	// ---------------------------------------------------------- moving about

	// the place nearest the way pushed (dx, dy: the screen's), from the one chosen
	void move(float dx, float dy)
	{
		if (sel < 0 || places.empty())
			return;
		const Place& from = places[sel];
		int best = -1;
		float bestScore = 1e9f;
		for (size_t i = 0; i < places.size(); i++) {
			if ((int)i == sel)
				continue;
			const float vx = places[i].x - from.x, vy = (places[i].y - from.y) * 0.75f;
			const float d  = sqrtf(vx * vx + vy * vy);
			if (d < 1e-4f)
				continue;
			const float c = (vx * dx + vy * dy) / d;
			if (c < 0.35f)
				continue;
			const float score = d * (1.0f + 2.5f * (1.0f - c));
			if (score < bestScore)
				bestScore = score, best = (int)i;
		}
		if (best >= 0) {
			sel = best;
			sound(SMS_SOUND_CURSOR);
		}
	}

	bool key(int k) const { return k > 0 && input().key_pressed(k); }

	void controls()
	{
		SmsPad p = pad(0);
		const uint16_t held = p.connected ? p.held : 0;
		const uint16_t pressed = held & ~prevHeld;
		prevHeld = held;

		// a direction: the stick, the D-pad or the arrow keys, again while held
		int dir = 0;
		if (p.stick_y > 0.6f || (held & SMS_PAD_UP) || (kUp > 0 && input().key_down(kUp)))
			dir = 1;
		else if (p.stick_y < -0.6f || (held & SMS_PAD_DOWN) || (kDown > 0 && input().key_down(kDown)))
			dir = 2;
		else if (p.stick_x < -0.6f || (held & SMS_PAD_LEFT) || (kLeft > 0 && input().key_down(kLeft)))
			dir = 3;
		else if (p.stick_x > 0.6f || (held & SMS_PAD_RIGHT) || (kRight > 0 && input().key_down(kRight)))
			dir = 4;
		const double now = core().time();
		bool step = false;
		if (dir && dir != lastDir)
			step = true, nextRepeat = now + 0.35;
		else if (dir && now >= nextRepeat)
			step = true, nextRepeat = now + 0.12;
		lastDir = dir;

		const bool choose = (pressed & SMS_PAD_A) || key(kEnter) || key(kSpace);
		const bool back   = (pressed & (SMS_PAD_B | SMS_PAD_START)) || key(kEscape) || key(kBack) || key(openKey);

		if (inList) {
			const Place& pl = places[sel];
			const int n = (int)pl.entries.size();
			if (step && n > 0 && (dir == 1 || dir == 2)) {
				item = (item + (dir == 1 ? n - 1 : 1)) % n;
				sound(SMS_SOUND_CURSOR);
			}
			if (back) {
				inList = false;
				sound(SMS_SOUND_CANCEL);
			} else if (choose && n > 0)
				teleport(pl.entries[item]);
			return;
		}
		if (step) {
			const float dx = dir == 3 ? -1.0f : dir == 4 ? 1.0f : 0.0f, dy = dir == 1 ? -1.0f : dir == 2 ? 1.0f : 0.0f;
			move(dx, dy);
		}
		if (back)
			closeMap();
		else if (choose && sel >= 0) {
			const Place& pl = places[sel];
			if (!pl.open || pl.entries.empty()) {
				sound(SMS_SOUND_DAMAGE);
			} else if (pl.entries.size() == 1)
				teleport(pl.entries[0]);
			else {
				inList = true;
				item   = 0;
				// start on the episode played last, or the newest
				for (size_t i = 0; i < pl.entries.size(); i++)
					if (pl.entries[i].area == state().area && pl.entries[i].episode == state().episode)
						item = (int)i;
				sound(SMS_SOUND_DECIDE);
			}
		}
	}

	bool openPressed() const
	{
		if (openKey > 0 && input().key_pressed(openKey))
			return true;
		const int b = setting("pad_button", 1);
		if (b == 0)
			return false;
		SmsPad p = pad(0);
		if (!p.connected || !(p.pressed & SMS_PAD_UP))
			return false;
		return b == 1 || (p.held & SMS_PAD_Z);
	}

	// ---------------------------------------------------------- drawing

	void drawMap(float cw, float ch)
	{
		const SmsDrawApi& d = draw();
		const double now    = core().time();
		d.rect(0, 0, cw, ch, SMS_RGBA(0, 0, 0, 150));

		// the map's frame: as big as fits with a panel at its side
		float mapW = cw - 300.0f;
		if (mapW > 620.0f)
			mapW = 620.0f;
		if (mapW < 320.0f)
			mapW = 320.0f;
		float mapH = mapW * (kPicH / kPicW);
		if (mapH > ch - 70.0f) {
			mapH = ch - 70.0f;
			mapW = mapH * (kPicW / kPicH);
		}
		const float panelW = 240.0f;
		const float total  = mapW + 16.0f + panelW;
		const float mx = (cw - total) * 0.5f, my = (ch - mapH) * 0.5f + 10.0f;
		const float px = mx + mapW + 16.0f;
		const float s  = mapW / kPicW; // the picture's units to the canvas

		// the sea
		d.rect_gradient(mx, my, mapW, mapH, SMS_RGB(70, 150, 205), SMS_RGB(40, 105, 170));
		for (int i = 1; i < 8; i++) { // a faint grid, as on a chart
			d.line(mx + mapW * i / 8.0f, my, mx + mapW * i / 8.0f, my + mapH, 1.0f, SMS_RGBA(255, 255, 255, 25));
			d.line(mx, my + mapH * i / 8.0f, mx + mapW, my + mapH * i / 8.0f, 1.0f, SMS_RGBA(255, 255, 255, 25));
		}
		// the island: its shore, its sand and its green
		const int nLand = sizeof kLand / sizeof *kLand;
		for (int i = 0; i < nLand; i++)
			d.circle(mx + kLand[i][0] * s, my + kLand[i][1] * s, (kLand[i][2] + 14.0f) * s, SMS_RGBA(150, 205, 230, 140));
		for (int i = 0; i < nLand; i++)
			d.circle(mx + kLand[i][0] * s, my + kLand[i][1] * s, (kLand[i][2] + 4.0f) * s, SMS_RGB(236, 214, 150));
		for (int i = 0; i < nLand; i++)
			d.circle(mx + kLand[i][0] * s, my + kLand[i][1] * s, (kLand[i][2] - 8.0f) * s, SMS_RGB(120, 180, 90));
		// the frame and its name
		d.rect_outline(mx, my, mapW, mapH, 3.0f, SMS_RGB(90, 60, 30));
		d.text(mx + 12, my + 10, 22, SMS_RGB(255, 240, 200), SMS_TEXT_OUTLINE, "Isle Delfino");

		// the places
		for (size_t i = 0; i < places.size(); i++) {
			const Place& pl = places[i];
			const float x = mx + pl.x * mapW, y = my + pl.y * mapH;
			const bool on = (int)i == sel;
			if (on) {
				const float pulse = 9.0f + 2.0f * sinf((float)now * 6.0f);
				d.circle_outline(x, y, pulse, 2.5f, SMS_RGB(255, 230, 60));
			}
			d.circle(x, y, 6.0f, SMS_RGB(60, 40, 20));
			d.circle(x, y, 4.5f, pl.open ? SMS_RGB(230, 40, 40) : SMS_RGB(150, 150, 150));
			if (pl.open || on) {
				const float size = on ? 13.0f : 10.0f;
				const int flags  = SMS_TEXT_OUTLINE | (pl.onMap ? SMS_TEXT_CENTER : 0);
				d.text(pl.onMap ? x : x + 10, pl.onMap ? y + 8 : y - 6, size, pl.open ? SMS_RGB(255, 255, 255) : SMS_RGB(200, 200, 200),
				       flags, pl.name.c_str());
			}
		}

		// the panel: the place chosen, and its episodes
		d.rect(px, my, panelW, mapH, SMS_RGBA(20, 25, 40, 230));
		d.rect_outline(px, my, panelW, mapH, 2.0f, SMS_RGB(90, 60, 30));
		float y = my + 12;
		if (sel < 0) {
			d.text(px + 12, y, 13, SMS_RGB(255, 255, 255), SMS_TEXT_SHADOW, "No levels yet");
		} else {
			const Place& pl = places[sel];
			d.clip_push(px, my, panelW, mapH);
			d.text(px + 12, y, 17, SMS_RGB(255, 230, 60), SMS_TEXT_SHADOW, pl.name.c_str());
			y += 28;
			if (!pl.open) {
				d.text(px + 12, y, 12, SMS_RGB(200, 200, 200), SMS_TEXT_SHADOW, "Not reached yet");
			} else if (!inList) {
				char line[64];
				if (pl.entries.size() == 1)
					snprintf(line, sizeof line, "%s", "A to teleport");
				else
					snprintf(line, sizeof line, "%d to choose from, A to see", (int)pl.entries.size());
				d.text(px + 12, y, 12, SMS_RGB(220, 220, 220), SMS_TEXT_SHADOW, line);
				y += 20;
				for (size_t i = 0; i < pl.entries.size() && y < my + mapH - 20; i++, y += 17)
					d.text(px + 16, y, 11, SMS_RGB(170, 180, 200), SMS_TEXT_SHADOW, pl.entries[i].label.c_str());
			} else {
				// the list, scrolled to keep the one chosen in view
				const float rowH = 19.0f;
				const int rows   = (int)((my + mapH - 30.0f - y) / rowH);
				int first        = item - rows / 2;
				const int n      = (int)pl.entries.size();
				if (first > n - rows)
					first = n - rows;
				if (first < 0)
					first = 0;
				for (int i = first; i < n && i < first + rows; i++, y += rowH) {
					const bool on = i == item;
					if (on)
						d.rect(px + 6, y - 3, panelW - 12, rowH, SMS_RGBA(255, 230, 60, 60));
					d.text(px + 14, y, 12, on ? SMS_RGB(255, 255, 255) : SMS_RGB(180, 190, 210), SMS_TEXT_SHADOW,
					       pl.entries[i].label.c_str());
				}
			}
			d.clip_pop();
		}
		// what the buttons do
		char help[128];
		snprintf(help, sizeof help, "%s", inList ? "Up and down choose   A teleport   B back"
		                                         : "Move to a level   A choose   B or M close");
		d.text(cw * 0.5f, my + mapH + 8, 12, SMS_RGB(255, 255, 255), SMS_TEXT_SHADOW | SMS_TEXT_CENTER, help);
	}

	// ---------------------------------------------------------- the mod

	int load() override
	{
		openKey = bindKey("open_map", "M");
		kUp     = input().key_from_name("Up");
		kDown   = input().key_from_name("Down");
		kLeft   = input().key_from_name("Left");
		kRight  = input().key_from_name("Right");
		kEnter  = input().key_from_name("Return");
		kSpace  = input().key_from_name("Space");
		kEscape = input().key_from_name("Escape");
		kBack   = input().key_from_name("Backspace");
		static const char* const kButtons[] = {"None", "D-pad up", "Z and D-pad up"};
		optionChoice("pad_button", "Controller button to open the map", std::vector<const char*>(kButtons, kButtons + 3), 1);

		const SmsGameState st = state();
		if (st.save_file >= 0)
			loadVisited(SMS_FILES_CURRENT);

		// the save file's levels: read as it is chosen, and each level it goes to
		on<SmsEvSaveFile>(SMS_EV_SAVE_FILE, [this](SmsEvSaveFile& f) {
			if (f.what == SMS_SAVE_FILE_LOADED)
				loadVisited(f.file);
			else if (f.what == SMS_SAVE_FILE_NEW)
				visited.clear(), fileLoaded = f.file;
		});
		on<SmsEvStage>(SMS_EV_STAGE_LOADED, [this](SmsEvStage& s) {
			endLeaving();
			shown = false;
			const SmsGameState g = state();
			if (g.save_file < 0 || s.area < 0 || s.area > 254 || s.episode < 0 || s.episode > 255)
				return;
			if (sunshineWorlds() && areaKind(s.area) == SMS_AREA_TITLE)
				return;
			if (fileLoaded != g.save_file)
				loadVisited(SMS_FILES_CURRENT), fileLoaded = g.save_file;
			if (visited.insert(std::make_pair(s.area, s.episode)).second)
				saveVisited();
		});
		// nothing hurts him while the map is up
		on<SmsEvent>(SMS_EV_PLAYER_DAMAGE, [this](SmsEvent&) { return shown || leaving ? SMS_CANCEL : SMS_CONTINUE; });

		on<SmsEvFrame>(SMS_EV_FRAME, [this](SmsEvFrame&) {
			const SmsGameState st = state();
			if (leaving) {
				if (core().time() - leftAt > 6.0) // the warp never came
					endLeaving();
				return;
			}
			if (!shown) {
				if (canOpen(st) && openPressed())
					openMap();
				return;
			}
			if (st.mode != SMS_MODE_STAGE || !st.stage_ready || st.paused) {
				closeMap(true);
				return;
			}
			controls();
		});
		on<SmsEvDraw>(SMS_EV_DRAW, [this](SmsEvDraw& d) {
			if (shown)
				drawMap(d.canvas_w, d.canvas_h);
			else if (leaving)
				draw().text(d.canvas_w * 0.5f, d.canvas_h * 0.5f, 18, SMS_RGB(255, 255, 255), SMS_TEXT_OUTLINE | SMS_TEXT_CENTER,
				            "Teleporting");
		});

		info("loaded; %s opens the map", openKey > 0 ? input().key_name(openKey) : "(no key)");
		return 0;
	}
};

SMS_MOD_CLASS(MapTeleport)
