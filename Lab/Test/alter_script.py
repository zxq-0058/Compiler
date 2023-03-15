import sys

# Get filename from command line arguments
if len(sys.argv) > 1:
    filename = sys.argv[1]
else:
    print("Please provide a filename as a command line argument.")
    sys.exit()

# Read the file
with open(filename, 'r') as file:
    text = file.read()

# Replace tabs with two spaces
text = text.replace('    ', '  ')

# Write the modified text back to the file
with open(filename, 'w') as file:
    file.write(text)
print("alter successfully!");
