#include <iostream>
#include <string>
#include <vector>

struct fumen_status
{
	std::string difficulty;
	std::string dual;
	std::string side;
	std::string level;
	int operation;

	void represent()
	{
		std::cout << "course:\t" << difficulty << std::endl;
		std::cout << "dual: \t" << dual << std::endl;
		std::cout << "side: \t" << side << std::endl;
		std::cout << "level: \t" << level << std::endl;
		std::cout << "operate:" << operation << std::endl;
	}
};

struct beat_measure
{
	int beat;
	int measure;

	beat_measure(int b, int m)
		:beat(b), measure(m) {}
	beat_measure()
		:beat(4), measure(4) {}

	bool operator==(beat_measure& object)
	{
		return beat == object.beat && measure == object.measure;
	}
	bool operator!=(beat_measure& object)
	{
		return beat != object.beat || measure != object.measure;
	}

	void represent()
	{
		std::cout << beat << "/" << measure << std::endl;
	}
};
const static beat_measure BEAT_MEASURE_DEFAULT;

struct branch_state
{
	bool is_branched;
	std::vector<std::string> branch_condition;
	int branch_side;
	int condition_reset_times;

	void represent()
	{
		std::cout << "branch: " << (is_branched ? "true" : "false") << std::endl;
		std::cout << "condition: ";
		for(std::string& segment: branch_condition) { std::cout << segment << ", "; }
		std::cout << std::endl;
		std::cout << "side: " << branch_side << std::endl;
		std::cout << "reset: " << condition_reset_times << std::endl;
	}
};

struct fumen_header
{
	std::string TITLE; 
	std::string SUBTITLE;
	std::string WAVE;
	std::vector<int> BALLOON;
	int LEVEL;
	int COURSE;
	double BPM;
	double OFFSET;
	double DEMOSTART;
	int SCOREINIT;
	int SCOREDIFF;
	int SCOREMODE;
	int SONGVOL;
	int SEVOL;
	int SIDE;
	std::string GAME;

	//unknown
	std::string TOTAL;
	std::string STYLE;

	void represent()
	{
		std::cout << "TITLE: " << TITLE << std::endl;
		std::cout << "SUBTITLE: " << SUBTITLE << std::endl;
		std::cout << "WAVE:" << WAVE << std::endl;
		std::cout << "LEVEL: " << LEVEL << std::endl;
		std::cout << "COURSE: " << COURSE << std::endl;
		std::cout << "SIDE: " << SIDE << std::endl;
		std::cout << "BPM:" << BPM << std::endl;
		std::cout << "OFFSET: " << OFFSET << std::endl;
		std::cout << "BALLOON: ";
		for (int& balloon : BALLOON) { std::cout << balloon << ", "; }
		std::cout << std::endl;
		std::cout << "DEMOSTART:" << DEMOSTART << std::endl;
		std::cout << "SONGVOL:" << SONGVOL << std::endl;
		std::cout << "SEVOL:" << SEVOL << std::endl;
		std::cout << "SCOREINIT:" << SCOREINIT << std::endl;
		std::cout << "SCOREDIFF:" << SCOREDIFF << std::endl;
		std::cout << "SCOREMODE:" << SCOREMODE << std::endl;
		std::cout << "GAME:" << GAME << std::endl;
		std::cout << "TOTAL:" << TOTAL << std::endl;
		std::cout << "STYLE:" << STYLE << std::endl;
	}
	
	std::string get_header()
	{
		std::string container;

		container += "TITLE:" + TITLE + "\n";
		container += "SUBTITLE:" + SUBTITLE + "\n";
		container += "WAVE:" + WAVE + "\n";
		container += "LEVEL:" + std::to_string(LEVEL) + "\n";
		container += "COURSE:" + std::to_string(COURSE) + "\n";
		container += "SIDE:" + std::to_string(SIDE) + "\n";
		container += "BPM:" + std::to_string(BPM) + "\n";
		container += "OFFSET:" + std::to_string(OFFSET) + "\n";
		container += "BALLOON:";
		for (int& balloon : BALLOON) { container += std::to_string(balloon) + ", "; }
		container += "\n";
		container += "DEMOSTART:" + std::to_string(DEMOSTART) + "\n";
		container += "SONGVOL:" + std::to_string(SONGVOL) + "\n";
		container += "SEVOL:" + std::to_string(SEVOL) + "\n";
		container += "SCOREINIT:" + (SCOREINIT == -1 ? "" : std::to_string(SCOREINIT)) + "\n";
		container += "SCOREDIFF:" + (SCOREDIFF == -1 ? "" : std::to_string(SCOREDIFF)) + "\n";
		container += "SCOREMODE:" + std::to_string(SCOREMODE) + "\n";
		container += "GAME:" + GAME + "\n";
		container += "TOTAL:" + TOTAL + "\n";
		container += "STYLE:" + STYLE + "\n";

		return container;
	}
};

struct bar_note_set
{
	bar_note_set(int b, int n)
		:bar(b), note(n) {}
	bar_note_set()
		:bar(-1), note(-1) {}
	int bar;
	int note;

	bool operator==(const bar_note_set& object) { return bar == object.bar && note == object.note; }
	bool operator!=(const bar_note_set& object) { return bar != object.bar || note != object.note; }
	bool operator>=(const bar_note_set& object) { return bar >= object.bar || (bar == object.bar && note >= object.note); }
	bool operator<=(const bar_note_set& object) { return bar <= object.bar || (bar == object.bar && note <= object.note); }
	bool operator>(const bar_note_set& object) { return bar > object.bar || (bar == object.bar && note > object.note); }
	bool operator<(const bar_note_set& object) { return bar < object.bar || (bar == object.bar && note < object.note); }

	void operator=(const bar_note_set& object)
	{
		bar = object.bar;
		note = object.note;
	}

	void represent()
	{
		std::cout << "bar: " << bar << " note: " << note << std::endl;
	}

	void simple_repersent()
	{
		std::cout << bar << " - " << note;
	}
};
const static bar_note_set BAR_NOTE_NULL;
const static bar_note_set BAR_NOTE_INIT(0,0);

struct roll_info
{
	roll_info(bar_note_set s, bar_note_set e)
		:type(-1), start(s), end(e) {}
	roll_info()
		:type(-1) {}
	int type;
	bar_note_set start;
	bar_note_set end;

	void represent()
	{
		std::cout << type << ": [" << start.bar << "," << start.note << "] {" << end.bar << "," << end.note << "]" << std::endl;
	}
};

struct note_status
{
	char note;
	double bpm;
	double scroll;
	beat_measure measure;
	bool barline;
	bool gogo;
	double delay;

	bool operator==(note_status& object)
	{
		return note == object.note && bpm == object.bpm && scroll == object.scroll && 
			measure == object.measure && barline == object.barline && gogo == object.gogo && delay == object.delay;
	}

	void represent()
	{
		std::cout << "NOTE: " << note << std::endl;
		std::cout << "#BPM" << bpm << std::endl;
		std::cout << "#SCROLL " << scroll << std::endl;
		std::cout << "#MEASURE ";
		measure.represent();
		std::cout << "#BARLINE" << (barline ? "ON" : "OFF") << std::endl;
		std::cout << "#GOGO" << (gogo ? "START" : "END") << std::endl;
		std::cout << "#DELAY " << delay << std::endl;
	}
};
