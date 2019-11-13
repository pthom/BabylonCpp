import os
import os.path
from typing import *

CodeLine = str
CompilerLine = str
Folder = str
Extension = str
FileFullPath = str
FileBasename = str
LineNumber = int


def ToCamelCase(snake_case: str) -> str:
    items  = snake_case.split("_")
    def UpperFirstLetter(word):
        if len(word) == 0:
            return ""
        return word[:1].upper() + word[1:]    
    itemsCamel = map(UpperFirstLetter, items)
    r = "".join(itemsCamel)
    return r

def ClassName_from_filename(file: FileFullPath):
    basename = file.split("/")[-1]
    basename_no_extension = basename.replace(".cpp", "").replace(".h", "")
    class_name = ToCamelCase(basename_no_extension)
    return class_name

def files_with_extension(folder: Folder, extension: Extension) -> List[FileFullPath]:
    r = []
    ext_len = len(extension)
    if not os.path.isdir(folder):
        print("ouch")
    for root, _, files in os.walk(folder):
        for file in files:
            if file[-ext_len:] == extension:
                full_file = root + "/" + file
                full_file = full_file.replace("\\", "/")
                r.append(full_file)
    return r

def read_file_lines_no_eol(FileFullPath: FileFullPath) -> List[CodeLine]:
    with open(FileFullPath, "r") as f:
        content = f.read()
    lines = content.split("\n")
    return lines
    # lines_no_eol = []
    # for line in lines:
    #     if line[-1:] == "\n":
    #         lines_no_eol.append(line[:-1])
    #     else:
    #         lines_no_eol.append(line)
    # return lines_no_eol


def write_file_lines_no_eol(FileFullPath: FileFullPath, lines: List[CodeLine]):
    content = "\n".join(lines)
    with open(FileFullPath, "w") as f:
        f.write(content)


def remove_include(h_file_to_remove: FileFullPath, src_file_to_process: FileFullPath):
    lines = read_file_lines_no_eol(src_file_to_process)
    def keep_line(line):
        header_file_quoted = "<" + h_file_to_remove + ">"
        match = ("#include" in line) and (header_file_quoted in line)
        if match:
            print("match!")
        return not match
    lines_filtered = list(filter(keep_line, lines))
    if lines_filtered != lines:
        write_file_lines_no_eol(src_file_to_process, lines_filtered)
        print("{} file was impacted".format(src_file_to_process))


def is_include_line(line: str):
    return line.startswith("#include")


def is_warning_trivial_destructor(line):
    return "trivial destructor" in line and "hicpp-use-equals-default" in line


def warning_location(line) -> (FileFullPath, LineNumber):
    items = line.split(" ")
    if ":" in items[0]:
        location = items[0]
    elif ":" in items[1]:
        location = items[1]
    else:
        sdfmfdklm() # error
    items2 = location.split(":")
    file_full_path = items2[0]
    line_number = items2[1]
    return file_full_path, int(line_number) - 1

def is_cpp(file: FileFullPath) -> bool:
    return file.endswith(".cpp")


def h_file_from_cpp(cpp_file: FileFullPath, all_h_files: List[FileFullPath]) -> Optional[FileFullPath]:    
    items = cpp_file.split("/")
    file_with_parent_folder = "/".join(items[-2:])
    basename_with_parent_folder = file_with_parent_folder.replace(".cpp", "")
    found_h_files = list(filter(lambda f: basename_with_parent_folder + ".h" in f, all_h_files))
    assert(len(found_h_files) <= 1)
    if len(found_h_files) == 1:
        return found_h_files[0]
    else:
        return None


def correct_one_trivial_destructor(cpp_file, cpp_file_line, h_file):
    ClassName = None

    # correct cpp file
    lines = read_file_lines_no_eol(cpp_file)
    line_with_warning = lines[cpp_file_line]
    index_start = line_with_warning.find("~")
    index_end = line_with_warning.find("(", index_start)
    if index_start < 0 or index_end < 0:        
        return
    ClassName = line_with_warning[index_start + 1:index_end]
    # code_to_search = "::~{}(".format(ClassName).lower()
    # need_process = False
    # if code_to_search in line_with_warning.replace(" ", "").lower():
    #     need_process = True
    # if not need_process:
    #     print("Argh")
    if True:
        exclusion_start = cpp_file_line
        exclusion_end = 0
        idx_line = exclusion_start
        found_closing = False
        while found_closing == False:
            line = lines[idx_line]
            if "}" in line:
                found_closing = True
            idx_line = idx_line + 1
        exclusion_end = idx_line + 1
        lines_new = []
        for idx, line in enumerate(lines):
            if idx >= exclusion_start and idx < exclusion_end:
                continue
            if idx == exclusion_end:
                lines_new.append("{}::~{}() = default;".format(ClassName, ClassName))
                lines_new.append("")
            lines_new.append(line)
        # print("\n".join(lines_new))
        write_file_lines_no_eol(cpp_file, lines_new)

    # correct h file
    lines = read_file_lines_no_eol(h_file)
    lines_new = []
    for line in lines:
        if "~" in line and ClassName + "()" in line:
            if "//" in line:
                line = line[:line.index("//")]
                line = line.rstrip()
            line = line.replace(" override ", "")
            line = line.replace("override ", "")
            line = line.replace(" override", "")
            line = line.replace("override", "")
            line = line + " // = default"

        lines_new.append(line)
    write_file_lines_no_eol(h_file, lines_new)



def correct_trivial_destructors(all_h_files):
    warning_lines = read_file_lines_no_eol("/home/pascal/dvp/BabylonCpp/warnings.txt")
    warning_destructor = filter(is_warning_trivial_destructor, warning_lines)
    locations = [warning_location(line) for line in warning_destructor]
    # print(locations)
    for i, location in enumerate(locations):
        cpp_file, cpp_file_line = location
        print("Processing {} {}/{}".format(i, cpp_file, len(locations)))
        h_file = h_file_from_cpp(cpp_file, all_h_files)
        correct_one_trivial_destructor(cpp_file, cpp_file_line, h_file)


def main():
    repo_dir = os.path.dirname(os.path.realpath(__file__ +  "/.."))
    src_dir = repo_dir + "/src"
    h_files = files_with_extension(src_dir, ".h")
    cpp_files = files_with_extension(src_dir, ".cpp")
    correct_trivial_destructors(h_files)

if __name__ == "__main__":
    main()

