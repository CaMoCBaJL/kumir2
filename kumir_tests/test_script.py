import os
import sys
import subprocess

#constants
class CONSOLE_BG_COLORS:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKCYAN = '\033[96m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

class TEST_RESULT_STATE:
    COMPLETED = 1
    MISSING_EXPECTATION = 2
    COMPILER_ERROR_HAPPEND = 3
    FAILED = -1
    NONE = 0

COMPILER_ERROR_LABEL = "ERROR!"

EXPECTATION_FILE_EXTENTION = ".exp"
SOURCE_FILE_EXTENTION = ".kum"

CONTROL_CHARACTERS = ["\n", "\r", "\t", " "]
BYTES_TO_READ_COUNT = 1024
CHAR_THRESHOLD = 0.3
TEXT_CHARACTERS = ''.join(
    [chr(code) for code in range(32, 127)] +
    list('\b\f\n\r\t')
)
BINARY_CHAR_EXAMPLE = '\x00'

TESTS_FOLDER_NAME = "Tests"

ARGS = {"help": ["-h", "--help"],
        "translator": ["-tr", "--translator"],
        "output": ["-o", "--output"],
        "duplicate": ["-d", "--duplicate"],
        "skip-successfull": ["-ss", "--skip-successfull"],
        "skip-failed": ["-sf", "--skip-failed"],
        "skip-without-expectation": ["-swe", "--skip-without-expectation"],
        "skip-with-compiler-error": ["-sce", "--skip-with-compiler-error"],
        "brief": ["-b", "--brief"],
        "path-to-tests": ["-t", "path-to-tests"]
        }

ERRORS = {
    "wrong input": "Wrong args input - you should type path to file after -c or -o flags!",
    "file not exist": "File doesn't exist!",
    "wrong file extention": "File extention for translator is not correct! It should be a bin-file.",
    "no path to tests": "Wrong args input - you should type path to tests folder after -t flag!"
}

#data structure to store test results
class TestResult:

    __text_color = CONSOLE_BG_COLORS.OKGREEN
    __header_text = ""
    SKIPPED_TEST_TYPES = [TEST_RESULT_STATE.NONE]

    def __init__(self, name: str, source: str, expectation: str, result: str):
        self.name = name
        self.source_file_name = source
        self.expectation_file_name = expectation
        self.resultFileName = result
        self.state = TEST_RESULT_STATE.NONE

    def __str__(self):
        self.__setup_output()
        additional_test_data = f'''
        Test expectations: {self.expectation_file_name}
        Test results: {self.resultFileName}. 
        To get more detail info about comparison results, print:
        vimdiff {self.source_file_name} {self.expectation_file_name} {self.resultFileName}
        or vimdiff {self.expectation_file_name} {self.resultFileName}{CONSOLE_BG_COLORS.ENDC}
        '''
        return f'''{self.__text_color}
        Test {self.name}.
        {self.__header_text}
        Sources for the test: {self.source_file_name}
        {additional_test_data if self.state is TEST_RESULT_STATE.COMPLETED or self.state is TEST_RESULT_STATE.FAILED else ""}
        '''

    def __setup_output(self):
        if self.state == TEST_RESULT_STATE.COMPLETED:
            self.__text_color = CONSOLE_BG_COLORS.OKGREEN
            self.__header_text = "Congratulations! Test completed successfully!"
        elif self.state == TEST_RESULT_STATE.MISSING_EXPECTATION:
            self.__text_color = CONSOLE_BG_COLORS.OKCYAN
            self.__header_text = "Oh! Test didn't complete: no expectation"
        elif self.state == TEST_RESULT_STATE.COMPILER_ERROR_HAPPEND:
            self.__text_color = CONSOLE_BG_COLORS.WARNING
            self.__header_text = "Oh! Test didn't complete: compiler error"
        elif self.state == TEST_RESULT_STATE.FAILED:
            self.__text_color = CONSOLE_BG_COLORS.FAIL
            self.__header_text = "Sorry, but test failed..."

class TestSection:

    def __init__(self, section_name) -> None:
        self.name = section_name
        self.test_results = []
    
    def __str__(self) -> str:
        columns, _ = os.get_terminal_size()
        return f"""
        {CONSOLE_BG_COLORS.WARNING + "-" *columns + CONSOLE_BG_COLORS.ENDC}
        Test section {self.name} starts here:
        {os.linesep.join(list(map(lambda test_result: str(test_result), self.test_results)))}
        End of {self.name} section tests
        {CONSOLE_BG_COLORS.WARNING + "-" *columns + CONSOLE_BG_COLORS.ENDC}
        """

#functions
def remove_control_characters(data_array):
    result = []
    for i in data_array:
        for cc in CONTROL_CHARACTERS:
            i = i.replace(cc, "")

        if i:
            result.append(i)

    return result

def get_file_data(filename):
    file = open(filename, "r")
    result = file.readlines()
    file.close()
    return result

def has_errors(text):
    return COMPILER_ERROR_LABEL in text

def process_sources(source_filename, path_to_translator):
    path, _ = os.path.splitext(source_filename)
    result_filename = path + ".kumir.c"

    #call kumir2-arduino with params: --out="path_to_cwd/results/test_name.kumir.c" -s ./test_name.kum
    popen = subprocess.Popen([path_to_translator,
                              f'--out={result_filename}',
                              '-s',
                              source_filename],
                             stdout=subprocess.PIPE)
    popen.wait()
    popen.stdout.read()

    return result_filename

def compare_data(expected_data, processed_data) -> TEST_RESULT_STATE:
    result_without_kumir_ref = remove_control_characters(processed_data[2:])
    expected_data = remove_control_characters(expected_data)

    if (has_errors(result_without_kumir_ref)):
        return TEST_RESULT_STATE.COMPILER_ERROR_HAPPEND

    for i in range(len(result_without_kumir_ref)):
        if result_without_kumir_ref[i] != expected_data[i]:
            return TEST_RESULT_STATE.FAILED

    return TEST_RESULT_STATE.COMPLETED

def get_test_result_type_counters(test_sections):
    counters = {
        TEST_RESULT_STATE.COMPLETED: 0,
        TEST_RESULT_STATE.FAILED: 0,
        TEST_RESULT_STATE.COMPILER_ERROR_HAPPEND: 0,
        TEST_RESULT_STATE.MISSING_EXPECTATION: 0,
    }

    for test_section in test_sections:
        for test_result in test_section.test_results:
            counters[test_result.state] += 1


    return counters

#log comparison results to file
def log_test_results(logs_filename, data_to_log: TestSection, log_to_console):
    result_type_counters = get_test_result_type_counters(data_to_log)
    completed_tests_count, failed_tests_count, compiler_error_happend_tests_count, missing_expectations_tests_count = \
    [result_type_counters[k] for k in result_type_counters]

    log_data = []
    for test_section in data_to_log:
        test_section.test_results = list(filter(lambda test_result: test_result.state not in TestResult.SKIPPED_TEST_TYPES, test_section.test_results))
        log_data.append(test_section)

    log_data_strings = list(map(lambda x: str(x), log_data))
    log_data_strings.insert(0, f'''
    There were found: {len(log_data)} tests.
    {CONSOLE_BG_COLORS.OKGREEN} Completed: {completed_tests_count} {CONSOLE_BG_COLORS.ENDC}
    {CONSOLE_BG_COLORS.FAIL} Failed: {failed_tests_count} {CONSOLE_BG_COLORS.ENDC}
    {CONSOLE_BG_COLORS.WARNING} With compiler error happend: {compiler_error_happend_tests_count} {CONSOLE_BG_COLORS.ENDC}
    {CONSOLE_BG_COLORS.OKCYAN} Missed expectation file: {missing_expectations_tests_count} {CONSOLE_BG_COLORS.ENDC}''')

    if not os.path.exists(logs_filename):
        open(logs_filename, "a").close()

    log_file = open(logs_filename, "a")
    log_file.writelines(log_data_strings)
    log_file.close()

    if log_to_console:
        print(f"{os.linesep}".join(log_data_strings))

def is_binary_file(filename):
    file_stream = open(filename, 'rb')
    file_content = file_stream.read(BYTES_TO_READ_COUNT)
    file_stream.close()
    
    if not len(file_content):
        #file is empty, nothing to read
        return False
    
    if ord(BINARY_CHAR_EXAMPLE) in file_content:
        #file contains binary symbols
        return True
    
    binary_chars = file_content.translate(TEXT_CHARACTERS)
    return float(len(binary_chars)) / len(file_content) > CHAR_THRESHOLD

def validate_file_name(filename: str, is_log_file: bool):   
    if not os.path.exists(filename):
        print(filename + ERRORS.get("file not exist"))
        sys.exit(2)
    if not os.path.isfile(filename):
        print(ERRORS.get("wrong input"))
        sys.exit(2)
    if not is_log_file and not is_binary_file(filename):
        print(ERRORS.get("wrong file extention"))
        sys.exit(2)
        
def show_help():
    print(""" kumir2-arduino tester.
    Description:
    Approach of this app is to debug the work of kumir2 to arduino translator. It uses compiled translator's instance, pre-builded locally on PC.
    To start the work you should input path to compiler and path to logs file.
    Flags:
    [-h] [--help] - show help.
    
    [-t] [--translator] ["the path to pre-builded kumir2 to arduino translator instance"] - show app what translator instance to use.
    
    [-o] [--output] ["the path to log file"] - show app where to store test logs.
    
    [-d] [--duplicate] - duplicate output to console.

    [-ss] [--skip-successfull] - skip open log info about succesfully completed tests.

    [-sf] [--skip-failed] - skip open log info about failed tests.

    [-swe] [--skip-without-expectation] - skip open log info about tests for which the file with expectations was not found.

    [-sce] [--skip-with-compiler-error] - skip open log info about tests ended with compiler error.

    [-b] [--brief] - skip open log info about all tests.
    """)

def process_args():
    result = ["", "", False, ""]
    if ARGS["help"][0] in sys.argv or ARGS["help"][1] in sys.argv:
        show_help()
        sys.exit(2)

    if ARGS["skip-successfull"][0] in sys.argv or ARGS["skip-successfull"][1] in sys.argv: 
        TestResult.SKIPPED_TEST_TYPES.append(TEST_RESULT_STATE.COMPLETED)
    
    if ARGS["skip-without-expectation"][0] in sys.argv or ARGS["skip-without-expectation"][1] in sys.argv: 
        TestResult.SKIPPED_TEST_TYPES.append(TEST_RESULT_STATE.MISSING_EXPECTATION)

    if ARGS["skip-with-compiler-error"][0] in sys.argv or ARGS["skip-with-compiler-error"][1] in sys.argv: 
        TestResult.SKIPPED_TEST_TYPES.append(TEST_RESULT_STATE.COMPILER_ERROR_HAPPEND)

    if ARGS["skip-failed"][0] in sys.argv or ARGS["skip-failed"][1] in sys.argv: 
        TestResult.SKIPPED_TEST_TYPES.append(TEST_RESULT_STATE.FAILED)

    if ARGS["brief"][0] in sys.argv or ARGS["brief"][1] in sys.argv: 
        TestResult.SKIPPED_TEST_TYPES = [
            TEST_RESULT_STATE.COMPILER_ERROR_HAPPEND, 
            TEST_RESULT_STATE.COMPLETED, 
            TEST_RESULT_STATE.FAILED,
            TEST_RESULT_STATE.MISSING_EXPECTATION
            ]

    args = sys.argv[1:]
    for i in range(1, len(args)):
        if args[i - 1] in ARGS["translator"] or args[i - 1] in ARGS["output"] or args[i - 1] in ARGS["path-to-tests"]:
            if (os.path.isfile(args[i])):
                validate_file_name(args[i], False if args[i - 1] in ARGS["translator"] else True)

            if args[i - 1] in ARGS["translator"]:
                result[0] = os.path.abspath(args[i])
            elif args[i - 1] in ARGS["output"]: 
                result[1] = os.path.abspath(args[i])
            elif args[i - 1] in ARGS["path-to-tests"]:
                result[3] = os.path.abspath(args[i])
        elif args[i-1] in ARGS["duplicate"]:              
            result[2] = True
            
    return result

def get_files_with_absolute_paths(folder_name):
    path_to_folder = os.path.join(os.getcwd(), folder_name)
    files = list(map(lambda x: os.path.join(path_to_folder, x), os.listdir(path=path_to_folder)))
    files.sort()

    return files

def get_source_and_expectation(dir_files):
    sources = []
    expectations = []
    for file in dir_files:
        if (os.path.isfile(file)):
            ext = os.path.splitext(file)[1]
            if (ext == EXPECTATION_FILE_EXTENTION):
                expectations.append(file)
            elif (ext == SOURCE_FILE_EXTENTION):
                sources.append(file)
    
    return [sources, expectations]

def get_folder_contents_full_paths(path_to_folder):
    return list(
        map(
            lambda x: os.path.join(os.sep, path_to_folder, x),
            os.listdir(path_to_folder)
            )
        )

def calculate_test_sections(path_to_tests_folder):
    result = []
    test_folder_paths = get_folder_contents_full_paths(path_to_tests_folder)
    for test_folder_path in test_folder_paths:
        if os.path.isfile(test_folder_path):
            continue

        result.append(TestSection(test_folder_path.split(os.sep)[-1]))
        test_paths = get_folder_contents_full_paths(test_folder_path)
        for test_dir_path in test_paths:
            source_files, expectation_files = get_source_and_expectation(get_folder_contents_full_paths(test_dir_path))
            if os.path.isfile(test_dir_path) or len(expectation_files) > 1 or len(source_files) > 1:
                continue
            
            result[-1].test_results.append(TestResult(
                test_dir_path.split(os.sep)[-1],
                source_files[0],
                "", ""
                )
            )

            if len(expectation_files) == 1:
                result[-1].test_results[-1].expectation_file_name  = expectation_files[0]
                result[-1].test_results[-1].resultFileName = process_sources(source_files[0], args_data[0])

                result_data = get_file_data(result[-1].test_results[-1].resultFileName)
                expected_data = get_file_data(result[-1].test_results[-1].expectation_file_name)

                result[-1].test_results[-1].state = compare_data(expected_data, result_data)
            else: 
                result[-1].test_results[-1].state = TEST_RESULT_STATE.MISSING_EXPECTATION

    return result

if __name__=="__main__":
    args_data = process_args()

    if not args_data[3]:
        print("Didn't find any test to execute. Shutting down.")       
        sys.exit()

    test_results = calculate_test_sections(args_data[3])

    log_test_results(args_data[1], test_results, args_data[2])
