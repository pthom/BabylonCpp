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

def to_snake_case(CamelCase: str) -> str:
    r = ""
    if CamelCase[0] == "I" and CamelCase[1].isupper():
        r = "i" + CamelCase[1].lower()
        CamelCase = CamelCase[2:]
    else:
        r = CamelCase[0].lower()
        CamelCase = CamelCase[1:]

    # PBRMetallicRoughnessMaterial
    for i in range(len(CamelCase)):
        c = CamelCase[i]
        add_underscore = False
        if c.isupper() and i < len(CamelCase) - 1 and CamelCase[i + 1].islower():
            add_underscore = True
        if add_underscore:
            r = r + "_"            
        r = r + c.lower()
    return r

# print(to_snake_case("HelloWorld"))
# print(to_snake_case("PBRMetallicRoughnessMaterial"))
# print(to_snake_case("ICanvas"))

def unique(list1): 
    unique_list = [] 
    for x in list1: 
        if x not in unique_list: 
            unique_list.append(x) 
    return  unique_list


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


def read_file_lines_no_eol(file_full_path: FileFullPath) -> List[CodeLine]:
    with open(file_full_path, "r") as f:
        content = f.read()
    lines = content.split("\n")
    return lines


def write_file_lines_no_eol(file_full_path: FileFullPath, lines: List[CodeLine]):
    content = "\n".join(lines)
    with open(file_full_path, "w") as f:
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
        raise Exception("Could not parse warning")
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


def move_trivial_destructors_to_h(cpp_file: FileFullPath, all_h_files):
    ClassName = ClassName_from_filename(cpp_file)

    # process cpp file
    lines = read_file_lines_no_eol(cpp_file)
    has_trivial_destructor = False
    lines_new = []
    for line in lines:
        code_to_search = "::~{}(".format(ClassName).lower()
        if code_to_search in line.replace(" ", "").lower() and "=default" in line.replace(" ", "").lower():
            has_trivial_destructor = True
        else:
            lines_new.append(line)
    write_file_lines_no_eol(cpp_file, lines_new)

    # process h file
    h_file = h_file_from_cpp(cpp_file, all_h_files)
    if has_trivial_destructor:
        lines = read_file_lines_no_eol(h_file)
        lines_new = []
        for line in lines:
            code_to_search = "~{}(".format(ClassName).lower()
            if code_to_search in line.replace(" ", "").lower():
                line = line.replace(" // = default", "") [:-1] + " = default;"
            lines_new.append(line)
        write_file_lines_no_eol(h_file, lines_new)




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
    if h_file is not None:
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


def find_all_struct_classes_in_header(h_file):
    if "iscene_producer" in h_file:
        print("ah")
    lines = read_file_lines_no_eol(h_file)
    def is_decl(i, line):
        is_decl = False
        if line.startswith("class ") or line.startswith("struct "):
            for i_next in range(i, len(lines)):
                next_line = lines[i_next]
                idx1 = next_line.find(";")
                idx2 = next_line.find("{")                
                if idx1 >= 0 and idx2 >= 0:
                    is_decl = (idx2 < idx1)
                if idx1 < 0 and idx2 < 0:
                    continue
                if idx2 >=0 and idx1 <0:
                    is_decl = True
                    break
                if idx1 >= 0 and idx2 < 0:
                    is_decl = False
                    break
        return is_decl
    def extract_decl_name(decl_line):
        s = decl_line.strip().replace("{", "").strip()
        s = s.replace("BABYLON_SHARED_EXPORT ", "")
        s = s.replace("class ", "")
        s = s.replace("struct ", "")
        if s.find(":") > 0:
            s = s[:s.find(":")].strip()
        if "<" in s: 
            s = s[:s.find("<")].strip()
            print("Arg")
        return s

    all_classes = []
    for i, line in enumerate(lines):
        if is_decl(i, line):
            decl_name = extract_decl_name(line)
            all_classes.append(decl_name)
    return all_classes


def make_babylon_include_path(h_file: FileFullPath):
    include = h_file
    idx = include.find("include/babylon")
    if idx < 0:
        return None
    include = include[idx + 8:]
    return include


def remove_forward_class_decls(h_file: FileFullPath, classes_per_header):
    lines = read_file_lines_no_eol(h_file)
    h_file_include = make_babylon_include_path(h_file)
    def extract_forward_decl_class(forward_decl_line):
        ClassName = forward_decl_line.replace("class", "").replace("struct", "")
        ClassName = ClassName.replace(";", "").replace(" ", "")
        return ClassName
    def is_forward_decl(line):
        if (line.startswith("class ") or line.startswith("struct ")) and line.endswith(";"):
            class_name = extract_forward_decl_class(line)
            if class_name in classes_per_header[h_file_include]:
                return False
            return True
        return False
    def find_header_file(forward_decl_line):
        ClassName = extract_forward_decl_class(forward_decl_line)
        candidate_h_files = []
        for header, classes in classes_per_header.items():
            if ClassName in classes:
                candidate_h_files.append(header)
        if len(candidate_h_files) == 0:
            print("Argh")
            return None
        if len(candidate_h_files) > 1:
            print("Argh")
            candidate_h_files = sorted(candidate_h_files, key = len)
        header_file =candidate_h_files[0]
        return header_file


    def is_removable_forward_decl(line):
        return is_forward_decl(line) and find_header_file(line) is not None

    def extract_shared_ptr_class(ptr_decl_line):
        class_name = ptr_decl_line[ ptr_decl_line.index("<") + 1 : ptr_decl_line.index(">") ]
        class_name = class_name.strip()
        return class_name

    def is_Ptr_decl(line):
        if "Ptr " in line and line.startswith("using") and "shared_ptr<" in line:
            class_name = extract_shared_ptr_class(line)
            if class_name in classes_per_header[h_file_include]:
                return False
            return True
        return False

    def is_removable_Ptr_decl(line):
        if not is_Ptr_decl(line):
            return False
        class_name = extract_shared_ptr_class(line)
        for header, classes in classes_per_header.items():
            for class_name2 in classes:
                if class_name == class_name2:
                    return True
        return False


    def is_babylon_include(line):
        return "#include <babylon/" in line
    def extract_babylon_include(babylon_include_line):
        s = babylon_include_line.strip().replace("#include <", "")
        s = s.replace(">", "")
        return s

    def find_idx_line_last_babylon_include():
        idx_line_last_babylon_include = 0
        for i, line in  enumerate(lines):
            if is_babylon_include(line):
                idx_line_last_babylon_include = i
        return idx_line_last_babylon_include

    def process_new_babylon_includes():
        previous_includes_lines = list(filter(is_babylon_include, lines))
        previous_includes = list(map(extract_babylon_include, previous_includes_lines))
        # print(previous_includes)

        fwd_decls_lines = list(filter(is_removable_forward_decl, lines))
        additional_includes = list(map(find_header_file, fwd_decls_lines))
        # print(additional_includes)

        all_includes = previous_includes + additional_includes
        all_includes = sorted(all_includes)
        all_includes = unique(all_includes)
        # print(all_includes)
        return all_includes

    fwd_decls_lines = list(filter(is_removable_forward_decl, lines))
    if len(fwd_decls_lines) > 0:
        idx_line_last_babylon_include = find_idx_line_last_babylon_include()
        new_babylon_includes = process_new_babylon_includes()
        lines_new = []
        for i, line in enumerate(lines):
            if not is_removable_forward_decl(line) and not is_removable_Ptr_decl(line) and not is_babylon_include(line):
                lines_new.append(line)
            if i == idx_line_last_babylon_include:
                for include in new_babylon_includes:
                    include_line = "#include <{}>".format(include)
                    lines_new.append(include_line)

        # print("\n".join(lines_new))
        write_file_lines_no_eol(h_file, lines_new)


def main():
    repo_dir = os.path.dirname(os.path.realpath(__file__ +  "/.."))
    src_dir = repo_dir + "/src"
    all_h_files = files_with_extension(src_dir, ".h")
    all_cpp_files = files_with_extension(src_dir, ".cpp")
    
    correct_trivial_destructors(all_h_files)

    # classes_per_header = {}
    # for i, h_file in enumerate(all_h_files):
    #     print("Processing {} {}/{}".format(h_file, i, len(all_h_files)))
    #     classes = find_all_struct_classes_in_header(h_file)
    #     classes_per_header[make_babylon_include_path(h_file)] = classes
    # print("Done")


    # for i, h_file in enumerate(all_h_files):
    #     print("Processing {} {}/{}".format(h_file, i, len(all_h_files)))
    #     # move_trivial_destructors_to_h(cpp_file, all_h_files)
    #     remove_forward_class_decls(h_file, classes_per_header)


if __name__ == "__main__":
    main()

