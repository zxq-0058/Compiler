import re
import os
import sys


def camel_to_snake(name):
    s1 = re.sub('(.)([A-Z][a-z]+)', r'\1_\2', name)
    return re.sub('([a-z0-9])([A-Z])', r'\1_\2', s1).lower()


def snake_to_camel(name):
    return ''.join(word.title() for word in name.split('_'))


def process_file(filename):
    with open(filename, 'r') as f:
        content = f.read()

    # 替换函数名
    pattern = r'\b([a-zA-Z_]\w*)\('
    def repl_func_name(match):
        return snake_to_camel(match.group(1)) + '('
    content = re.sub(pattern, repl_func_name, content)

    # 替换变量名
    pattern = r'([^\w])([a-zA-Z_]\w*)'
    def repl_var_name(match):
        prefix = match.group(1)
        name = match.group(2)
        if prefix == '.':
            return prefix + snake_to_camel(name)
        else:
            return prefix + camel_to_snake(name)
    content = re.sub(pattern, repl_var_name, content)

    with open(filename, 'w') as f:
        f.write(content)


def process_dir(dirname):
    for root, dirs, files in os.walk(dirname):
        for file in files:
            if file.endswith('.c') or file.endswith('.h'):
                filename = os.path.join(root, file)
                process_file(filename)


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print('Usage: python rename.py [file or directory]')
        sys.exit(1)

    path = sys.argv[1]
    if os.path.isfile(path):
        process_file(path)
    elif os.path.isdir(path):
        process_dir(path)
    else:
        print('Invalid path:', path)
        sys.exit(1)
