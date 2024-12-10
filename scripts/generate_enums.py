import argparse
import os.path


def generate_file(data, class_name, output_path):
    with open(output_path, "w", encoding='utf-8') as f:
        f.write("#pragma once\n\n")
        f.write("#include <dwarf.h>\n\n")

        f.write("namespace cppdwarf {\n")

        f.write(f"enum class {class_name} " + "{\n")
        for name, value, raw_name in data:
            f.write(f"{name} = {raw_name},\n")
        f.write("};\n\n")
        f.write("}\n")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("path", help="Path to dwarf.h")
    parser.add_argument("--output_folder", "-of", help="Output folder", default=".")
    args = parser.parse_args()

    tags = []
    attributes = []
    forms = []

    with open(args.path, "r") as f:
        lines = f.readlines()
        for line in lines:
            result = line.strip().split(' ')
            result = [item for item in result if item]
            if len(result) < 3 or result[0] != "#define":
                continue

            raw_name = result[1]
            value = result[2]
            if raw_name.startswith("DW_TAG_"):
                name = raw_name[len("DW_TAG_"):]
                tags.append((name, value, raw_name))
            elif raw_name.startswith("DW_AT_"):
                name = raw_name[len("DW_AT_"):]
                attributes.append((name, value, raw_name))
            elif raw_name.startswith("DW_FORM_"):
                name = raw_name[len("DW_FORM_"):]
                forms.append((name, value, raw_name))
            else:
                continue

    generate_file(tags, "tag", os.path.join(args.output_folder, "tag.h"))
    generate_file(attributes, "attribute_t", os.path.join(args.output_folder, "attribute_t.h"))
    generate_file(forms, "form", os.path.join(args.output_folder, "form.h"))


if __name__ == "__main__":
    main()
