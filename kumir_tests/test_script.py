import os
import sys
import subprocess

BYTES_TO_READ_COUNT = 1024
CHAR_THRESHOLD = 0.3
TEXT_CHARACTERS = ''.join(
	[chr(code) for code in range(32,127)] +
	list('\b\f\n\r\t')
)
BINARY_CHAR_EXAMPLE = '\x00'

SOURCES_FOLDER_NAME = "Sources"
EXPECTATIONS_FOLDER_NAME = "Expectations"
RESULTS_FOLDER_NAME = "Results"

ARGS = {"help":["-h", "--help"],
        "compiler": ["-c", "--compiler"],
        "output": ["-o", "--output"],
        "duplicate": ["-d", "--duplicate"]}
        
ERRORS = {
	"wrong input": "Wrong args input - you should type path to file after -c or -o flags!",
	"file not exist": "File doesn't exist!",
	"wrong file extention": "File extention for translator is not correct! It should be a bin-file."
		}

def get_file_data(filename):
    file = open(filename, "r")
    result = file.readlines()
    file.close()
    return result

def process_sources(source_filename, path_to_translator):
    if RESULTS_FOLDER_NAME.lower() not in os.listdir(os.getcwd()):
        os.mkdir(os.path.join(get.getcwd()))
    
    #call kumir2-arduino with params: --out="path_to_cwd/results/test_name.kumir.c" -s ./test_name.kum
    popen = subprocess.Popen([path_to_translator, f'--out={path.join(os.getcwd(), source_filename}.c', '-s', source_filename], stdout=subprocess.PIPE)
    popen.wait()
    return popen.stdout.read()

def compare_data(result_data, processed_data):
    pass

#log comparision results to file 
def calculate_compare_results():
    pass

def is_binary_file(filename):
    file_stream = open(filename, 'rb')
    file_content = file_stream.read(BYTES_TO_READ_COUNT)
    file_stream.close()
    
    if not len(file_content):
        #file is empty, nothing to read
        return False
    
    if ord('\x00') in file_content:
        #file contatins binary symbols
        return True
    
    binary_chars = file_content.translate(TEXT_CHARACTERS)
    return float(len(binary_chars)) / len(file_content) >= CHAR_THRESHOLD

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
    """)

def process_args():
    result = ["", "", False]
    if ARGS["help"][0] in sys.argv or ARGS["help"][1] in sys.argv:
        show_help()
        sys.exit(2)
        
    args = sys.argv[1:]
    for i in range(1, len(args)):
        if args[i - 1] in ARGS["compiler"] or args[i - 1] in ARGS["output"]:
            validate_file_name(args[i], False if args[i - 1] in ARGS["compiler"] else True)
            if args[i - 1] in ARGS["compiler"]:
                result[0] = args[i]
            else: 
                result[1] = args[i]
        elif args[i-1] in ARGS["duplicate"]:              
            result[2] = True
            
    return result        
#TODO: create function to hightlight diff in tests, like:
#   test_num: n
#   source_file: test_file_name.kum
#   Result: OK - go to the next.
#   in other case it should highlight the differences in expectation and result
#   mb, just call vimdiff and retranslate output.
if __name__=="__main__":
    args_data = process_args()
    processed_dir_names = map(lambda str: str.lower(), os.listdir(os.getcwd()))
    
    if SOURCES_FOLDER_NAME.lower() not in processed_dir_names and EXPECTATIONS_FOLDER_NAME.lower() not in processed_dir_names:
        print("Execution folder doesn't contain folders with source and processed data")       
        sys.exit(2)
    
    source_files = os.listdir(os.path.join(os.getcwd(), SOURCES_FOLDER_NAME))
    expectation_files = os.listdir(os.path.join(os.getcwd(), EXPECTATIONS_FOLDER_NAME))
    
    if len(source_files) > len(expectation_files):
        print("Tests amount > expectations amount")
        sys.exit(2)
    
    for i in range(len(source_files)):
        expected_data = get_file_data(expectation_files[i])
        result_data = process_sources(source_files[i])
        if compare_data(result_data, expected_data):
            
