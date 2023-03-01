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

CONTROL_CHARACTERS = ["\n", "\r", "\t", " "]
BYTES_TO_READ_COUNT = 1024
CHAR_THRESHOLD = 0.3
TEXT_CHARACTERS = ''.join(
    [chr(code) for code in range(32, 127)] +
    list('\b\f\n\r\t')
)
BINARY_CHAR_EXAMPLE = '\x00'

SOURCES_FOLDER_NAME = "Sources"
EXPECTATIONS_FOLDER_NAME = "Expectations"
RESULTS_FOLDER_NAME = "Results"

ARGS = {"help": ["-h", "--help"],
        "translator": ["-t", "--translator"],
        "output": ["-o", "--output"],
        "duplicate": ["-d", "--duplicate"],
        "skip-successfull": ["-ss", "--skip-successfull"],
        "skip-failed": ["-sf", "--skip-failed"],
        "skip-without-expectation": ["-swe", "--skip-without-expectation"],
        "skip-with-compiler-error": ["-sce", "--skip-with-compiler-error"]
        }

ERRORS = {
    "wrong input": "Wrong args input - you should type path to file after -c or -o flags!",
    "file not exist": "File doesn't exist!",
    "wrong file extention": "File extention for translator is not correct! It should be a bin-file."
}

#data structure to store test results
class TestResult:
    __text_color = CONSOLE_BG_COLORS.OKGREEN
    __header_text = ""
    SKIPPED_TEST_TYPES = []

    def __init__(self, number: int, name: str, source: str, expectation: str, result: str, state: TEST_RESULT_STATE):
        self.test_number = number
        self.name = name
        self.source_file_name = source
        self.expectation_file_name = expectation
        self.resultFileName = result
        self.state = state

    def __str__(self):
        self.__setup_output()
        str_representation = f'''{self.calculate_color()}
        Test №{self.test_number}. Test {self.name}.
        Test is {"completed successfully!" if self.is_completed else "not completed("}
        Sources for the test: {self.source_file_name}
        Test expectations: {self.expectation_file_name}
        Test results: {self.resultFileName}. 
        To get more detail info about comparison results, print:
        vimdiff {self.source_file_name} {self.expectation_file_name} {self.resultFileName}
        or vimdiff {self.expectation_file_name} {self.resultFileName}{CONSOLE_BG_COLORS.ENDC}
        '''
        return str_representation

    def __setup_output(self):
        if self.state == TEST_RESULT_STATE.COMPLETED:
            __text_color = CONSOLE_BG_COLORS.OKGREEN
            __header_text = "Congratulations! Test completed successfully!"
        elif self.state == TEST_RESULT_STATE.MISSING_EXPECTATION:
            __text_color = CONSOLE_BG_COLORS.OKCYAN
            __header_text = "Oh! Test didn't complete: no expectation("
        elif self.state == TEST_RESULT_STATE.COMPILER_ERROR_HAPPEND:
            __text_color = CONSOLE_BG_COLORS.WARNING
            __header_text = "Oh! Test didn't complete: compiler error("
        else:
            __text_color = CONSOLE_BG_COLORS.FAILED
            __header_text = "Sorry, but test failed..."

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
    return COMPILER_ERROR_HAPPEND in text

def process_sources(source_filename, path_to_translator):
    if RESULTS_FOLDER_NAME not in os.listdir(os.getcwd()):
        os.mkdir(os.path.join(os.getcwd(), RESULTS_FOLDER_NAME))

    path_to_results_folder = os.path.join(os.getcwd(), RESULTS_FOLDER_NAME)
    result_filename = os.path.join(path_to_results_folder, source_filename.split(os.sep)[-1].split(".")[0]) + ".kumir.c"

    #call kumir2-arduino with params: --out="path_to_cwd/results/test_name.kumir.c" -s ./test_name.kum
    popen = subprocess.Popen([path_to_translator,
                              f'--out={result_filename}',
                              '-s',
                              source_filename],
                             stdout=subprocess.PIPE)
    popen.wait()
    translation_data = popen.stdout.read()

    return result_filename

def compare_data(expected_data, processed_data):
    result_without_kumir_ref = remove_control_characters(processed_data[2:])
    expected_data = remove_control_characters(expected_data)

    if (has_errors(result_without_kumir_ref)):
        return TEST_RESULT_STATE.COMPILER_ERROR_HAPPEND

    for i in range(len(result_without_kumir_ref)):
        if result_without_kumir_ref[i] != expected_data[i]:
            return TEST_RESULT_STATE.FAILED

    return TEST_RESULT_STATE.COMPLETED

#log comparison results to file
def log_test_results(logs_filename, log_data: TestResult[], log_to_console):
    completed_tests = sum(not x.state == TEST_RESULT_STATE.COMPLETED for x in log_data)
    log_data.sort(key=lambda x: x.state, reverse=True)

    log_data_strings = list(map(lambda x: str(x), log_data))
    log_data_strings.insert(0, f'''
    There were: {len(log_data)} tests.
    Completed / Uncompleted: {completed_tests} / {len(log_data) - completed_tests}''')

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

    [-ss] [--skip-successfull] - skip succesfully completed tests.

    [-sf] [--skip-failed] - skip failed tests.

    [-swe] [--skip-without-expectation] - skip tests for which the file with expectations was not found.

    [-sce] [--skip-with-compiler-error] - skip tests ended with compiler error.
    """)

def process_args():
    result = ["", "", False]
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
        
    args = sys.argv[1:]
    for i in range(1, len(args)):
        if args[i - 1] in ARGS["translator"] or args[i - 1] in ARGS["output"]:
            validate_file_name(args[i], False if args[i - 1] in ARGS["translator"] else True)
            if args[i - 1] in ARGS["translator"]:
                result[0] = args[i]
            else: 
                result[1] = args[i]
        elif args[i-1] in ARGS["duplicate"]:              
            result[2] = True
            
    return result

def get_files_with_absolute_paths(folder_name):
    path_to_folder = os.path.join(os.getcwd(), folder_name)
    files = list(map(lambda x: os.path.join(path_to_folder, x), os.listdir(path=path_to_folder)))
    files.sort()

    return  files

if __name__=="__main__":
    args_data = process_args()
    processed_dir_names = map(lambda str: str.lower(), os.listdir(os.getcwd()))
    
    if SOURCES_FOLDER_NAME.lower() not in processed_dir_names \
            and EXPECTATIONS_FOLDER_NAME.lower() not in processed_dir_names:
        print("Execution folder doesn't contain folders with source and processed data")       
        sys.exit(2)

    source_files = get_files_with_absolute_paths(SOURCES_FOLDER_NAME)
    expectation_files = get_files_with_absolute_paths(EXPECTATIONS_FOLDER_NAME)

    test_results = []

    for i in range(len(source_files)):
        if os.path.isfile(expectation_files[i]):
            expected_data = get_file_data(expectation_files[i])
            test_result = TestResult(i,
                                     source_files[i].split(".")[0],
                                     source_files[i],
                                     expectation_files[i],
                                     process_sources(source_files[i], args_data[0]), TEST_RESULT_STATE.NONE)
            result_data = get_file_data(test_result.resultFileName)

            test_result.state = compare_data(expected_data, result_data)
        else:
            test_result = TestResult(i,
                                     source_files[i].split(".")[0],
                                     source_files[i],
                                     expectation_files[i],
                                     "", TEST_RESULT_STATE.MISSING_EXPECTATION)

        if test_result.state not in TestResult.SKIPPED_TEST_TYPES:                                   
            test_results.append(test_result)

    log_test_results(args_data[1], test_results, args_data[2])
