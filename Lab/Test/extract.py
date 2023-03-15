import pytesseract
from PIL import Image
import sys

# Get filename from command line arguments
if len(sys.argv) > 1:
    filename = sys.argv[1]
else:
    print("Please provide an image filename as a command line argument.")
    sys.exit()

# Load the image
image = Image.open(filename)

# Extract text from the image
text = pytesseract.image_to_string(image)

# Print the text (including whitespace)
print(text)
