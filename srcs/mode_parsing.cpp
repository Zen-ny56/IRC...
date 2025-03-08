#include <iostream>
#include <sstream>
#include <vector>
#include <string>

#define GREEN "\033[32m"
#define RED "\033[31m"
#define RESET "\033[0m"

void testModeParsing(const std::string& message, const std::vector<std::string>& expectedModes, const std::vector<std::string>& expectedParams)
{
    std::istringstream iss(message);
    std::string command, channelName, token;
    std::vector<std::string> modeTokens;
    std::vector<std::string> params;

    // Read command
    iss >> command;

    // Read channel name
    if (!(iss >> channelName)) {
        std::cerr << "Error: No channel name provided.\n";
        return;
    }

    // Temporary storage for modes
    std::string currentModes;
    bool expectingParam = false;

    // Parse the modes first
    while (iss >> token) {
        if (!token.empty() && (token[0] == '+' || token[0] == '-')) {
            // Push previous modes (if any) before starting new mode group
            if (!currentModes.empty()) {
                modeTokens.push_back(currentModes);
                currentModes.clear();
            }

            // Now process the new mode
            char modeType = token[0];
            for (size_t i = 1; i < token.size(); ++i) {
                std::string mode = std::string(1, modeType) + token[i];
                modeTokens.push_back(mode);
                if (token[i] == 'k' || token[i] == 'l') {
                    expectingParam = true;  // Set flag when we expect a parameter
                }
            }
        } else if (expectingParam) {
            // If we're expecting a parameter (after 'k' or 'l' mode), consume the next token
            params.push_back(token);
            expectingParam = false; // Reset flag after consuming the parameter
        } else {
            // If it's not a mode or expected parameter, push it into params directly
            params.push_back(token);
        }
    }

    // Display results
    std::cout << "Testing: " << message << std::endl;
    
    bool modesCorrect = (modeTokens == expectedModes);
    bool paramsCorrect = (params == expectedParams);

    std::cout << "Expected Modes: ";
    std::cout << (modesCorrect ? GREEN : RED);
    for (size_t i = 0; i < expectedModes.size(); ++i) {
        std::cout << expectedModes[i] << " ";
    }
    std::cout << RESET << std::endl;

    std::cout << "Parsed Modes:   ";
    std::cout << (modesCorrect ? GREEN : RED);
    for (size_t i = 0; i < modeTokens.size(); ++i) {
        std::cout << modeTokens[i] << " ";
    }
    std::cout << RESET << std::endl;

    std::cout << "Expected Params: ";
    std::cout << (paramsCorrect ? GREEN : RED);
    for (size_t i = 0; i < expectedParams.size(); ++i) {
        std::cout << expectedParams[i] << " ";
    }
    std::cout << RESET << std::endl;

    std::cout << "Parsed Params:   ";
    std::cout << (paramsCorrect ? GREEN : RED);
    for (size_t i = 0; i < params.size(); ++i) {
        std::cout << params[i] << " ";
    }
    std::cout << RESET << std::endl;

    if (modesCorrect && paramsCorrect)
        std::cout << GREEN << "✅ Test Passed!" << RESET << std::endl;
    else
        std::cout << RED << "❌ Test Failed!" << RESET << std::endl;

    std::cout << "-----------------------------\n";
}

int main() {
    std::cout << "AAAA" << std::endl;

    std::vector<std::string> test1_modes;
    test1_modes.push_back("+k");
    std::vector<std::string> test1_params;
    test1_params.push_back("secretpass");
    testModeParsing("/mode #chatroom +k secretpass", test1_modes, test1_params);

    std::vector<std::string> test2_modes;
    test2_modes.push_back("+i");
    test2_modes.push_back("+t");
    test2_modes.push_back("+l");
    test2_modes.push_back("+k");
    std::vector<std::string> test2_params;
    test2_params.push_back("secretpass");
    test2_params.push_back("20");
    testModeParsing("/mode #chatroom +itlk secretpass 20", test2_modes, test2_params);

    std::vector<std::string> test3_modes;
    test3_modes.push_back("+i");
    test3_modes.push_back("-t");
    test3_modes.push_back("+k");
    test3_modes.push_back("-l");
    std::vector<std::string> test3_params;
    test3_params.push_back("newpass");
    testModeParsing("/mode #chatroom +i -t +k newpass -l", test3_modes, test3_params);

    std::vector<std::string> test4_modes;
    test4_modes.push_back("+i");
    test4_modes.push_back("-t");
    test4_modes.push_back("+k");
    test4_modes.push_back("-l");
    std::vector<std::string> test4_params;
    // No parameters in this test
    testModeParsing("/mode #chatroom +i -t +k -l", test4_modes, test4_params);

    std::vector<std::string> test5_modes;
    test5_modes.push_back("+k");
    test5_modes.push_back("-l");
    std::vector<std::string> test5_params;
    test5_params.push_back("secretpass");
    test5_params.push_back("30");
    testModeParsing("/mode #chatroom +k secretpass -l 30", test5_modes, test5_params);

    std::vector<std::string> test6_modes;
    test6_modes.push_back("+i");
    test6_modes.push_back("-t");
    test6_modes.push_back("+k");
    test6_modes.push_back("-l");
    std::vector<std::string> test6_params;
    test6_params.push_back("newpass");
    testModeParsing("/mode #chatroom +i -t +k newpass -l", test6_modes, test6_params);

    return 0;
}

