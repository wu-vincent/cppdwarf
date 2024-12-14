import os
import subprocess
from pathlib import Path
import argparse
import glob
from concurrent.futures import ProcessPoolExecutor
import re

from tqdm import tqdm


def recursive_sub(pattern: str, repl: str, content: str):
    while True:
        output = re.sub(pattern, repl, content)
        if output == content:
            break
        content = output

    return content


def process_file(args):
    """
    Process a single file. Replace this with your file handling logic.
    """
    file_path, input_path, output_path = args

    with open(file_path, "r", encoding="utf-8") as f:
        content = f.read()

    # Replace every occurrence of 'std::__1::' with 'std::'
    content = content.replace("std::__1::", "std::")
    content = content.replace(
        "std::basic_string<char, std::char_traits<char>, std::allocator<char> >",
        "std::string",
    )
    content = content.replace(
        "std::chrono::time_point<std::chrono::steady_clock, std::chrono::duration<long long, std::ratio<1L, 1000000000L> > >",
        "std::chrono::steady_clock::time_point",
    )

    # Regex to match and replace complex types in std::unique_ptr with default_delete
    pattern = r"std::unique_ptr<(.+?),\s*std::default_delete<\1\s*>\s*>"
    content = recursive_sub(pattern, r"std::unique_ptr<\1>", content)

    # Regex to match and replace std::vector with default allocator
    pattern = r"std::vector<(.+?),\s*std::allocator<\1\s*>\s*>"
    content = recursive_sub(pattern, r"std::vector<\1>", content)

    # Regex for std::unordered_map with complex default parameters
    pattern = (
        r"std::unordered_map<(.+?),\s*(.+?),\s*"
        r"std::hash<\1\s*>,\s*"
        r"std::equal_to<\1\s*>,\s*"
        r"std::allocator<std::pair<const \1,\s*\2\s*>\s*>\s*>"
    )
    content = recursive_sub(pattern, r"std::unordered_map<\1, \2>", content)

    # Regex for std::map with complex default parameters
    pattern = (
        r"std::map<(.+?),\s*(.+?),\s*std::less<\1\s*>,\s*"
        r"std::allocator<std::pair<const \1,\s*\2\s*>\s*>\s*>"
    )
    content = recursive_sub(pattern, r"std::map<\1, \2>", content)

    # Regex for std::unordered_set with complex default parameters
    pattern = (
        r"std::unordered_set<(.+?),\s*std::hash<\1\s*>,\s*"
        r"std::equal_to<\1\s*>,\s*std::allocator<\1\s*>\s*>"
    )
    content = recursive_sub(pattern, r"std::unordered_set<\1>", content)

    # Calculate relative path
    relative_path = file_path.relative_to(input_path)

    # Construct the full output path
    result_file = output_path / relative_path

    # Ensure the directory of the result file exists
    result_file.parent.mkdir(parents=True, exist_ok=True)

    # Example file processing logic
    with result_file.open('w') as f:
        f.write(content)

    # Call clang-format on the file
    clang_format_file = os.path.join(os.path.dirname(__file__), ".clang-format")
    subprocess.run(
        ["clang-format", "-i", f"-style=file:{clang_format_file}", result_file],
        check=True,
    )


def main():
    # Set up argument parsing
    parser = argparse.ArgumentParser(description="Process files in a specified directory using multiprocessing.")
    parser.add_argument('--input', '-i', required=True, type=str,
                        help="Path to the input directory containing the files to process.")
    parser.add_argument('--output', '-o', required=True, type=str,
                        help="Path to the output directory to store results.")
    args = parser.parse_args()

    # Ensure the output directory exists
    Path(args.output).mkdir(parents=True, exist_ok=True)

    # Find all files in the input directory
    input_files = glob.glob(str(Path(args.input) / '**' / '*.*'), recursive=True)

    if not input_files:
        print("No files found in the input directory.")
        return

    # Create tuples of file path, input path, and output path for processing
    files_with_paths = [(Path(file), Path(args.input), Path(args.output)) for file in input_files if
                        os.path.isfile(file)]

    # Process the files using a multiprocessing pool with tqdm for progress tracking
    with ProcessPoolExecutor() as executor:
        list(tqdm(executor.map(process_file, files_with_paths), total=len(files_with_paths), desc="Processing files"))

    print("Processing complete!")


if __name__ == "__main__":
    main()
