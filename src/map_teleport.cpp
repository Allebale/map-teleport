// Map Teleport: a map of Isle Delfino, opened in a stage (N, or a controller button), with every
// level the save file has reached on it; choose one and Mario is taken there, as a warp pipe would.
// No episodes to choose: a Super Mario Sunshine world goes to the one its episode select offers
// first, the plaza as the story has it, other levels to where they were first been to.
//
// Which levels: the ones this save file has been to since the mod was on (the mod's own file for
// each save file of each game, written at once), and in Super Mario Sunshine also every world the
// file has a Shine of. Super Mario Eclipse numbers its stages its own way, so there only the levels
// been to. A setting opens every level.
//
// The map is the mod's own: Isle Delfino drawn from circles, each level's place on it found by its
// name (game->area_name); a level whose name is not known goes in a list at the map's side. The
// panel at its side shows the level's picture (pictures/<name>.png, the player's own screenshots),
// whether it is reached, and in Super Mario Sunshine its Shines, coin record and a few words.
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
// (more particular names first: "pianta pit" before "pianta")
const Spot kSpots[] = {
    // Super Mario Sunshine's
    {"airstrip", 1165, 530}, {"plaza", 1015, 632}, {"bianco", 848, 497}, {"ricco station", 850, 650}, {"ricco", 795, 610},
    {"gelato", 678, 483}, {"pinna", 503, 783}, {"sirena", 468, 560}, {"pianta pit", 760, 415}, {"pianta", 780, 362},
    {"noki", 532, 362}, {"corona", 947, 442},
    // Super Mario Eclipse's island
    {"lacrima", 710, 289}, {"rugiv", 797, 271}, {"warship", 882, 298}, {"arido", 995, 353}, {"erto", 1044, 463},
    {"crypt", 912, 537}, {"fire shrine", 985, 553}, {"giovane", 740, 522}, {"sonnolento", 642, 347},
    {"arcobaleno", 454, 443}, {"marea", 638, 655}, {"lighthouse", 657, 747}, {"delfino express", 1090, 600},
    {"peach beach", 770, 905}, {"peach", 1250, 640},
    // and the worlds the Daisy Cruiser sails to, round it
    {"daisy", 925, 872}, {"dream", 830, 985}, {"spettro", 920, 1015}, {"vaporwave", 1015, 1000}, {"lancia", 1095, 950},
    {"red lily", 1110, 860}, {"redlily", 1110, 860}, {"yoshi village", 1040, 800},
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
// each world's other three Shines (the 100 coins' and the blue coins')
const unsigned char kEtcShines[7][3] = {
    {0x64, 0x08, 0x09}, {0x65, 0x12, 0x13}, {0x66, 0x1C, 0x1D}, {0x67, 0x26, 0x27},
    {0x68, 0x30, 0x31}, {0x6A, 0x44, 0x45}, {0x69, 0x3A, 0x3B},
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
	int kUp = -1, kDown = -1, kLeft = -1, kRight = -1;
	bool waitRelease = false; // closed: the game sees nothing until every button is let go

	bool shown   = false; // the map is up
	bool leaving = false; // a teleport asked: the controls kept until the stage changes
	double leftAt = 0;
	// Going back from an episode select a teleport opened: the stage it left, while the select may still
	// be up (backArmed); B pressed there (goingBack): the game has no way back, so the select is closed
	// with A (the episode on its cursor) and, the moment that episode loads, a teleport back is asked
	int backArea = -1, backEpisode = -1;
	bool backArmed = false, goingBack = false, bHeld = true;
	int pressFrames = 0;
	// going back: the openings of the episode passed through and of the stage gone back to are skipped
	// (B pressed for the player, as skipping one by hand, until the stage starts); returning: on the way
	bool skipping = false, returning = false;
	double skipFrom = 0;
	int sel = 0;
	std::map<std::string, SmsImage> pictures; // each level's picture (0: none), loaded once
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

	// the list's file: the mod's own (written at once, so a level counts even when the game is not
	// saved), one for each save file of each game
	std::string listName(int file) const
	{
		char n[48];
		snprintf(n, sizeof n, "visited-%s-%c.txt", state().eclipse ? "eclipse" : "sunshine", 'a' + (file & 3));
		return n;
	}
	void loadVisited(int file)
	{
		visited.clear();
		fileLoaded = file;
		const SmsFilesApi* f = files();
		if (!f || file < 0 || file > 2)
			return;
		static char buf[64 * 1024];
		int n = f->read(handle(), SMS_FILES_MOD, listName(file).c_str(), buf, sizeof buf - 1);
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
		if (fileLoaded >= 0 && fileLoaded <= 2)
			f->write(handle(), SMS_FILES_MOD, listName(fileLoaded).c_str(), s.data(), (uint32_t)s.size());
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
		// Super Mario Eclipse's bonus levels that are old versions of these ("Beta Bianco Hills"): the list
		static const char* const kOld[] = {"beta", "e3 ", "spaceworld", "old ", " v1"};
		for (size_t i = 0; i < sizeof kOld / sizeof *kOld; i++)
			if (n.find(kOld[i]) != std::string::npos)
				return;
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
		const bool all       = setting("all_levels", 0) != 0;
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
			if ((wid == 1 && shines > 0) || all)
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
				next = next > 7 || all ? 7 : next;
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
					if (a == 1 || all || areaVisited(a)) {
						Entry en = {a == 1 ? p.name : partName(a, kind), a, kStoryEpisode};
						p.entries.push_back(en);
					}
					continue;
				}
				if (all && !areaVisited(a)) {
					Entry en = {a == wid ? p.name : partName(a, kind), a, 0};
					p.entries.push_back(en);
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
		const bool all = setting("all_levels", 0) != 0;
		for (std::map<std::string, Place>::iterator it = by.begin(); it != by.end(); ++it) {
			Place& p = it->second;
			int part = 0;
			for (size_t i = 0; i < p.areas.size(); i++) {
				const int a = p.areas[i];
				if (!areaVisited(a)) {
					if (all) { // never been: its first episode
						std::string label = p.areas.size() > 1 ? "Part " + std::to_string(i + 1) : p.name;
						Entry en = {label, a, 0};
						p.entries.push_back(en);
						part++;
					}
					continue;
				}
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
		// one place to go for each level, no episodes to choose: a Sunshine world's the one its episode
		// select offers first (the newest with no Shine), the plaza as the story has it, else the first
		// part and episode been to
		for (std::map<std::string, Place>::iterator it = by.begin(); it != by.end(); ++it) {
			Place& p = it->second;
			if (p.entries.empty())
				continue;
			Entry go = p.entries[0];
			if (sunshineWorlds() && episodeShines(p.world))
				for (size_t i = 0; i < p.entries.size(); i++)
					if (p.entries[i].area == p.world && p.entries[i].episode >= go.episode)
						go = p.entries[i];
			// no episode named: the game opens a world's episode select (TMarDirector::moveStage), picks the
			// plaza's, the hotel's and the park's by the story, and takes any other level's first
			go.episode = kStoryEpisode;
			p.entries.assign(1, go);
		}
		places.clear();
		int side = 0;
		for (std::map<std::string, Place>::iterator it = by.begin(); it != by.end(); ++it) {
			Place p = it->second;
			// one way in for each place (no episodes to choose: a warp's episode is not always kept,
			// Eclipse picks its own): a Sunshine world's newest episode (as its episode select
			// offers first), else the first one listed
			if (p.entries.size() > 1) {
				size_t keep = 0;
				for (size_t i = 0; i < p.entries.size(); i++)
					if (p.world >= 0 && p.entries[i].area == p.world && episodeShines(p.world) &&
					    p.entries[i].episode >= p.entries[keep].episode && p.entries[keep].area == p.world)
						keep = i;
				Entry e = p.entries[keep];
				p.entries.clear();
				p.entries.push_back(e);
			}
			if (!p.entries.empty())
				p.entries[0].label = p.name;
			placeOnMap(p);
			if (!p.onMap) {
				if (!p.open)
					continue; // an unknown name not reached: left out
				const int row = side % 12, col = side / 12;
				side++;
				p.x = 0.04f + 0.22f * (float)col, p.y = 0.12f + 0.07f * (float)row;
			}
			places.push_back(p);
		}
		int reached = 0;
		for (size_t i = 0; i < places.size(); i++)
			reached += places[i].open ? 1 : 0;
		info("map: %s, %d places, %d reached, %d Shines", sunshineWorlds() ? "Super Mario Sunshine" : "Super Mario Eclipse",
		     (int)places.size(), reached, game().shines ? game().shines() : 0);
		if (game().flag_get)
			info("Shadow Mario in the plaza: %s (nozzle at 25 Shines %d, at 30 %d, story %d %d %d)", shadowMarioWaits() ? "waits" : "no",
			     (int)game().flag_get(0x10367u), (int)game().flag_get(0x10366u), (int)game().flag_get(0x10389u),
			     (int)game().flag_get(0x1038Fu), (int)game().flag_get(0x103AEu));
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
		return !by && !waitRelease;
	}

	void sound(uint32_t id) const
	{
		if (api().audio && api().audio->play_system_sound)
			audio().play_system_sound(id);
	}

	void openMap()
	{
		build();
		shown   = true;
		lastDir = 0;
		prevHeld = 0xFFFF; // the button that opened it: let go first
		sound(SMS_SOUND_DECIDE);
	}
	void closeMap(bool quiet = false)
	{
		shown       = false;
		waitRelease = true;
		if (!quiet)
			sound(SMS_SOUND_CANCEL);
	}
	// Super Mario Sunshine: Shadow Mario waits in Delfino Plaza for a chase (TMarDirector::setupObjects,
	// the plaza's story at its episode 8 as decideNextScenario picks it: Yoshi's Egg after Shine 33 (Pinna
	// Park's episode 4), then the Turbo Nozzle's at 25 Shines and the Rocket Nozzle's at 30, which opens
	// Pianta Village). Super Mario Eclipse keeps the nozzle chases at 25 and 30 Shines but has its own plaza
	// story and Shines: only the nozzles and the Shine count are read there.
	bool shadowMarioWaits() const
	{
		const SmsGameApi& g = game();
		if (!g.flag_get || !g.has_shine)
			return false;
		if (!sunshineWorlds()) {
			const int shines = (int)g.flag_get(0x40000u);
			const bool turbo = g.flag_get(0x10367u) != 0, rocket = g.flag_get(0x10366u) != 0;
			return (!turbo && shines >= 25) || (turbo && !rocket && shines >= 30);
		}
		if (g.flag_get(0x103AEu) || !g.flag_get(0x10389u))
			return false; // the plaza's story is past them, or not there yet
		static const int kChases[7] = {0x06, 0x10, 0x1A, 0x24, 0x2E, 0x38, 0x42};
		bool allBeaten = true;
		for (int i = 0; i < 7; i++)
			allBeaten = allBeaten && g.has_shine(kChases[i]);
		if (allBeaten)
			return false;
		const int shines = (int)g.flag_get(0x40000u);
		const bool yoshi = g.flag_get(0x1038Fu) != 0, turbo = g.flag_get(0x10367u) != 0, rocket = g.flag_get(0x10366u) != 0;
		return (!yoshi && g.has_shine(33)) || (!turbo && yoshi && shines >= 25) || (!rocket && turbo && shines >= 30);
	}
	bool shadowMarioFirst() const { return setting("shadow_mario", 1) != 0 && shadowMarioWaits(); }

	void teleport(Entry e)
	{
		// Shadow Mario waits in the plaza: there first, so his chase isn't missed
		if (shadowMarioFirst() && e.area != 1) {
			if (state().area == 1) {
				sound(SMS_SOUND_DAMAGE);
				return;
			}
			info("Shadow Mario waits in Delfino Plaza: going there instead of area %d", e.area);
			e = Entry{"Delfino Plaza", 1, kStoryEpisode};
		}
		if (!game().warp || !game().warp(e.area, e.episode)) {
			sound(SMS_SOUND_DAMAGE);
			return;
		}
		info("teleport to area %d, episode %d (%s)", e.area, e.episode, e.label.c_str());
		const SmsGameState here = state();
		backArea = here.area, backEpisode = here.episode;
		backArmed = setting("go_back", 1) != 0 && backArea >= 0 && backEpisode >= 0 && backArea != e.area;
		goingBack = false, bHeld = true;
		sound(SMS_SOUND_DECIDE);
		shown   = false;
		leaving = true; // the controls stay ours until the stage changes (no jump from the A held)
		leftAt  = core().time();
	}
	void endLeaving()
	{
		if (leaving)
			waitRelease = true;
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

	// every button and stick let go (the controller as read, keyboard and mouse too)
	bool allLetGo() const
	{
		SmsPad p = pad(0);
		return !p.connected || (!p.held && fabsf(p.stick_x) < 0.3f && fabsf(p.stick_y) < 0.3f && fabsf(p.cstick_x) < 0.3f &&
		                        fabsf(p.cstick_y) < 0.3f && p.trigger_l < 0.2f && p.trigger_r < 0.2f);
	}

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

		// (the keyboard and mouse as the port maps them: a left click is A, a right click B, Enter Start)
		const bool choose = (pressed & SMS_PAD_A) != 0;
		const bool back   = (pressed & (SMS_PAD_B | SMS_PAD_START)) || key(openKey);

		if (step) {
			const float dx = dir == 3 ? -1.0f : dir == 4 ? 1.0f : 0.0f, dy = dir == 1 ? -1.0f : dir == 2 ? 1.0f : 0.0f;
			move(dx, dy);
		}
		if (back)
			closeMap();
		else if (choose && sel >= 0) {
			const Place& pl = places[sel];
			if (!pl.open || pl.entries.empty())
				sound(SMS_SOUND_DAMAGE);
			else
				teleport(pl.entries[0]);
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

	// a level's picture: pictures/<its name>.png in the mod's folder (lower case, spaces as '-'),
	// for the player's own screenshots
	static std::string pictureName(const std::string& name)
	{
		std::string k;
		for (size_t i = 0; i < name.size(); i++) {
			const char c = (char)tolower((unsigned char)name[i]);
			if (isalnum((unsigned char)c))
				k += c;
			else if (c == ' ' && !k.empty() && k[k.size() - 1] != '-')
				k += '-';
		}
		while (!k.empty() && k[k.size() - 1] == '-')
			k.erase(k.size() - 1);
		return "pictures/" + k + ".png";
	}
	SmsImage picture(const std::string& name)
	{
		const std::string file = pictureName(name);
		std::map<std::string, SmsImage>::iterator it = pictures.find(file);
		if (it != pictures.end())
			return it->second;
		SmsImage im = draw().image_load ? draw().image_load(handle(), file.c_str()) : 0;
		if (!im && state().eclipse && file != "pictures/eclipse.png") // a Super Mario Eclipse level with none of its own: the game's picture
			im = picture("Eclipse");
		pictures[file] = im;
		return im;
	}

	// a few words on a Super Mario Sunshine level (the mod's own), or null
	static const char* about(const std::string& name)
	{
		static const struct {
			const char* key;
			const char* text;
		} kAbout[] = {
		    // from the Super Mario Wiki (mariowiki.com), in the mod's words
		    {"airstrip", "The first area. A small landing strip on islets near Delfino Plaza, where Mario finds FLUDD and faces Proto Piranha."},
		    {"plaza", "Isle Delfino's biggest town and the hub to every level, with the Shine Gate, the Grand Pianta Statue, a lighthouse and sewers below."},
		    {"bianco", "The first level, entered through a Rainbow M. A windmill village of Piantas, green hills and a lake with the Big Windmill. Boss, Petey Piranha."},
		    {"ricco station", "A pathway. Its Shine comes from cleaning a red sign."},
		    {"ricco", "A busy port of ships, the tall Ricco Tower and a building site of cranes. Bosses, Gooper Blooper and Shadow Mario."},
		    {"gelato", "The isle's largest and sunniest beach, where mirrors keep the Sand Bird's egg warm in the Shine Tower. Bosses, Wiggler and Plungelo."},
		    {"pinna", "An amusement park on the dolphin's tail fin, with a roller coaster, a Ferris wheel and pirate ships. Boss, Mecha-Bowser."},
		    {"sirena", "A south coast beach, home of Hotel Delfino with its attic maze and casino. Opened by Yoshi eating a giant pineapple. Boss, King Boo."},
		    {"noki", "The Nokis' home. A bay of cliffs and waterfalls where sprayed paintings show hidden paths. Boss, Eely-Mouth, among the most bosses of any level."},
		    {"pianta pit", "A treetop village with Chucksters, a great tree and campfires to put out. Tramplin Stu roams the pit."},
		    {"pianta", "The Piantas' old home, built on a giant tree over a pit. Reached by a pipe atop the Shine Gate, with the Rocket Nozzle."},
		    {"corona", "The final level, a volcano behind Delfino Plaza, of lava and a mudboat ride. Bowser and Bowser Jr. wait at the top."},
		    // Super Mario Eclipse's (its own guide)
		    {"erto", "A town with a train station, a pavilion, rock pillars with tightropes and a cave tunnel."},
		    {"lighthouse", "A lighthouse, a fruit hut, a forest path and a Blooper race through stone rings."},
		    {"lacrima", "A hotel with a reception desk, a kitchen, an arcade and a mafia basement."},
		    {"warship", "Pirate ships, a submarine and an underwater base with Cheep Cheep tanks."},
		    {"fire shrine", "Corona Mountain made over, of pillars, clouds, lava and blocks. It turns back into Corona Mountain once the plaza floods."},
		    {"giovane", "A pathway of waterfalls. Its Shine stands on a pedestal behind the lowest one."},
		    {"crypt", "A pathway where all nine torches on the walls must be put out."},
		    {"marea", "A pathway where a water mushroom must be ground pounded."},
		    {"arido", "A pathway where eight flowers by the wall must be sprayed open."},
		    {"delfino express", "A pathway where a little girl talks until a Shine appears."},
		    {"arcobaleno", "A pathway where a pearl in a clam's mouth must be ground pounded."},
		    {"sonnolento", "A pathway with a lamp fire to put out in the middle of the grove."},
		    {"rugiv", "A pathway where stone statues tell their story."},
		    {"daisy", "The hub after the game is beaten. A cruise ship sailing to seven new levels, with the FLUDDless Flight Path Peril."},
		    {"dream", "Broken pillars, a rotating square, an arch of coins and a rainbow bridge."},
		    {"spettro", "A casino of poker chips and card houses full of Boos."},
		    {"vaporwave", "A vaporwave world of checkerboards and blue Cataquacks."},
		    {"lancia", "An icy mountain with a castle, ice puzzles and a giant ice cube."},
		    {"red lily", "A town of alleys, bubbles, a playground and a Coconut Cafe."},
		    {"redlily", "A town of alleys, bubbles, a playground and a Coconut Cafe."},
		    {"peach beach", "Shipping containers, a coral ring, a plaza and go-karts."},
		    {"yoshi village", "Yoshi houses, an umbrella and a flagpole, by the cruiser."},
		    {"peach", "The bonus hub, leading to the bonus levels."},
		};
		const std::string n = lower(name);
		if (n.find("beta") != std::string::npos || n.find("e3 ") != std::string::npos || n.find("spaceworld") != std::string::npos ||
		    n.find("old ") != std::string::npos)
			return nullptr; // an old version of a level (Super Mario Eclipse's bonus levels)
		for (size_t i = 0; i < sizeof kAbout / sizeof *kAbout; i++)
			if (n.find(kAbout[i].key) != std::string::npos)
				return kAbout[i].text;
		return nullptr;
	}

	// text in lines no wider than w
	float wrapText(float x, float y, float w, float size, SmsColor c, const char* text) const
	{
		const SmsDrawApi& d = draw();
		std::string line, word;
		const std::string t = std::string(text) + " ";
		for (size_t i = 0; i < t.size(); i++) {
			if (t[i] != ' ') {
				word += t[i];
				continue;
			}
			const std::string next = line.empty() ? word : line + " " + word;
			if (!line.empty() && d.text_width(size, next.c_str()) > w) {
				d.text(x, y, size, c, SMS_TEXT_SHADOW, line.c_str());
				y += size + 4;
				line = word;
			} else
				line = next;
			word.clear();
		}
		if (!line.empty()) {
			d.text(x, y, size, c, SMS_TEXT_SHADOW, line.c_str());
			y += size + 4;
		}
		return y;
	}

	void drawPanel(float px, float py, float pw, float ph)
	{
		const SmsDrawApi& d = draw();
		d.rect(px, py, pw, ph, SMS_RGBA(20, 25, 40, 230));
		d.rect_outline(px, py, pw, ph, 2.0f, SMS_RGB(90, 60, 30));
		float y = py + 12;
		if (sel < 0) {
			d.text(px + 12, y, 13, SMS_RGB(255, 255, 255), SMS_TEXT_SHADOW, "No levels yet");
			return;
		}
		const Place& pl = places[sel];
		d.clip_push(px, py, pw, ph);
		y = wrapText(px + 12, y, pw - 24, 16, SMS_RGB(255, 230, 60), pl.name.c_str()) + 6;

		// its picture (16 by 9), or where one may go
		const float iw = pw - 24, ih = iw * 9.0f / 16.0f;
		const SmsImage im = picture(pl.name);
		if (im) {
			d.image(im, px + 12, y, iw, ih, pl.open ? SMS_RGB(255, 255, 255) : SMS_RGB(110, 110, 110));
		} else {
			d.rect(px + 12, y, iw, ih, SMS_RGBA(255, 255, 255, 18));
			d.rect_outline(px + 12, y, iw, ih, 1.0f, SMS_RGBA(255, 255, 255, 60));
			d.text(px + 12 + iw * 0.5f, y + ih * 0.5f - 12, 11, SMS_RGB(170, 170, 180), SMS_TEXT_SHADOW | SMS_TEXT_CENTER, "No picture yet");
			d.text(px + 12 + iw * 0.5f, y + ih * 0.5f + 4, 9, SMS_RGB(130, 130, 140), SMS_TEXT_SHADOW | SMS_TEXT_CENTER,
			       pictureName(pl.name).c_str());
		}
		y += ih + 10;

		// reached or not, and what to press
		d.text(px + 12, y, 12, pl.open ? SMS_RGB(140, 230, 140) : SMS_RGB(200, 200, 200), SMS_TEXT_SHADOW,
		       pl.open ? "A to teleport here" : "Not reached yet");
		y += 20;

		// Super Mario Sunshine: its Shines and coin record
		if (sunshineWorlds()) {
			const SmsGameApi& g = game();
			if (const unsigned char* ep = episodeShines(pl.world)) {
				const unsigned char* etc = kEtcShines[shineStage(pl.world) - 2];
				int had = 0;
				for (int i = 0; i < 8; i++)
					had += g.has_shine && g.has_shine(ep[i]) ? 1 : 0;
				for (int i = 0; i < 3; i++)
					had += g.has_shine && g.has_shine(etc[i]) ? 1 : 0;
				char line[64];
				snprintf(line, sizeof line, "Shines  %d of 11", had);
				d.text(px + 12, y, 12, SMS_RGB(255, 255, 255), SMS_TEXT_SHADOW, line);
				y += 17;
				const int record = g.flag_get ? (int)g.flag_get(0x20005u + (uint32_t)pl.world) : 0;
				snprintf(line, sizeof line, "Coin record  %d", record);
				d.text(px + 12, y, 12, SMS_RGB(255, 255, 255), SMS_TEXT_SHADOW, line);
				y += 21;
			}
		}
		if (const char* a = about(pl.name))
			y = wrapText(px + 12, y, pw - 24, 11, SMS_RGB(190, 200, 220), a);
		d.clip_pop();
	}

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

		// the panel: the place chosen, its picture and what is known of it
		drawPanel(px, my, panelW, mapH);

		// what the buttons do
		char help[128];
		snprintf(help, sizeof help, "%s", "Move to a level   A teleport   B close");
		d.text(cw * 0.5f, my + mapH + 8, 12, SMS_RGB(255, 255, 255), SMS_TEXT_SHADOW | SMS_TEXT_CENTER, help);
		if (shadowMarioFirst())
			d.text(cw * 0.5f, my + mapH + 26, 12, SMS_RGB(255, 170, 90), SMS_TEXT_SHADOW | SMS_TEXT_CENTER,
			       state().area == 1 ? "Shadow Mario is here in Delfino Plaza. Catch him first"
			                         : "Shadow Mario is in Delfino Plaza. Teleporting takes you there until you catch him");
	}

	// ---------------------------------------------------------- the mod

	int load() override
	{
		openKey = bindKey("open_map", "N");
		kUp     = input().key_from_name("Up");
		kDown   = input().key_from_name("Down");
		kLeft   = input().key_from_name("Left");
		kRight  = input().key_from_name("Right");
		static const char* const kButtons[] = {"None", "D-pad up", "Z and D-pad up"};
		optionChoice("pad_button", "Controller button to open the map", std::vector<const char*>(kButtons, kButtons + 3), 1);
		optionBool("all_levels", "Every level can be teleported to (not only the ones reached)", false);
		optionBool("shadow_mario", "Go to Delfino Plaza first while Shadow Mario waits there", true);
		optionBool("go_back", "B in the episode select goes back to where you teleported from", true);

		const SmsGameState st = state();
		if (st.save_file >= 0)
			loadVisited(st.save_file);

		// the save file's levels: read as it is chosen, and each level it goes to
		on<SmsEvSaveFile>(SMS_EV_SAVE_FILE, [this](SmsEvSaveFile& f) {
			const SmsFilesApi* fs = files();
			if (f.what == SMS_SAVE_FILE_LOADED)
				loadVisited(f.file);
			else if (f.what == SMS_SAVE_FILE_NEW) { // a new game: none yet
				visited.clear(), fileLoaded = f.file;
				if (fs)
					fs->remove(handle(), SMS_FILES_MOD, listName(f.file).c_str());
			} else if (f.what == SMS_SAVE_FILE_ERASED && fs)
				fs->remove(handle(), SMS_FILES_MOD, listName(f.file).c_str());
			else if (f.what == SMS_SAVE_FILE_COPIED && fs && f.from >= 0) {
				static char buf[64 * 1024];
				const int n = fs->read(handle(), SMS_FILES_MOD, listName(f.from).c_str(), buf, sizeof buf);
				if (n > 0 && n <= (int)sizeof buf)
					fs->write(handle(), SMS_FILES_MOD, listName(f.file).c_str(), buf, (uint32_t)n);
				else
					fs->remove(handle(), SMS_FILES_MOD, listName(f.file).c_str());
			}
		});
		// the opening over: nothing more to skip
		on<SmsEvStage>(SMS_EV_STAGE_START, [this](SmsEvStage&) {
			if (!returning)
				skipping = false;
		});
		// the episode select closed for going back: the stage left loads in place of the episode chosen,
		// so none is passed through (a game without this event: warped back from it, below)
		on<SmsEvStageLoading>(SMS_EV_STAGE_LOADING, [this](SmsEvStageLoading& l) {
			if (!goingBack)
				return;
			info("back from the episode select to area %d, episode %d (instead of %d, %d)", backArea, backEpisode,
			     l.area, l.episode);
			l.area = backArea, l.episode = backEpisode;
		});
		on<SmsEvStage>(SMS_EV_STAGE_LOADED, [this](SmsEvStage& s) {
			endLeaving();
			shown = false;
			if (goingBack && s.area == backArea && s.episode == backEpisode) { // back already: its opening skipped
				goingBack = backArmed = false;
				skipping = true, skipFrom = core().time(), pressFrames = 0;
			} else if (goingBack) { // the episode select closed for going back: straight on to the stage left
				goingBack = backArmed = false;
				if (game().warp && game().warp(backArea, backEpisode)) {
					info("back from the episode select to area %d, episode %d", backArea, backEpisode);
					leaving = true, leftAt = core().time();
					skipping = returning = true, skipFrom = core().time(), pressFrames = 0;
				}
			} else if (returning) { // back: its opening skipped too
				returning = false;
				skipping = true, skipFrom = core().time(), pressFrames = 0;
			} else
				backArmed = false; // an episode chosen: here to stay
			const SmsGameState g = state();
			if (g.save_file < 0 || s.area < 0 || s.area > 254 || s.episode < 0 || s.episode > 255)
				return;
			if (sunshineWorlds() && areaKind(s.area) == SMS_AREA_TITLE)
				return;
			if (fileLoaded != g.save_file)
				loadVisited(g.save_file);
			if (visited.insert(std::make_pair(s.area, s.episode)).second)
				saveVisited();
		});
		// the game (and the other mods) see no controller while the map is up, and after it until
		// everything is let go (input->capture also kept the keyboard's key-ups from the game: a key
		// held as it opened stayed held)
		on<SmsEvPadRead>(SMS_EV_PAD_READ, [this](SmsEvPadRead& r) {
			// going back: the openings skipped with B, pressed and let go in turn (none for long: a few
			// openings can't be skipped)
			if (skipping) {
				if (core().time() - skipFrom > 12.0)
					skipping = false;
				else {
					for (int i = 0; i < 4; i++) {
						const int8_t err = r.pads[i].err;
						memset(&r.pads[i], 0, sizeof r.pads[i]);
						r.pads[i].err = err;
					}
					if ((pressFrames++ / 3) % 2 == 0)
						r.pads[0].buttons = SMS_PAD_B;
					r.pads[0].err = 0;
					return;
				}
			}
			// the episode select a teleport opened: B (pressed anew) goes back, A closing it for us
			if (backArmed && state().mode == SMS_MODE_TITLE) {
				bool b = false;
				for (int i = 0; i < 4; i++)
					b = b || (r.pads[i].err == 0 && (r.pads[i].buttons & SMS_PAD_B));
				if (!goingBack && b && !bHeld) {
					goingBack = true, pressFrames = 0;
					sound(SMS_SOUND_CANCEL);
				}
				bHeld = b;
				if (goingBack) {
					for (int i = 0; i < 4; i++) {
						const int8_t err = r.pads[i].err;
						memset(&r.pads[i], 0, sizeof r.pads[i]);
						r.pads[i].err = err;
					}
					// A pressed and let go in turn, until the select takes it
					if ((pressFrames++ / 3) % 2 == 0)
						r.pads[0].buttons = SMS_PAD_A;
					r.pads[0].err = 0;
					return;
				}
			}
			if (waitRelease && !shown && !leaving && allLetGo())
				waitRelease = false;
			if (!shown && !leaving && !waitRelease)
				return;
			for (int i = 0; i < 4; i++) {
				const int8_t err = r.pads[i].err;
				memset(&r.pads[i], 0, sizeof r.pads[i]);
				r.pads[i].err = err;
			}
		}, SMS_PRIORITY_EARLY);
		// nothing hurts him while the map is up
		on<SmsEvent>(SMS_EV_PLAYER_DAMAGE, [this](SmsEvent&) { return shown || leaving ? SMS_CANCEL : SMS_CONTINUE; });

		on<SmsEvFrame>(SMS_EV_FRAME, [this](SmsEvFrame&) {
			const SmsGameState st = state();
			if (leaving) {
				// the stage left (an episode select is up, or the next stage loading), or the warp never came
				if (st.mode != SMS_MODE_STAGE || core().time() - leftAt > 6.0)
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
			if (backArmed && !goingBack && state().mode == SMS_MODE_TITLE)
				// bottom right, clear of the select's score
				draw().text(d.canvas_w - 28.0f, d.canvas_h - 36.0f, 14, SMS_RGB(255, 255, 255), SMS_TEXT_OUTLINE | SMS_TEXT_RIGHT,
				            "B  Go back");

		});

		info("loaded; %s opens the map", openKey > 0 ? input().key_name(openKey) : "(no key)");
		return 0;
	}
};

SMS_MOD_CLASS(MapTeleport)
