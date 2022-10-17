import os

SOURCES_FOLDER_NAME = "Sources"
EXPECTATIONS_FOLDER_NAME = "Expectations"
RESULTS_FOLDER_NAME = "Results"

def get_file_data(filename):
    file = open(filename, "r")
    result = file.readlines()
    file.close()
    return result

def process_sources(source_filename):
    if RESULTS_FOLDER_NAME.lower() not in os.listdir():
        os.mkdir(os.path.join(get.getcwd()))
        
    #call kumir2-arduino wth params: --out="path_to_cwd/results/test_name.kumir.c" -s ./test_name.kum

def compare_data(result_data, processed_data):
    pass

#log comparision results to file 
def print_compare_results():
    pass

#TODO: think about cmd params and help
#TODO: where should be placed kumir2-robots.bin/kumir2-robots.exe?
if __name__=="__main__":
    processed_dir_names = map(lambda str: str.lower(), os.listdir(os.getcwd()))
    
    if SOURCES_FOLDER_NAME.lower() not in processed_dir_names and
    EXPECTATIONS_FOLDER_NAME.lower() not in processed_dir_names:
        print("Execution folder doesn't contain folders with source and processed data")       
        return
    
    source_files = os.listdir(os.path.join(os.getcwd(), SOURCES_FOLDER_NAME))
    expectation_files = os.listdir(os.path.join(os.getcwd(), EXPECTATIONS_FOLDER_NAME))
    
    if len(source_files) > len(expectation_files):
        print("Tests amount > expectations amount")
        return
    
    for i in range(len(source_files)):
        source_data = get_file_data(source_files[i])
        expected_data = get_file_data(expectation_files[i])
        result_data = process_sources(source_data)
        
    
    
    
