#include "fumen_container.hpp"

const char* roll_type_name(int sign)
{
    switch (sign)
    {
    case 5: return "Small Drum Roll";
    case 6: return "Big Drum Roll";
    case 7: return "Balloon / Burst";
    case 9: return "Yam / Kusudama";
    default: return "-";
    }
}

const char* branch_type_name(int sign)
{
    switch (sign)
    {
    case 0: return "Normal";
    case 1: return "Expert";
    case 2: return "Master";
    default: return "-";
    }
}

enum class STAGE { BASIC, BRANCH, FINAL, EXIT };

STAGE StageBasic(const char* input_fumen, int& user_input_fumen)
{
    TaikoFumenBasic fumen_basic(input_fumen);

    system("cls");
    cout << "Input file: " << input_fumen << "\n\n";
    cout << "Fumen Found:" << endl;
    cout << "-------------------------------------------------" << endl;
    int index = 0;
    for (fumen_status& fs : fumen_basic.fuman_classfication)
    {
        cout << "Fumen [" << index << "]";
        cout << "\t" << fs.difficulty;
        cout << "\t" << fs.level;
        cout << "\t" << fs.side;
        cout << "\t" << fs.dual;
        cout << "\t" << fs.operation;
        index++;
        cout << endl;
    }
    cout << "-------------------------------------------------" << endl;

    if (fumen_basic.fuman_classfication.size() > 1)
    {
        cout << "\nPlease enter fumen choice: ";
        cin >> user_input_fumen;
        if (user_input_fumen < 0 || user_input_fumen >= (int)fumen_basic.fuman_classfication.size())
        {
            cout << "\nInvalid fumen choice" << endl;
            system("pause");
            return STAGE::EXIT;
        }
    }
    else
    {
        user_input_fumen = 0;
    }
    return STAGE::BRANCH;
}

STAGE StageBranch(const char* input_fumen, int user_input_fumen, int& user_input_branch)
{
    TaikoFumenContext fumen_context(input_fumen, user_input_fumen);

    system("cls");
    cout << ">>> Currently chosen fumen: " << user_input_fumen << endl;
    cout << "----------------------" << endl;
    fumen_context.fuman_classfication[user_input_fumen].represent();
    cout << "----------------------" << endl;

    if (fumen_context.is_branch_exist)
    {
        cout << "\nBranch structure was found in fumen, enter branch choice" << endl;
        cout << "- 0 is for Normal" << endl;
        cout << "- 1 is for Expert" << endl;
        cout << "- 2 is for Master" << endl;
        cout << "> ";
        cin >> user_input_branch;
        if (user_input_branch != 0 && user_input_branch != 1 && user_input_branch != 2)
        {
            cout << "\nInvalid branch choice" << endl;
            system("pause");
            return STAGE::EXIT;
        }
    }
    else
    {
        user_input_branch = -1;
    }
    return STAGE::FINAL;
}

STAGE StageFinal(const char* input_fumen, int user_input_fumen, int user_input_branch)
{
    TaikoFumenBranchSpecified fumen_final(input_fumen, user_input_fumen, user_input_branch);

    system("cls");
    cout << ">>> Currently chosen fumen: " << user_input_fumen << endl;
    cout << ">>> Currently branch state: " << branch_type_name(user_input_branch) << endl;
    cout << "-----------------------------" << endl;
    cout << "h : Show header of Fumen " << endl;
    cout << "t : Find Duration of Fumen" << endl;
    cout << "d : Find Density of Fumen" << endl;
    cout << "e : Find Extreme Instant Density of Fumen" << endl;
    cout << "n : Show number of notes" << endl;
    cout << "r : Show status of rolls" << endl;
    cout << "f : Show every bar" << endl;
    cout << "c : Reconstruct overall fumen context" << endl;
    cout << "> ";
    char command;
    cin >> command;
    cout << endl;

    std::array<long double, 2> density;
    std::array<int, 5> result;
    std::vector<roll_info> rolls;
    long double roll_period = 0.0;
    long double total_roll_period = 0.0;
    int i = 0;
    switch (command)
    {
    case 'h':
        fumen_final.header.represent();
        break;
    case 't':
        cout << "Fumen period: " << fumen_final.duration() << " s" << endl;
        break;
    case 'd':
        cout << "Fumen density: " << fumen_final.density() << " note/s" << endl;
        break;
    case 'e':
        density = fumen_final.find_extreme_period();
        cout << "Max density: " << 1 / density[0] << " note/s" << endl;
        cout << "Min density: " << 1 / density[1] << " note/s" << endl;
        break;
    case 'n':
        result = fumen_final.get_number_of_notes_in_region();
        cout << "Total:\t" << result[0] << endl;
        cout << "L_Red:\t" << result[1] << endl;
        cout << "L_Blue:\t" << result[2] << endl;
        cout << "B_Red:\t" << result[3] << endl;
        cout << "B_Blue:\t" << result[4] << endl;
        break;
    case 'r':
        rolls = fumen_final.get_roll_information();
        cout << "roll type\t\t|     start  \t|      end  \t|  duration" << endl;
        cout << "--------------------------------------------------------------------" << endl;
        for (roll_info& each_roll : rolls)
        {
            roll_period = fumen_final.duration(each_roll.start, each_roll.end);
            total_roll_period += roll_period;
            cout << roll_type_name(each_roll.type);
            cout << " \t|  ";
            each_roll.start.simple_repersent();
            cout << " \t|  ";
            each_roll.end.simple_repersent();
            cout << " \t|  ";
            cout << roll_period;
            cout << endl;
        }
        cout << "--------------------------------------------------------------------" << endl;
        cout << "Total Length Of All Rolls: " << total_roll_period << " s" << endl;
        break;
    case 'f':
        for (std::string& bar : fumen_final.every_bar_result)
        {
            i += 1;
            cout << "bar " << i << "\t" << bar << endl;
        }
        break;
    case 'c':
        cout << reconstructed_fumen(fumen_final) << endl;
        break;
    default:
        break;
    }

    cout << "\n(1) Back to basic stage";
    cout << "\n(2) Back to branch stage";
    cout << "\n(3) Back to final stage";
    cout << "\n(X) Exit";
    cout << "\n> ";
    char next_step;
    cin >> next_step;

    switch (next_step)
    {
    case '1': return STAGE::BASIC;
    case '2': return STAGE::BRANCH;
    case '3': return STAGE::FINAL;
    default: return STAGE::EXIT;
    }
}

char* input_fumen;

int main(int argc, char* argv[])
{
    if (argc == 2)
    {
        input_fumen = argv[1];
    }
    else if (argc == 1)
    {
        cout << "Drag a fumen file (.tja) on this executable to continue correctly." << endl;
        cin.get();
        return 1;
    }
    else
    {
        cout << "Not a valid input" << endl;
        cout << "(Press Any Key To Exit...)" << endl;
        cin.get();
        return 1;
    }

    STAGE currentStage = STAGE::BASIC;
    int user_input_fumen = 0;
    int user_input_branch = -1;

    while (currentStage != STAGE::EXIT)
    {
        switch (currentStage)
        {
        case STAGE::BASIC:
            currentStage = StageBasic(input_fumen, user_input_fumen);
            break;
        case STAGE::BRANCH:
            currentStage = StageBranch(input_fumen, user_input_fumen, user_input_branch);
            break;
        case STAGE::FINAL:
            currentStage = StageFinal(input_fumen, user_input_fumen, user_input_branch);
            break;
        default:
            currentStage = STAGE::EXIT;
            break;
        }
    }
    return 0;
}