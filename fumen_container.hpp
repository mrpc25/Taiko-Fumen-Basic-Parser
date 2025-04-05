#include <iostream>
#include <fstream>
#include <limits>
#include <vector>
#include <array>
#include <string>
#include <sstream>
#include <algorithm>
#include <cstdlib>
#include <cassert>
#include <stdexcept>

#include "fumen_struct.hpp"

#define ASSERT(condition, message) \
   do { \
      assert(condition && #message); \
   } while (0)
using namespace std;

class TaikoFumenBasic
{
public:
    TaikoFumenBasic(const char* fumen_path)
    {
        //Load Fumen File Into Variable
        every_row = reading_file(fumen_path);

        //Processing Branch Beforehand Due to parsing logic used
        command_BPMCHANGE_storage = "";
        command_SCROLL_storage = "";
        command_MEASURE_storage = "";
        command_DELAY_storage = "";
        command_BARLINE_storage = "";
        command_GOGO_storage = "";

        branch_encounter = false;
        auto exist_before_comma = [=](const char* phrase, int row_index)
        {
            while (!phrase_exist(",", every_row[row_index]))
            {
                if (phrase_exist(phrase, every_row[row_index])) { return true; }
                row_index += 1;
            };
            return false;
        };

        for (int x = 0; x < every_row.size(); x++)
        {
            row = every_row[x];

            if (phrase_exist("#GOGO", row)) { command_GOGO_storage = row; }
            if (phrase_exist("#DELAY", row)) { command_DELAY_storage = row; }
            if (phrase_exist("#SCROLL", row)) { command_SCROLL_storage = row; }
            if (phrase_exist("#MEASURE", row)) { command_MEASURE_storage = row; }
            if (phrase_exist("#BARLINE", row)) { command_BARLINE_storage = row; }
            if (phrase_exist("#BPMCHANGE", row)) { command_BPMCHANGE_storage = row; }

            if (phrase_exist("#BRANCHSTART", row))
            {
                branch_encounter = true;

                command_GOGO_before_branch = command_GOGO_storage;
                command_DELAY_before_branch = command_DELAY_storage;
                command_SCROLL_before_branch = command_SCROLL_storage;
                command_MEASURE_before_branch = command_MEASURE_storage;
                command_BARLINE_before_branch = command_BARLINE_storage;
                command_BPMCHANGE_before_branch = command_BPMCHANGE_storage;
            }

            //20250304: waiting for check
            if (phrase_exist("#BRANCHEND", row))
            {
                branch_encounter = false;

                //maybe not needed below
                command_GOGO_before_branch.clear();
                command_DELAY_before_branch.clear();
                command_SCROLL_before_branch.clear();
                command_MEASURE_before_branch.clear();
                command_BARLINE_before_branch.clear();
                command_BPMCHANGE_before_branch.clear();
            }

            _every_row.push_back(row);
            if (branch_encounter)
            {
                branch_n = phrase_exist("#N", row);
                branch_e = phrase_exist("#E", row) && (!phrase_exist("#END", row));
                branch_m = phrase_exist("#M", row) && (!phrase_exist("#MEASURE", row));

                if (branch_n || branch_e || branch_m)
                {
                    if (command_GOGO_before_branch.size() != 0 && (!exist_before_comma("#GOGO", x))) { _every_row.push_back(command_GOGO_before_branch); }
                    if (command_DELAY_before_branch.size() != 0 && (!exist_before_comma("#DELAY", x))) { _every_row.push_back(command_DELAY_before_branch); }
                    if (command_SCROLL_before_branch.size() != 0 && (!exist_before_comma("#SCROLL", x))) { _every_row.push_back(command_SCROLL_before_branch); }
                    if (command_MEASURE_before_branch.size() != 0 && (!exist_before_comma("#MEASURE", x))) { _every_row.push_back(command_MEASURE_before_branch); }
                    if (command_BARLINE_before_branch.size() != 0 && (!exist_before_comma("#BARLINE", x))) { _every_row.push_back(command_BARLINE_before_branch); }
                    if (command_BPMCHANGE_before_branch.size() != 0 && (!exist_before_comma("#BPMCHANGE", x))) { _every_row.push_back(command_BPMCHANGE_before_branch); }
                }
            }

            //if (phrase_exist("#BRANCHEND", row))
            if (phrase_exist("#END", row))
            {
                command_BPMCHANGE_storage = "";
                command_SCROLL_storage = "";
                command_MEASURE_storage = "";
                command_DELAY_storage = "";
                command_BARLINE_storage = "";
                command_GOGO_storage = "";

                branch_encounter = false;
            }
        }
        every_row = _every_row;

        //Parse Heading Information
        song_difficulty = find_phrase_in_row("COURSE:");
        song_level = find_phrase_in_row("LEVEL:");

        song_begin = find_phrase_in_row("#START");
        song_endin = find_phrase_in_row("#END");

        ASSERT(song_begin.size() == song_endin.size(), "The amount of \"#START(P1/2)\' and \"#END\" command is not equal, you might need to check if some necessary command was lost.");

        // if a file contains more then one fumen, it's actually not necssacry to fill in all the header information unless it's first (uppermost in file) fumen. 
        // For those not filling everything, it can be filled in by default or previous value
        _last_record_difficulty = "N/A";
        _last_record_side = "-";
        _last_record_level = "?";
        _last_record_style = "";

        for (int i = 0; i < song_begin.size();i++)
        {
            int end = i != 0 ? song_endin[i - 1] + 1 : 0;
            int start = i != 0 ? song_begin[i] : song_begin[0];

            difficulty = remove_spaces(offset_thing_value("COURSE:", end, start));
            level = remove_spaces(offset_thing_value("LEVEL:", end, start));
            style = remove_spaces(offset_thing_value("STYLE:", end, start));
            directness = remove_spaces(offset_thing_value("SIDE:", end, start));
            start_cmd_string = remove_spaces(offset_thing_value("#START", end, start));

            hbscroll = find_phrase_in_row("#HBSCROLL", end, start);
            bmscroll = find_phrase_in_row("#BMSCROLL", end, start);

            std::string dualstatestr = "";
            for (char& alphabet : start_cmd_string)
            {
                if (alphabet == 'P' || alphabet == '1' || alphabet == '2') { dualstatestr += alphabet; }
            }

            //OPERATION
            if (hbscroll.size() + bmscroll.size() > 1) { throw std::runtime_error("Multiple #HBSCROLL or #BMSCROLL command."); }
            if (hbscroll.size() == 1) { operation = 2; }
            else if (bmscroll.size() == 1) { operation = 1; }
            else { operation = 0; }

            //COURSE
            still_in_same_course = false;
            if (difficulty == "4" || lower(difficulty) == "edit") {
                difficulty = "Edit";
            }
            else if (difficulty == "3" || lower(difficulty) == "oni") {
                difficulty = "Oni";
            }
            else if (difficulty == "2" || lower(difficulty) == "hard") {
                difficulty = "Hard";
            }
            else if (difficulty == "1" || lower(difficulty) == "normal") {
                difficulty = "Normal";
            }
            else if (difficulty == "0" || lower(difficulty) == "easy") {
                difficulty = "Easy";
            }
            else if (difficulty == "") {
                difficulty = _last_record_difficulty;
                still_in_same_course = true;
            }
            else {
                throw std::runtime_error("difficulty not existed");
            }
            _last_record_difficulty = difficulty;

            //LEVEL
            if (level == "") { level = _last_record_level; }
            _last_record_level = level;

            //SIDE
            if (directness == "1" || lower(directness) == "normal") {
                side = "EXT";
            }
            else if (directness == "2" || lower(directness) == "ex") {
                side = "INT";
            }
            else if (directness == "3") {
                side = "-";
            }
            else if (directness == "") {
                side = _last_record_side;
            }
            else {
                throw std::runtime_error("Not a available side symbol, expect 1/2/3/None");
            }
            _last_record_side = side;

            //STYLE
            if (dualstatestr == "P1") {
                dual = "P1";
                any_dual = true;
            }
            else if (dualstatestr == "P2") {
                dual = "P2";
                any_dual = true;
            }
            else if (dualstatestr == "") {
                dual = "-";
            }
            else {
                throw std::runtime_error("Invalid input for \"dualstatestr\"");
            }

            if (style == "" && still_in_same_course) { style = _last_record_style; }
            _last_record_style = style;

            //log each header info
            fumen_status header;
            header.difficulty = difficulty;
            header.dual = dual;
            header.side = side;
            header.operation = operation;
            header.level = level;

            fuman_classfication.push_back(header);
        }
    }
    std::vector<fumen_status> fuman_classfication;

protected:
    bool phrase_exist(const char* target, std::string carrier)
    {
        return ((int)carrier.find(target)) != -1;
    }

    std::string remove_spaces(string str)
    {
        str.erase(remove(str.begin(), str.end(), ' '), str.end());
        return str;
    }

    std::string lower(std::string str)
    {
        for (char& alphabet : str)
        {
            if (alphabet >= 48 && alphabet <= 57) { continue; }
            if (!((alphabet >= 65 && alphabet <= 90) || (alphabet >= 97 && alphabet <= 122)))
            {
                cout << "not a alphabet: " << alphabet << "(" << (int)alphabet << ")";
                exit(1);
            }
            if (alphabet < 97) { alphabet += 32; }
        }
        return str;
    }

    std::vector<std::string> reading_file(const char* file_path)
    {
        std::vector<std::string> rows;
        std::ifstream ifs(file_path, std::ios::in);
        if (!ifs.is_open())
        {
            cout << "Failed to open file.\n";
            exit(1);
        }

        std::string current_row;
        while (std::getline(ifs, current_row)) { rows.push_back(current_row); }
        ifs.close();

        return rows;
    }

    std::vector<int> find_phrase_in_row(const char* phrase, int start = 0, int end = -1)
    {
        std::vector<int> Order;
        if (end == -1)
        {
            end = (int)every_row.size();
        }

        std::vector<int> show_up_history;
        for (int i = start; i < end; i++)
        {
            int reference = (int)every_row[i].find(phrase);
            int annotation = (int)every_row[i].find("//");
            bool reference_exist = reference != -1;                           //if target phrase exists
            bool annotation_exist = annotation != -1;                         //if annotation (//) exists
            if ((!annotation_exist) && reference_exist)
            {
                Order.push_back(i);
                continue;
            }
            if (annotation_exist && reference_exist && reference < annotation)
            {
                Order.push_back(i);
            }
        }
        return Order;
    }

    std::vector<int> find_phrase_index(std::string random_string, const char* target)
    {
        std::vector<int> show_up_history;
        int start_point = 0;
        while (true)
        {
            int current_found_index = (int)random_string.find(target, start_point);
            if (current_found_index == -1)
            {
                break;
            }
            show_up_history.push_back(current_found_index);
            start_point = current_found_index + 1;
        }
        return show_up_history;
    }

    std::string offset_thing_value(const char* phrase, int start = 0, int end = -1)
    {
        std::vector<int> phrase_region = find_phrase_in_row(phrase, start, end);
        std::string output = "";
        if (phrase_region.size() == 0) { return output; }

        std::string observed = every_row[phrase_region[0]];
        int anno = (int)observed.find("//");
        int pare = (int)observed.find("(");
        int end_location;

        if (anno == -1 && pare == -1) { end_location = (int)observed.size(); }
        else if (anno == -1) { end_location = pare; }
        else if (pare == -1) { end_location = anno; }
        else { end_location = pare < anno ? pare : anno; }

        std::string phrase_std = phrase;
        for (int i = (int)observed.find(phrase) + (int)phrase_std.size(); i < end_location; i++)
        {
            output = output + observed[i];
        }

        return output;
    }

    std::string offset_thing_value_pare_ignored(const char* phrase, int start = 0, int end = -1)
    {
        std::vector<int> phrase_region = find_phrase_in_row(phrase, start, end);
        std::string output = "";
        if (phrase_region.size() == 0) { return output; }

        std::string observed = every_row[phrase_region[0]];
        int anno = (int)observed.find("//");
        int end_location = anno == -1 ? (int)observed.size() : anno;

        std::string phrase_std = phrase;
        for (int i = (int)observed.find(phrase) + (int)phrase_std.size(); i < end_location; i++)
        {
            output = output + observed[i];
        }

        return output;
    }

    std::vector<std::string> every_row;
    std::vector<std::string> _every_row;

    std::string command_BPMCHANGE_storage;
    std::string command_SCROLL_storage;
    std::string command_MEASURE_storage;
    std::string command_DELAY_storage;
    std::string command_BARLINE_storage;
    std::string command_GOGO_storage;

    std::string command_BPMCHANGE_before_branch;
    std::string command_SCROLL_before_branch;
    std::string command_MEASURE_before_branch;
    std::string command_DELAY_before_branch;
    std::string command_BARLINE_before_branch;
    std::string command_GOGO_before_branch;

    bool branch_encounter;
    bool branch_n;
    bool branch_e;
    bool branch_m;
    std::string row;

    std::vector<int> song_difficulty;
    std::vector<int> song_level;
    std::vector<int> song_begin;
    std::vector<int> song_endin;

    std::string _last_record_difficulty;
    std::string _last_record_side;
    std::string _last_record_level;
    std::string _last_record_style;

    std::string difficulty;
    std::string level;
    std::string style;
    std::string directness;
    std::string start_cmd_string;
    std::vector<int> hbscroll;
    std::vector<int> bmscroll;

    bool still_in_same_course;
    std::string side;
    std::string dual;
    int operation;

    bool any_dual = false;
};

class TaikoFumenContext : public TaikoFumenBasic
{
public:
    TaikoFumenContext(const char* fumen_path, int user_choice = 0)
        : TaikoFumenBasic(fumen_path)
    {
        //Find Region Of Interest To Raw Text Rows By User Input
        if (user_choice >= 0)
        {
            chosen_ready = user_choice == 0 ? 0 : song_endin[user_choice - 1];
            chosen_begin = song_begin[user_choice];
            chosen_endin = song_endin[user_choice];
        }
        else
        {
            throw std::runtime_error("Not A Valid Option Sign.");
        }

        //some initialization
        current_measure = "";

        //Parse Fumen Context By User Input
        for (int i = chosen_begin + 1; i < chosen_endin; i++)
        {
            if (every_row[i].size() == 0) { continue; }

            //inspect whether the row contains annotation sign "//", and record its location
            annotation = (int)every_row[i].find("//");

            //inspect whether the row contains fumen command sign "#", end record its location
            //in addition, text after annotation will be meaningless, so its relative location is considered.
            function_usage = (int)every_row[i].substr(0, annotation == -1 ? every_row[i].size() : annotation).find("#");

            //check if this row is using fumen command like "#SCROLL", "#BPMCHANGE"...
            if (function_usage != -1) { continue; }

            //check every char in a raw row text
            for (int j = 0; j < every_row[i].size(); j++)
            {
                //check if a potential bar is ended. (the bar end sign is ",")
                if (every_row[i][j] != ',')
                {
                    if (!isnumeric(every_row[i][j])) { continue; }
                    if (annotation != -1 && j >= annotation) { continue; }  //inspect each char in a bar, record if it's a number char

                    current_measure += every_row[i][j];         //record actual char
                    current_measure_row_location.push_back(i);  //record where the char is (row)
                }
                else
                {
                    current_measure += every_row[i][j];         //record actual char, and it's end of bar
                    current_measure_row_location.push_back(i);  //record where the char is (row)


                    every_bar.push_back(current_measure);   //store the bar context
                    current_measure = "";                   //clear temporary bar context

                    every_bar_row_location.push_back(current_measure_row_location); //store the correspoding location set
                    current_measure_row_location.clear();                           //clear temporary char location 
                }
            }

            //bar location
            bar_begin_location_temp = every_bar_row_location[0][0];
            bar_begin_location.push_back(bar_begin_location_temp);
            for (std::vector<int>& bar : every_bar_row_location)
            {
                note_in_bar_location.clear();
                for (int& notes_location : bar)
                {
                    if (notes_location == bar_begin_location_temp) { continue; }
                    bar_begin_location_temp = notes_location;
                    bar_begin_location.push_back(bar_begin_location_temp);
                    note_in_bar_location.push_back(bar_begin_location_temp);
                }
                bar_location.push_back(note_in_bar_location);
            }
        }

        scroll_set = find_data_of_each_notes<double>
        (
            "1.0",
            "#SCROLL",
            [](char c) -> bool { return isnumeric(c) || c == '+' || c == '-' || c == '.'; },
            [](std::string s, bool _unused) -> double { return atof(s.c_str()); }
        );

        bpm_set = find_data_of_each_notes<double>
        (
            std::to_string(get_initial_bpm()).c_str(),
            "#BPMCHANGE",
            [](char c) -> bool { return isnumeric(c) || c == '+' || c == '-' || c == '.'; },
            [](std::string s, bool _unused) -> double { return atof(s.c_str()); }
        );

        delay_set = find_data_of_each_notes<double>
        (
            "0.0",
            "#DELAY",
            [](char c) -> bool { return isnumeric(c) || c == '+' || c == '-' || c == '.'; },
            [](std::string s, bool read_nothing) -> double { return read_nothing ? atof(s.c_str()) : 0.0;}
        );

        gogo_set = find_data_of_each_notes<bool>
        (
            "END",
            "#GOGO",
            [](char c) -> bool { return true; },
            [](std::string s, bool _unused) -> bool { return s == "START"; }
        );

        barline_set = find_data_of_each_notes<bool>
        (
            "ON",
            "#BARLINE",
            [](char c) -> bool { return true; },
            [](std::string s, bool _unused) -> bool { return s == "ON"; }
        );

        beatmeasure_set = find_data_of_each_notes<beat_measure>
        (
            "4/4",
            "#MEASURE",
            [](char c) -> bool { return isnumeric(c) || c == '+' || c == '-' || c == '.' || c == '/'; },
            [](std::string s, bool _unused) -> beat_measure
            {
                std::stringstream ss(s);
                std::string temp;
                std::vector<std::string> temp_vector;
                while (getline(ss, temp, '/')) { temp_vector.push_back(temp); }
                if (temp_vector.size() != 2) { throw std::runtime_error("Not a valid #MEASURE command."); }
                beat_measure bm_config(atol(temp_vector[0].c_str()), atol(temp_vector[1].c_str()));
                return bm_config;
            }
        );

        branchstate_set = find_branch_state_of_each_notes();        

        is_branch_exist = find_phrase_in_row("#BRANCHSTART", chosen_begin, chosen_endin).size() > 0;
    }

    double get_initial_bpm()
    {
        std::string temp_content = remove_spaces(offset_thing_value("BPM:", chosen_ready, chosen_begin));
        if (temp_content.size()==0)
        {
            return atof(remove_spaces(offset_thing_value("BPM:", 0, song_begin[0])).c_str());
        }
        return atof(temp_content.c_str());
    }

    bool is_branch_exist;

protected:
    std::vector<std::string> split(std::string input_string, const char c)
    {
        std::stringstream ss(input_string);
        std::string temp;
        std::vector<std::string> temp_vector;
        while (getline(ss, temp, c)) { temp_vector.push_back(temp); }
        return temp_vector;
    }

    std::vector<int> split_to_int(std::string input_string, const char c)
    {
        std::stringstream ss(input_string);
        std::string temp;
        std::vector<int> temp_vector;
        while (getline(ss, temp, c)) { temp_vector.push_back(atoi(temp.c_str())); }
        return temp_vector;
    }

    static bool isnumeric(const char character) { return character >= 48 && character <= 57; }

    template<typename custom_type>
    std::vector<std::vector<custom_type>> find_data_of_each_notes
        (
            const char* default_value,
            const char* target_command,
            bool (*filter_function)(char),
            custom_type(*convert_function)(std::string, bool)
        )
    {
        //initialization
        int last_location = chosen_begin;
        std::vector<std::vector<custom_type>> measure_info;
        custom_type temp_content;

        //temporary variables
        std::vector<custom_type> notes_info;
        std::string content;
        std::string read_content = default_value;

        for (std::vector<int>& bar_row_location : every_bar_row_location)
        {
            for (int& notes_location : bar_row_location)
            {
                content = remove_spaces(offset_thing_value(target_command, last_location, notes_location));

                if (content.size() > 0)
                {
                    read_content = "";
                    //filter characters that have meaning for target usage
                    for (char& letter : content)
                    {
                        if (!filter_function(letter)) { continue; }
                        read_content += letter;
                    }
                }
                temp_content = convert_function(read_content, content.size() > 0);
                notes_info.push_back(temp_content);
                last_location = notes_location;
            }
            measure_info.push_back(notes_info);
            notes_info.clear();
        }

        return measure_info;
    }

    //branch state is parsed seperately due to its more complex mechanism
    std::vector<std::vector<branch_state>> find_branch_state_of_each_notes()
    {
        //initialization
        int last_location = chosen_begin;
        std::vector<std::vector<branch_state>> measure_info;
        branch_state branch_state_container;

        bool is_branched = false;
        std::vector<std::string> branch_condition;
        int branch_side = -1;
        int condition_reset_times = find_phrase_in_row("#SECTION", chosen_begin, chosen_endin).size() == 0 ? -1 : 0;

        //temporary variables
        std::vector<branch_state> notes_info;
        std::string content;
        std::string read_content = "END";

        for (std::vector<int>& bar_row_location : every_bar_row_location)
        {
            for (int& notes_location : bar_row_location)
            {
                if (find_phrase_in_row("#BRANCHSTART", last_location, notes_location).size() > 0)
                {
                    is_branched = true;
                    branch_condition = split(remove_spaces(offset_thing_value("#BRANCHSTART", last_location, notes_location)), ',');
                }

                if (is_branched)
                {
                    int branch_count_n = (int)find_phrase_in_row("#N", last_location, notes_location).size();
                    int branch_count_e = (int)find_phrase_in_row("#E", last_location, notes_location).size() - (int)find_phrase_in_row("#END", last_location, notes_location).size();
                    int branch_count_m = (int)find_phrase_in_row("#M", last_location, notes_location).size() - (int)find_phrase_in_row("#MEASURE", last_location, notes_location).size();

                    ASSERT(branch_count_n + branch_count_e + branch_count_m <= 1,  "There sholdn't be higher then 1 branch command showing up in fumen simutaneously.");

                    if (branch_count_n > 0) { branch_side = 0; }
                    if (branch_count_e > 0) { branch_side = 1; }
                    if (branch_count_m > 0) { branch_side = 2; }
                }

                if (find_phrase_in_row("#BRANCHEND", last_location, notes_location).size() > 0)
                {
                    is_branched = false;
                    branch_condition.clear();
                    branch_side = -1;
                }

                if (find_phrase_in_row("#SECTION", last_location, notes_location).size() > 0) { condition_reset_times += 1; }

                branch_state_container.is_branched = is_branched;
                branch_state_container.branch_condition = branch_condition;
                branch_state_container.branch_side = branch_side;
                branch_state_container.condition_reset_times = condition_reset_times;

                notes_info.push_back(branch_state_container);
                last_location = notes_location;
            }
            measure_info.push_back(notes_info);
            notes_info.clear();
        }

        return measure_info;
    }

    int chosen_ready;
    int chosen_begin;
    int chosen_endin;

    int annotation;
    int function_usage;
    std::string current_measure;
    std::vector<int> current_measure_row_location;
    std::vector<std::vector<int>> every_bar_row_location;

    int bar_begin_location_temp;
    std::vector<int> bar_begin_location;
    std::vector<int> note_in_bar_location;
    std::vector<std::vector<int>> bar_location;

    std::vector<std::vector<branch_state>> branchstate_set;
    std::vector<std::vector<beat_measure>> beatmeasure_set;
    std::vector<std::vector<double>> scroll_set;
    std::vector<std::vector<double>> bpm_set;
    std::vector<std::vector<double>> delay_set;
    std::vector<std::vector<bool>> gogo_set;
    std::vector<std::vector<bool>> barline_set;

    std::vector<string> every_bar;
};

class TaikoFumenBranchSpecified : public TaikoFumenContext
{
public:
    TaikoFumenBranchSpecified(const char* fumen_path, int user_choice = 0, int branch_choice = -1)
        : TaikoFumenContext(fumen_path, user_choice)
    {
        every_bar.clear();
        every_bar_row_location.clear();

        branchstate_set.clear();
        beatmeasure_set.clear();
        barline_set.clear();
        scroll_set.clear();
        delay_set.clear();
        gogo_set.clear();
        bpm_set.clear();

        for (int i = 0; i < _every_bar.size(); i++)
        {
            if (_branchstate_set[i][0].branch_side != -1 && _branchstate_set[i][0].branch_side != branch_choice)
            {
                continue;
            }

            every_bar.push_back(_every_bar[i]);
            every_bar_row_location.push_back(_every_bar_row_location[i]);

            branchstate_set.push_back(_branchstate_set[i]);
            beatmeasure_set.push_back(_beatmeasure_set[i]);
            barline_set.push_back(_barline_set[i]);
            scroll_set.push_back(_scroll_set[i]);
            delay_set.push_back(_delay_set[i]);
            gogo_set.push_back(_gogo_set[i]);
            bpm_set.push_back(_bpm_set[i]);
        }

        _branchstate_set.clear();
        _beatmeasure_set.clear();
        _barline_set.clear();
        _scroll_set.clear();
        _delay_set.clear();
        _gogo_set.clear();
        _bpm_set.clear();

        current_bpm = remove_spaces(offset_thing_value("BPM:", chosen_ready, chosen_begin));
        current_offset = remove_spaces(offset_thing_value("OFFSET:", chosen_ready, chosen_begin));
        current_demostart = remove_spaces(offset_thing_value("DEMOSTART:", chosen_ready, chosen_begin));
        current_side = lower(remove_spaces(offset_thing_value("SIDE:", chosen_ready, chosen_begin)));
        current_game = remove_spaces(offset_thing_value("GAME:", chosen_ready, chosen_begin));

        search_start = any_dual ? song_difficulty[user_choice] : chosen_ready;
        search_end = any_dual ? song_difficulty[user_choice] + 1 : chosen_begin;

        current_course = lower(remove_spaces(offset_thing_value("COURSE:", search_start, search_end)));
        current_level = remove_spaces(offset_thing_value("LEVEL:", search_start, search_end));
        current_scoreinit = remove_spaces(offset_thing_value("SCOREINIT:", search_start, search_end));
        current_socrediff = remove_spaces(offset_thing_value("SCOREDIFF:", search_start, search_end));
        current_socremode = remove_spaces(offset_thing_value("SCOREMODE:", search_start, search_end));

        if (current_course.size() == 0) {
            course_sign = -1;
        }
        else if (current_course == "0" || current_course == "easy") {
            course_sign = 0;
        }
        else if (current_course == "1" || current_course == "normal") {
            course_sign = 1;
        }
        else if (current_course == "2" || current_course == "hard") {
            course_sign = 2;
        }
        else if (current_course == "3" || current_course == "oni") {
            course_sign = 3;
        }
        else if (current_course == "4" || current_course == "edit") {
            course_sign = 4;
        }
        else { throw std::runtime_error("course sign not existed"); }

        if (current_side.size() == 0 || current_side == "both" || current_side == "3") {
            side_sign = 3;
        }
        else if (current_side == "ex" || current_side == "2") {
            side_sign = 2;
        }
        else if (current_side == "normal" || current_side == "1") {
            side_sign = 1;
        }
        else { throw std::runtime_error("side sign not existed"); }

        header.TITLE = offset_thing_value_pare_ignored("TITLE:", 0, song_begin[0]);
        header.SUBTITLE = offset_thing_value_pare_ignored("SUBTITLE:", 0, song_begin[0]);
        header.WAVE = offset_thing_value_pare_ignored("WAVE:", 0, song_begin[0]);
        header.BPM = atol(current_bpm.size() > 0 ? current_bpm.c_str() : remove_spaces(offset_thing_value("BPM:", 0, song_begin[0])).c_str());
        header.OFFSET = atol(current_offset.size() > 0 ? current_offset.c_str() : remove_spaces(offset_thing_value("OFFSET:", 0, song_begin[0])).c_str());
        header.COURSE = course_sign;
        header.LEVEL = current_level.size() == 0 ? 0 : atoi(current_level.c_str());
        header.BALLOON = split_to_int(remove_spaces(offset_thing_value("BALLOON:", chosen_ready, chosen_begin)), ',');
        header.DEMOSTART = atol(current_demostart.size() > 0 ? current_demostart.c_str() : remove_spaces(offset_thing_value("DEMOSTART:", 0, song_begin[0])).c_str());
        header.SCOREINIT = current_scoreinit.size() == 0 ? -1 : atoi(current_scoreinit.c_str());
        header.SCOREDIFF = current_socrediff.size() == 0 ? -1 : atoi(current_socrediff.c_str());
        header.SCOREMODE = current_socremode.size() == 0 ? 1 : atoi(current_socremode.c_str());
        header.SONGVOL = atoi(remove_spaces(offset_thing_value("SONGVOL:", chosen_ready, chosen_begin)).c_str());
        header.SEVOL = atoi(remove_spaces(offset_thing_value("SEVOL:", chosen_ready, chosen_begin)).c_str());
        header.SIDE = side_sign;
        header.GAME = current_game.size() == 0 ? "Taiko" : current_game;

        for (int i = 0; i < every_bar.size(); i++)
        {
            std::vector<note_status> notes_status_bar;
            for (int j = 0; j < every_bar[i].size(); j++)
            {
                char notes = every_bar[i][j];
                bar_note_set location(i, j);
                if (notes != ',')
                {
                    every_passed_notes_location.push_back(location);
                }
                if (notes == '1' || notes == '2' || notes == '3' || notes == '4')
                {
                    every_actual_notes_location.push_back(location);
                }
                note_status note_status_unit = { notes, bpm_set[i][j], scroll_set[i][j], beatmeasure_set[i][j], barline_set[i][j], gogo_set[i][j], delay_set[i][j] };
                notes_status_bar.push_back(note_status_unit);
            }
            notes_status_all.push_back(notes_status_bar);
        }

        total_duration = duration();
        every_bar_result = every_bar;
    }

    long double duration(bar_note_set start = BAR_NOTE_NULL, bar_note_set end = BAR_NOTE_NULL, bool delay_result = false)
    {
        if (start.bar == -1 || start.note == -1)
        {
            start.bar = 0;
            start.note = 0;
        }

        if (end.bar == -1 || end.note == -1)
        {
            end.bar = (int)every_bar.size() - 1;
            end.note = (int)every_bar[end.bar].size() - 1;
        }

        long double overall_duration = 0.0;
        long double delay_process = 0.0;
        bool backward_sign = start > end;

        int psb = backward_sign ? end.bar : start.bar;      //process start bar
        int psn = backward_sign ? end.note : start.note;    //process start note
        int peb = backward_sign ? start.bar : end.bar;      //process end bar
        int pen = backward_sign ? start.note : end.note;    //process end note
        int tsn, ten; //temporary

        // This pair need to be seen as a ratio in some part of compute process, so they're set as double
        double beat, measure;

        // Because the #DELAY command on first note will not change any possible period between first notes and the other.
        // But the #DELAY command on first note will automatically counts in algorithm below, so pre-minus that.
        overall_duration -= delay_set[psb][psn];
        delay_process -= delay_set[psb][psn];

        // Algorithm below will compute period between 2 fumen notes,
        // It may not follow actual music theory, it will follow how it works in "TaikoJiro" instead.
        for (int j = psb; j <= peb; j++)
        {
            //the last information in the bar is ",", therefore the actual note part is equal to size of that minus 1, so do oothers' place;
            //Therfore, when length of every_bar is 1, that means there's no actual notes in this bar, only have a comma for bar end sign.
            if (every_bar[j].size() == 1)
            {
                beat = beatmeasure_set[j][0].beat;
                measure = beatmeasure_set[j][0].measure;
                overall_duration += 60 / bpm_set[j][0] * 4 * (beat / measure);
            }
            else
            {
                if (psb == peb)
                {
                    tsn = psn;
                    ten = pen;
                }
                else
                {
                    if (j == psb)
                    {
                        tsn = psn;
                        ten = (int)every_bar[j].size() - 1;  
                    }
                    else if (j == peb)
                    {
                        tsn = 0;
                        ten = pen;
                    }
                    else
                    {
                        tsn = 0;
                        ten = (int)every_bar[j].size() - 1;
                    }
                }

                
                for (int i = tsn; i < ten; i++)
                {
                    beat = beatmeasure_set[j][i].beat;
                    measure = beatmeasure_set[j][i].measure;
                    overall_duration += delay_set[j][i] + 60 / bpm_set[j][i] * 4 * (beat / measure) / (every_bar[j].size() - 1);
                    delay_process += delay_set[j][i];
                }
            }

            // Because commands (whick start with "#") can be added at the end of bar (where "," is located) as well.
            // The delay command of end of bar is not considered in algorithm above, so here's a compensation for that.
            if (psb != peb && j != peb)
            {
                overall_duration += delay_set[psb][(int)delay_set[psb].size() - 1];
                delay_process += delay_set[psb][(int)delay_set[psb].size() - 1];
            }
        }

        // compensation for delay of the end note, it's just opposite to the start note case.
        overall_duration += delay_set[peb][pen];
        delay_process += delay_set[peb][pen];

        if (delay_result) { return backward_sign ? -delay_process : delay_process; }
        return backward_sign ? -overall_duration : overall_duration;
    }

    std::vector<roll_info> get_roll_information()
    {
        std::vector<roll_info> result;
        roll_info info;
        bool in_roll = false;

        for (int i = 0;i < (int)every_bar.size();i++)
        {
            for (int j = 0;j < (int)every_bar[i].size() - 1;j++)
            {
                bar_note_set location(i, j);
                char note = every_bar[i][j];
                if (!in_roll)
                {
                    if (note == '5' || note == '6' || note == '7' || note == '9')
                    {
                        info.type = note - '0'; //ascii
                        info.start = location;
                        in_roll = true;
                    }
                }
                else
                {
                    if (note == '8' || note == '1' || note == '2' || note == '3' || note == '4')
                    {
                        info.end = location;
                        in_roll = false;
                    }
                }

                if ((info.type != -1) && (info.start != BAR_NOTE_NULL) && (info.end != BAR_NOTE_NULL))
                {
                    result.push_back(info);
                    info.type = -1;
                    info.start = BAR_NOTE_NULL;
                    info.end = BAR_NOTE_NULL;
                }
            }
        }

        return result;
    }

    std::vector<int> get_hits_of_rolls()
    {
        int balloon_count = 0;
        std::vector<int> result;
        std::vector<roll_info> roll_information = get_roll_information();

        for (roll_info& roll : roll_information)
        {
            if (roll.type == 5 || roll.type == 6) { result.push_back(-1); }
            else if (roll.type == 7 || roll.type == 9)
            {
                result.push_back(balloon_count < header.BALLOON.size() ? header.BALLOON[balloon_count] : 5);
                balloon_count += 1;
            }
            else { throw std::runtime_error("Not a valid roll sign."); }
        }

        return result;
    }

    std::array<int, 5> get_number_of_notes()
    {
        std::array<int, 5> result = { 0,0,0,0,0 };

        for (std::string& bar : every_bar)
        {
            for (char& note : bar)
            {
                switch (note)
                {
                case '1':
                    ;
                    result[1] += 1;
                    break;
                case '2':
                    result[2] += 1;
                    break;
                case '3':
                    result[3] += 1;
                    break;
                case '4':
                    result[4] += 1;
                    break;
                default:
                    break;
                }
            }
        }
        result[0] = result[1] + result[2] + result[3] + result[4];

        return result;
    }

    std::array<int, 5> get_number_of_notes_in_region(bar_note_set start = BAR_NOTE_NULL, bar_note_set end = BAR_NOTE_NULL)
    {
        if (start == BAR_NOTE_NULL)
        {
            start.bar = 0;
            start.note = 0;
        }
        if (end == BAR_NOTE_NULL)
        {
            end.bar = every_bar.size() - 1;
            end.note = every_bar.back().size() - 1;
        }

        std::array<int, 5> result = { 0,0,0,0,0 };
        bool backward_sign = start.bar > end.bar || (start.bar == end.bar && start.note > end.note);

        int psb = backward_sign ? end.bar : start.bar;      //process start bar
        int psn = backward_sign ? end.note : start.note;    //process start note
        int peb = backward_sign ? start.bar : end.bar;      //process end bar
        int pen = backward_sign ? start.note : end.note;    //process end note
        int tsn, ten; //temporary

        for (int i = psb; i <= peb; i++)
        {
            //first bar and last bar is the same, meaning that it only iterates part of a bar
            if (psb == peb)
            {
                tsn = psn;
                ten = pen;
            }
            else
            {
                //first bar to iterate, so which note is to computed first is according to defined value
                if (i == psb)
                {
                    tsn = psn;
                    ten = (int)every_bar[i].size() - 2;  //the last information in the bar is ",", therefore the actual note part is equal to size of that minus 1, so does oothers' place;
                }
                //last bar to iterate, so which note is to computed last is according to defined value
                else if (i == peb)
                {
                    tsn = 0;
                    ten = pen;
                }
                //none of above, so the entire bar would be computed
                else
                {
                    tsn = 0;
                    ten = (int)every_bar[i].size() - 2;
                }
            }

            for (int j = tsn; j <= ten; j++)
            {
                switch (every_bar[i][j])
                {
                case '1':
                    result[1] += 1;
                    break;
                case '2':
                    result[2] += 1;
                    break;
                case '3':
                    result[3] += 1;
                    break;
                case '4':
                    result[4] += 1;
                    break;
                default:
                    break;
                }
            }
        }
        result[0] = result[1] + result[2] + result[3] + result[4];

        return result;
    }

    double density(bar_note_set start = BAR_NOTE_NULL, bar_note_set end = BAR_NOTE_NULL)
    {
        if (start == BAR_NOTE_NULL) { start = every_actual_notes_location[0]; }
        if (end == BAR_NOTE_NULL) { end = every_actual_notes_location.back(); }

        int amount = get_number_of_notes_in_region(start, end)[0];
        double length = duration(start, end);

        return length == 0.0 ? std::numeric_limits<double>::max() : amount / length;
    }

    double density_in_spread_range(bar_note_set center, double period)
    {
        int back_index = 0;
        int lead_index = every_actual_notes_location.size() - 1;

        bar_note_set back = every_actual_notes_location[back_index];
        bar_note_set lead = every_actual_notes_location[lead_index];

        if (center.bar > every_bar.size()) { return -1; }
        if (center.note > every_bar[center.bar].size()) { return -1; }

        if (center < back) { return -1; }
        if (center > lead) { return -1; }

        double T = period / 2;

        for (; duration(every_actual_notes_location[back_index], center) < T; back_index++)
        for (; duration(center, every_actual_notes_location[lead_index]) < T; lead_index++)

        //while (true)
        //{
        //    if (duration(every_actual_notes_location[back_index], center) < T) { break; }
        //    back_index += 1;
        //}

        //while (true)
        //{
        //    if (duration(center, every_actual_notes_location[lead_index]) < T) { break; }
        //    lead_index -= 1;
        //}

        return get_number_of_notes_in_region(every_actual_notes_location[back_index], every_actual_notes_location[lead_index])[0] / period;
    }

    std::array<double, 2> find_extreme_density(double period)
    {
        std::array<double, 2> result;
        int notes_number = (int)every_actual_notes_location.size();
        if (notes_number <= 1)
        {
            result = { 0.0, 0.0 };
            return result;
        }

        long double total = 0;
        std::vector<long double> relative_time = { 0.0 };
        for (int i = 0;i < notes_number - 1;i++)
        {
            total += duration(every_actual_notes_location[i], every_actual_notes_location[i + 1]);
            relative_time.push_back(total);
        }

        double temp_max = 0;
        double temp_min = std::numeric_limits<double>::max();
        double density;
        for (int i = 0;i < (int)relative_time.size();i++)
        {
            if (relative_time.back() - relative_time[i] < period) { break; }
            if (relative_time[i + 1] - relative_time[i] >= period) { temp_min = 0; }

            int offset = 1;
            while (relative_time[i + offset] - relative_time[i] < period || i + offset >= (int)relative_time.size())
            {
                offset += 1;
            }
            density = (offset - 1) / period;

            if (temp_max < density) { temp_max = density; }
            if (temp_min > density) { temp_min = density; }
        }

        result = { temp_min, temp_max };
        return result;
    }

    std::array<long double, 2> find_extreme_period()
    {
        long double current;
        long double temp_max;
        long double temp_min;
        std::array<long double, 2> result;

        int notes_number = (int)every_actual_notes_location.size();
        if (notes_number <= 1)
        {
            result = { std::numeric_limits<double>::max(), std::numeric_limits<double>::max() };
            return result;
        }

        temp_max = duration(every_actual_notes_location[0], every_actual_notes_location[1]);
        temp_min = duration(every_actual_notes_location[0], every_actual_notes_location[1]);
        for (int i = 0;i < notes_number - 1;i++)
        {
            current = duration(every_actual_notes_location[i], every_actual_notes_location[i + 1]);
            if (temp_max < current) { temp_max = current; }
            if (temp_min > current) { temp_min = current; }
        }

        result = { temp_min, temp_max };
        return result;
    }

    std::vector<std::string> every_bar_result;

    fumen_header header;
    long double total_duration;

    std::vector<std::vector<note_status>> notes_status_all;

protected:
    std::vector<std::vector<branch_state>> _branchstate_set = branchstate_set;
    std::vector<std::vector<beat_measure>> _beatmeasure_set = beatmeasure_set;
    std::vector<std::vector<double>> _scroll_set = scroll_set;
    std::vector<std::vector<double>> _bpm_set = bpm_set;
    std::vector<std::vector<double>> _delay_set = delay_set;
    std::vector<std::vector<bool>> _gogo_set = gogo_set;
    std::vector<std::vector<bool>> _barline_set = barline_set;

    std::vector<std::string> _every_bar = every_bar;
    std::vector<std::vector<int>> _every_bar_row_location = every_bar_row_location;

    int search_start;
    int search_end;

    std::string current_bpm;
    std::string current_course;
    std::string current_level;
    std::string current_offset;
    std::string current_demostart;
    std::string current_scoreinit;
    std::string current_socrediff;
    std::string current_socremode;
    std::string current_side;
    std::string current_game;
    int course_sign;
    int side_sign;

    std::vector<bar_note_set> every_passed_notes_location;
    std::vector<bar_note_set> every_actual_notes_location;
};

std::string reconstructed_fumen(TaikoFumenBranchSpecified& fumen, bool display_bar_number = true)
{
    std::string remastered_fumen_context = "#START\n";

    char _note = '-';
    double _bpm = fumen.get_initial_bpm();
    double _scroll = 1.0;
    beat_measure _measure = BEAT_MEASURE_DEFAULT;
    bool _barline = true;
    bool _gogo = false;

    for (std::vector<note_status>& bar_context : fumen.notes_status_all)
    {
        for (note_status& point : bar_context)
        {
            bool command_change = _bpm != point.bpm || _scroll != point.scroll || 
                _measure != point.measure || _barline != point.barline || _gogo != point.gogo;

            if (command_change && (_note == ',' || _note == '-')) { remastered_fumen_context.pop_back(); }
            if (_bpm != point.bpm)
            {
                _bpm = point.bpm;
                remastered_fumen_context += "\n#BPMCHANGE " + std::to_string(point.bpm);
            }
            if (_scroll != point.scroll)
            {
                _scroll = point.scroll;
                remastered_fumen_context += "\nSCROLL " + std::to_string(point.scroll);
            }
            if (_measure != point.measure)
            {
                _measure = point.measure;
                remastered_fumen_context += "\n#MEASURE " + std::to_string(point.measure.beat) + "/" + std::to_string(point.measure.measure);
            }
            if (_barline != point.barline)
            {
                _barline = point.barline;
                remastered_fumen_context += point.barline ? "\n#BARLINEON" : "\n#BARLINEOFF";
            }
            if (_gogo != point.gogo)
            {
                _gogo = point.gogo;
                remastered_fumen_context += point.gogo ? "\n#GOGOSTART" : "\n#GOGOEND";
            }
            if (point.delay != 0)
            {
                remastered_fumen_context += "\n#DELAY " + std::to_string(point.delay);
            }
            remastered_fumen_context += command_change ? "\n" : "";
            remastered_fumen_context.push_back(point.note);

            _note = point.note;
        }
        remastered_fumen_context += "\n";
    }
    remastered_fumen_context += "#END";

    if (display_bar_number)
    {
        std::string remastered_fumen_with_bar;
        int bar = 0;
        for (char& c : remastered_fumen_context)
        {
            remastered_fumen_with_bar += c;
            if (c == ',')
            {
                bar += 1;
                remastered_fumen_with_bar += "//" + std::to_string(bar);
            }
        }
        return fumen.header.get_header() + remastered_fumen_with_bar;
    }

    return fumen.header.get_header() + remastered_fumen_context;
}