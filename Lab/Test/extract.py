import os
import re

# Get the current working directory
cwd = os.getcwd()

# Loop over each file in the current working directory
for filename in os.listdir(cwd):
    # Check if the file is a text file
    if filename.endswith(".txt"):
        # Create the input and output filenames
        input_filename = os.path.join(cwd, filename)
        output_filename = os.path.join(cwd, "out" + filename)

        # Open the input file for reading
        with open(input_filename, "r") as input_file:
            # Open the output file for writing
            with open(output_filename, "w") as output_file:
                # Loop over each line in the input file
                for line in input_file:
                    # Regular expression pattern to extract error type and line number
                    pattern = r"Error type (\w) at Line (\d+):"

                    # Use regular expression to extract error type and line number
                    match = re.search(pattern, line)

                    # Check if a match was found
                    if match:
                        error_type = match.group(1)
                        line_number = match.group(2)

                        # Write the extracted error type and line number to the output file
                        output_file.write(error_type + " " + line_number + "\n")
                    else:
                        output_file.write("No match found for line: " + line.strip() + "\n")
