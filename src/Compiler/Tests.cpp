#include "Parser.h"
#include <iostream>
#include <fstream>

std::vector<std::pair<std::string, std::string>> LoadGoldenTests(std::string filename)
{
    std::ifstream file(filename);
    std::vector<std::pair<std::string, std::string>> ret;

    std::string temp;
    while (true)
    {
        if (!std::getline(file, temp)) break;
        int inputLC = std::stoi(temp);
        int outputLC = std::stoi(temp.substr(temp.find(',') + 1));

        std::string inp = ""; std::string out = "";

        for (int i = 0; i < inputLC; i++)
        {
            if (!std::getline(file, temp)) break;
            inp += temp + "\n";
        }
        for (int i = 0; i < outputLC; i++)
        {
            if (!std::getline(file, temp)) break;
            out += temp + "\n";
        }

        ret.push_back({ inp, out });
    }

    return ret;
}

void WriteGoldenTests(std::string filename, std::vector<std::pair<std::string, std::string>> tests)
{
    std::ofstream file(filename);
    for (auto& i : tests)
    {
        Assert(i.first.size() > 0, "Input cannot have size 0.");
        Assert(i.second.size() > 0, "Output cannot have size 0.");
        Assert(i.first[i.first.size() - 1] == '\n', "Input must end in a newline. (" + i.first.substr(i.first.size() - 1) + ")");
        Assert(i.second[i.second.size() - 1] == '\n', "Output must end in a newline. (" + i.second.substr(i.second.size() - 1) + ")");

        // both input and output should have a trailing newline, so the math works out
        int inpLC = 0; for (char c : i.first) { if (c == '\n') inpLC++; }
        int outLC = 0; for (char c : i.second) { if (c == '\n') outLC++; }

        file << inpLC << "," << outLC << "\n" << i.first << i.second;
    }
}

std::string RunTest(std::string in)
{
    std::string ret = "";

    std::vector<Token> tokens = Tokenize(in);
    for (Token& i : tokens)
    {
         ret += i.value + "\n";
    }
    ParsingContext pc = { { { "func1", LambdaType{ { AtomicType::Integer }, { AtomicType::String } } } } };
    Statement s = SingleStatement{ { LiteralExpression{ AtomicType::Error, { tokens, 0 } } } }; int n = 0;
    ret += (ParseStatement({tokens, 0}, pc, s, n) ? "Parsing worked!" : "Parsing failed.") + std::string("\n");
    ret += StatementToString(s) + "\n";
    ret += std::to_string(n) + " tokens parsed.\n";
    ret += "There were " + std::to_string(pc.errors.size()) + " errors.\n";
    for (auto& i : pc.errors)
    {
        ret += "Error (" + std::to_string(i.pos.line) + "," + std::to_string(i.pos.column) + "): " + i.msg + "\n";
        int pos = 0;
        for (int j = 1; j < i.pos.line; j++) j = 1 + in.find('\n', pos);
        for (int j = 1; j < i.pos.column; j++) ret += ' ';
        ret += "v\n" + in.substr(pos, in.find('\n', pos) - pos) + "\n\n";
    }

    return ret;
 }

void RunAllTests(std::string filename)
{
    std::vector<std::pair<std::string, std::string>> tests = LoadGoldenTests(filename);

    std::vector<std::pair<int,std::string>> failedTests;
    for (int i = 0; i < tests.size(); i++)
    {
        std::string out = RunTest(tests[i].first);
        if (out != tests[i].second)
        {
            failedTests.push_back({ i, out });
        }
    }

    std::cout << "Testing complete. " << tests.size() - failedTests.size() << " tests passed. " << failedTests.size() << " tests failed.";
    if (failedTests.size() == 0)
    {
        std::cout << " Congratulations." << std::endl;
    }
    else
    {
        std::cout << "\nWould you like to see the failing tests? (Y/n) ";
        std::string temp; std::cin >> temp;
        if (temp.size() >= 1 && (temp[0] == 'Y' || temp[0] == 'y'))
        {
            bool writeToGolden = false;

            for (auto& i : failedTests)
            {
                std::cout << "\n#################### FAILED TEST: " << i.first << " ####################\n\nInput:\n"
                    << tests[i.first].first << "\n\nDesired Output:\n" << tests[i.first].second << "\n\nActual Output:\n" << i.second << "\n\n";

                std::cout << "Would you like to update the golden? (Y/n) ";
                std::cin >> temp;
                if (temp.size() >= 1 && (temp[0] == 'Y' || temp[0] == 'y'))
                {
                    tests[i.first].second = i.second;
                    writeToGolden = true;
                    std::cout << "Updated golden!\n";
                }
            }

            if (writeToGolden) WriteGoldenTests(filename, tests);
        }
    }
}

// #define RUN_TESTS
#ifdef RUN_TESTS

int main()
{
    RunAllTests("golden_tests.txt");

    return 0;
}

#endif

#ifndef RUN_TESTS
#ifdef CREATE_TESTS

int main()
{
    auto tests = LoadGoldenTests("golden_tests.txt");
    while (true)
    {
        std::cout << "\nEnter test input. Type 'exit' to save and exit: ";
        std::string temp; std::getline(std::cin, temp);
        if (temp == "exit") break;
        std::string input = temp + "\n";
        std::string out = RunTest(input);
        std::cout << out << "\n\nDo you wish to add this as a golden test? (Y/n) ";
        std::getline(std::cin, temp);
        if (temp.size() >= 1 && (temp[0] == 'Y' || temp[0] == 'y'))
        {
            tests.push_back({ input, out });
        }
    }

    std::cout << "Would you like to save these new tests? (Y/n) ";
    std::string temp; std::getline(std::cin, temp);
    if (temp.size() >= 1 && (temp[0] == 'Y' || temp[0] == 'y'))
    {
        WriteGoldenTests("golden_tests.txt", tests);
    }

    return 0;
}

#endif
#endif

