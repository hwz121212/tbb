/*
    Copyright (c) 2018-2020 Intel Corporation

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include "harness.h"
#include "tbb/tbb_environment.h"

#include <string>
#include <algorithm>
#include <sstream>
#include <climits>
#include <utility>
#include <vector>

const char * environment_variable_name = "TEST_VARIABLE_NAME";

// For WIN8UI applications reading and writing the environment variables
// is prohibited due to the platform limitations
#if !__TBB_WIN8UI_SUPPORT

#if _WIN32 || _WIN64
    // Environment variable length is limited by 32K on Windows systems
    const size_t large_length = 32000;
#else
    const size_t large_length = 1000000;
#endif

template<typename T>
void set_and_get_test_variable( T (*environment_variable_getter )(const char *),
                                std::pair<std::string, T> test_case ) {
    Harness::SetEnv(environment_variable_name, test_case.first.c_str());
    T result = environment_variable_getter(environment_variable_name);
    ASSERT(result == test_case.second, "Wrong Get<Type>EnvironmentVariable return value");
    Harness::SetEnv(environment_variable_name, "");
}

Harness::FastRandom rnd(12345);

struct random_character_generator {
    char operator()() {
        return rnd.get() % 128; // 127 - the last ASCII symbol
    }
};

bool alternative_env_variable_checker(const char * str, bool) {
    bool result = false;
    for (unsigned i = 0; str[i]; i++) {
        if (str[i] == '1') {
            // if we found more the one '1' character -> return false
            result = !result;
            if (!result) return false;
        }
        else if (str[i] != ' ') {
            // if we found some character other than ' ' and '1'  -> return false
            return false;
        }
    }
    return result;
}

// Suitable alternative checker for GetLongEnvVariable() was not found
// So we use here code from GetLongEnvVariable() realization
long alternative_env_variable_checker(const char * str, long) {
    char* end;
    errno=0;
    long result = std::strtol(str, &end, 10);

    // We have exceeded the range, value is negative or string is incovertable
    if (errno == ERANGE || result < 0 || end==str) {
        result = -1;
    }

    for (; *end != '\0'; end++) {
        if (!std::isspace(*end))
            result = -1;
    }
    return result;
}

template <typename T>
std::pair<std::string, T> create_random_case(size_t length){
    ASSERT(length != 0, "Requested random string cannot be empty");
    std::string rand_string(length, ' ');
    std::generate(rand_string.begin(), rand_string.end(), random_character_generator());

    T expected_result = alternative_env_variable_checker(rand_string.c_str(), T());

    return std::make_pair(rand_string, expected_result);
}

template <typename T>
void prepare_random_cases(std::vector<std::pair<std::string, T> >& cases){
    // Random cases
    size_t length = 10000;

    for(size_t i =0; i < 10; ++i) {
        cases.push_back(create_random_case<T>((rnd.get() % length) + 1 ));
    }

    // Random case with large string
    cases.push_back(create_random_case<T>(large_length));
}

std::vector<std::pair<std::string, bool> > initialize_cases( bool wrong_result ){
    std::vector<std::pair<std::string, bool> > cases;
    // Valid cases
    cases.push_back(std::make_pair("1", true));
    cases.push_back(std::make_pair(" 1 ", true));
    cases.push_back(std::make_pair("1               ", true));
    cases.push_back(std::make_pair("            1               ", true));
    cases.push_back(std::make_pair("            1", true));
    cases.push_back(std::make_pair((std::string(large_length, ' ')+'1').c_str(), true));

    // Invalid cases

    cases.push_back(std::make_pair("", wrong_result));
    cases.push_back(std::make_pair(" ", wrong_result));
    cases.push_back(std::make_pair(" 11", wrong_result));
    cases.push_back(std::make_pair("111111", wrong_result));
    cases.push_back(std::make_pair("1 1", wrong_result));
    cases.push_back(std::make_pair(" 1 abc?", wrong_result));
    cases.push_back(std::make_pair("1;", wrong_result));
    cases.push_back(std::make_pair(" d ", wrong_result));
    cases.push_back(std::make_pair("0", wrong_result));
    cases.push_back(std::make_pair("0 ", wrong_result));
    cases.push_back(std::make_pair("000000", wrong_result));
    cases.push_back(std::make_pair("01", wrong_result));
    cases.push_back(std::make_pair("00000001", wrong_result));
    cases.push_back(std::make_pair("ABCDEFG", wrong_result));
    cases.push_back(std::make_pair("2018", wrong_result));
    cases.push_back(std::make_pair("ABC_123", wrong_result));
    cases.push_back(std::make_pair("true", wrong_result));
    cases.push_back(std::make_pair(std::string(large_length, 'A').c_str(), wrong_result));

    prepare_random_cases(cases);

    return cases;
}

std::vector<std::pair<std::string, long> > initialize_cases( long wrong_result ){
    std::vector<std::pair<std::string, long> > cases;
    std::stringstream ss;
    // Valid cases
    for (long i = 0; i < 100; i++) {
        ss << i;
        cases.push_back(std::make_pair(ss.str().c_str(), i));
        ss.str("");

        ss << "     " << i << "      ";
        cases.push_back(std::make_pair(ss.str().c_str(), i));
        ss.str("");

        ss << i << "     ";
        cases.push_back(std::make_pair(ss.str().c_str(),i));
        ss.str("");

        ss << "     " << i;
        cases.push_back(std::make_pair(ss.str().c_str(),i));
        ss.str("");
    }

    ss << LONG_MAX;
    cases.push_back(std::make_pair(ss.str().c_str(),LONG_MAX));
    ss.str("");

    cases.push_back(std::make_pair((std::string(large_length, ' ')+'1').c_str(), 1L));

    // Invalid cases
    cases.push_back(std::make_pair("", wrong_result));
    cases.push_back(std::make_pair("  ", wrong_result));
    cases.push_back(std::make_pair("a", wrong_result));
    cases.push_back(std::make_pair("^&*", wrong_result));
    cases.push_back(std::make_pair("  10   e", wrong_result));
    cases.push_back(std::make_pair("a   12", wrong_result));
    cases.push_back(std::make_pair("eeeeeeeeeeeeeeeeeeeeeeeeee", wrong_result));
    cases.push_back(std::make_pair("200000000000000000000000000", wrong_result));
    cases.push_back(std::make_pair("-1", wrong_result));
    cases.push_back(std::make_pair("-100", wrong_result));
    cases.push_back(std::make_pair("-20000000000000000000000000", wrong_result));
    cases.push_back(std::make_pair("ABBDDRR", wrong_result));
    cases.push_back(std::make_pair("10  10", wrong_result));
    cases.push_back(std::make_pair("true", wrong_result));
    cases.push_back(std::make_pair("false", wrong_result));
    cases.push_back(std::make_pair("1A", wrong_result));
    cases.push_back(std::make_pair("_123", wrong_result));
    cases.push_back(std::make_pair(std::string(large_length, 'A').c_str(), wrong_result));

    // Prepare string with LONG_MAX + 1 value
    ss << LONG_MAX / 10 << (LONG_MAX % 10 + 1);
    cases.push_back(std::make_pair(ss.str().c_str(),-1));
    ss.str("");

    prepare_random_cases(cases);
    return cases;
}

template <typename T>
void test_environment_variable( T (*environment_variables_handler )(const char *), T wrong_result ) {
    ASSERT(environment_variables_handler (environment_variable_name) == wrong_result,
           "Tested environment variable should not be defined in the beginning of the test");

    // Every pair is a test case:
    // pair.first -> value of environment variable
    // pair.second -> expected result
    std::vector< std::pair<std::string, T> > cases = initialize_cases(wrong_result);

    for (size_t i = 0; i != cases.size(); i++) {
        set_and_get_test_variable(environment_variables_handler, cases[i]);
    }
}


#else // __TBB_WIN8UI_SUPPORT

template <typename T>
void test_environment_variable(T (*environment_variables_handler )(const char *), T wrong_result) {
    for(size_t i = 0; i < 100; ++i) {
        ASSERT(environment_variables_handler(environment_variable_name) == wrong_result,
               "Get<Type>EnvironmentVariable should always return false for UWP applications");
    }
}

#endif // __TBB_WIN8UI_SUPPORT

int TestMain() {
    test_environment_variable(tbb::internal::GetBoolEnvironmentVariable, false);
    test_environment_variable(tbb::internal::GetIntegralEnvironmentVariable, -1L);
    return Harness::Done;
}
