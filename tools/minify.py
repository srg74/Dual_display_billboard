import subprocess
import sys
import os
import re
import gzip
import inspect
from pathlib import Path
from io import BytesIO

# Constants
src_dir = os.path.join("..", "data")
out_dir = os.path.join("..", "include")
out_file = "webcontent.h"

# List of required packages
required_packages = ["htmlmin2", "jsmin"]

current_file = inspect.getfile(inspect.currentframe())
script_dir = os.path.dirname(os.path.abspath(current_file))
out_path = os.path.join(script_dir, out_dir)
entries = []

def ensure_package(package):
    try:
        __import__(package)
    except ImportError:
        print(f"Installing missing package: {package}")
        subprocess.check_call([sys.executable, "-m", "pip", "install", package])


def sanitize_symbol(name: str):
    # Replace dots and dashes with underscores for safe C identifiers
    return name.replace('.', '_').replace('-', '_')


def binary_to_c_array(full_path, compress=False, bytes_per_line=16):
    file_name = Path(full_path).name
    array_name = sanitize_symbol(file_name)
    # Read binary file
    with open(full_path, "rb") as f:
        raw_data = f.read()
    # Gzip compress the input data
    if compress:
        buffer = BytesIO()
        with gzip.GzipFile(fileobj=buffer, mode="wb") as gz:
            gz.write(raw_data)
        out_data = buffer.getvalue()
    else:
        out_data = raw_data
    # Format as C array
    lines = []
    for i in range(0, len(out_data), bytes_per_line):
        chunk = out_data[i:i + bytes_per_line]
        hexes = ', '.join(f'0x{b:02X}' for b in chunk)
        lines.append(f'  {hexes},')
    # Append entries
    entry = f'    {{ "{file_name}", {array_name}, {len(out_data)} }}'
    entries.append(entry)
    # C-style array declaration
    array_declaration = f"const uint8_t {array_name}[] = {{\n" + '\n'.join(lines) + "\n};"
    return array_declaration


def string_to_c_array(s, name="data", compress=False, bytes_per_line=16):
    array_name = sanitize_symbol(name)
    # Gzip compress the input string
    if compress:
        buffer = BytesIO()
        with gzip.GzipFile(fileobj=buffer, mode='wb') as gz:
            gz.write(s.encode('utf-8'))
        out_data = buffer.getvalue()
    else:
        out_data = bytes(s, 'utf-8')
    # Format as C-style array
    lines = []
    for i in range(0, len(out_data), bytes_per_line):
        chunk = out_data[i:i + bytes_per_line]
        hexes = ', '.join(f'0x{b:02X}' for b in chunk)
        lines.append(f'  {hexes},')
    # Append entries
    entry = f'    {{ "{name}", {array_name}, {len(out_data)} }}'
    entries.append(entry)
    # C-style array declaration
    array_declaration = f"const uint8_t {array_name}[] = {{\n" + "\n".join(lines) + "\n};"
    return array_declaration


def process_html_file(path):
    print(f"Processing {path}")
    # Open source file
    with open(path, 'r', encoding='utf-8') as f:
        content = f.read()
    # Minify inline JS
    content = re.sub(
        r'(<script[^>]*>)(.*?)(</script>)',
        lambda m: f"{m.group(1)}{jsmin(m.group(2))}{m.group(3)}",
        content,
        flags=re.DOTALL
    )
    # Minify HTML
    minified_html = htmlmin.minify(content, remove_comments=True, reduce_empty_attributes=True)
    # Compress using gzip
    filename = Path(path).name
    return string_to_c_array(minified_html, name=filename, compress=False)


def process_jquery_file(path):
    print(f"Processing {path}")
    # Open source file
    with open(path, 'r', encoding='utf-8') as f:
        content = f.read()
        # Detect and remove leading copyright comment
        trimmed = re.sub(r'^\s*/\*![\s\S]*?\*/\s*', '', content, count=1)
        filename = Path(path).name
        return string_to_c_array(trimmed, name=filename, compress=True)


def process_copyright_mapping_file(path):
    print(f"Processing {path}")
    # Open source file
    with open(path, 'r', encoding='utf-8') as f:
        content = f.read()
        # Strip initial block comment (including multiline copyright)
        trimmed = re.sub(r'^/\*.*?\*/\s*', '', content, flags=re.DOTALL)
        # Remove sourceMappingURL comment at the end
        trimmed = re.sub(r'//\s*#\s*sourceMappingURL=.*$', '', trimmed)
        filename = Path(path).name
        return string_to_c_array(trimmed, name=filename, compress=True)


def process_misc_file(path):
    print(f"Processing {path}")
    # Open source file
    with open(path, 'r', encoding='utf-8') as f:
        content = f.read()
        filename = Path(path).name
        return string_to_c_array(content, name=filename, compress=True)


def process_css_file(filepath, filename):
    with open(filepath, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Minify CSS (basic)
    content = re.sub(r'/\*.*?\*/', '', content, flags=re.DOTALL)  # Remove comments
    content = re.sub(r'\s+', ' ', content)  # Compress whitespace
    content = content.strip()
    
    # Add to assets
    var_name = filename.replace('.', '_').replace('-', '_').upper()
    
    css_content = f'''
const char {var_name}[] PROGMEM = R"({content})";
'''
    
    return css_content, filename


def make_index():
    struct = """typedef struct {
    const char* filename;
    const uint8_t* data;
    const unsigned int length;
} EmbeddedAsset;

const EmbeddedAsset assets[] = {
""" + ',\n'.join(entries) + "\n};"

    return struct


def generate_utility_functions():
    return '''
// Auto-generated utility functions
#include <Arduino.h>

// Get asset by filename
inline const EmbeddedAsset* getAsset(const char* filename) {
    for (size_t i = 0; i < sizeof(assets) / sizeof(assets[0]); i++) {
        if (strcmp(assets[i].filename, filename) == 0) {
            return &assets[i];
        }
    }
    return nullptr;
}

// Convert binary data to String
inline String assetToString(const EmbeddedAsset* asset) {
    if (asset == nullptr) return String();
    return String((char*)asset->data, asset->length);
}

// Direct access to main portal HTML
inline String getPortalHTML() {
    return String((char*)portal_html, sizeof(portal_html) - 1);
}

// Get portal HTML size
inline size_t getPortalHTMLSize() {
    return sizeof(portal_html) - 1;
}

// Get total number of assets
inline size_t getAssetCount() {
    return sizeof(assets) / sizeof(assets[0]);
}

// Get asset by index
inline const EmbeddedAsset* getAssetByIndex(size_t index) {
    if (index >= getAssetCount()) return nullptr;
    return &assets[index];
}
'''


# =====================================
# Entry point
# =====================================

print(f"Generating HTML content")

# Try installing missing packages
for package in required_packages:
    try:
        __import__(package)
    except ImportError:
        print(f"Installing missing package: {package}")
        subprocess.check_call([sys.executable, "-m", "pip", "install", package])

# Now safe to import
try:
    import htmlmin
except ImportError:
    import htmlmin2 as htmlmin
try:
    from jsmin import jsmin
except ImportError:
    from jsmin2 import jsmin

# Clean existing file
output_file = os.path.join(script_dir, '..', 'include', 'webcontent.h')
output_file = os.path.normpath(output_file)
if os.path.exists(output_file):
    os.remove(output_file)

src_path = os.path.normpath(os.path.join(script_dir, src_dir))

# Process all source files one by one
with open(output_file, "w", encoding="utf-8") as f:
    # Prepend the header file
    f.write("#pragma once\n")
    f.write("#include <cstdint>\n\n")

    # Process all HTML files in the src directory
    for filename in os.listdir(src_path):
        full_path = os.path.join(src_path, filename)
        # html
        if filename.endswith('.html'):
            f.write(process_html_file(full_path) + "\n")
        # js
        elif filename.endswith('.js'):
            # jquery*.js, bootstrap*.js
            if filename.startswith("jquery"):
                f.write(process_jquery_file(full_path) + "\n")
            # bootstrap*.js, popper*.js
            elif filename.startswith(("bootstrap", "popper")):
                f.write(process_copyright_mapping_file(full_path) + "\n")
            # All other js files without copyright 
            else:
                f.write(process_misc_file(full_path) + "\n")
        # css
        elif filename.endswith('.css'):
            if filename.startswith('bootstrap'):
                f.write(process_copyright_mapping_file(full_path) + "\n")
            else:
                f.write(process_css_file(full_path, filename) + "\n")
        # ico, jpg
        elif filename.endswith((".ico",".jpg")):
            f.write(binary_to_c_array(full_path, compress=True) + "\n")

    # Append entries as a C-style index array
    f.write("\n" + make_index() + "\n")
    f.write(generate_utility_functions())

