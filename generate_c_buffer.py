def format_hex_string(input_string):
    # Split the string into pairs of characters
    hex_pairs = [input_string[i:i+2] for i in range(0, len(input_string), 2)]
    
    # Convert to 0x format
    hex_values = [f"0x{pair.upper()}" for pair in hex_pairs]
    
    # Split into rows of 8 and join with commas
    result = []
    for i in range(0, len(hex_values), 8):
        row = hex_values[i:i+8]
        result.append(", ".join(row))
    
    return result

# Input string
input_str = "A46164A261666358595A62736E6F30313233343536373839414243444562666E02637269641A68B95AA76172A16A706172616D65746572738363726473626677636D6573"

# Format and print
formatted_rows = format_hex_string(input_str)
for row in formatted_rows:
    print(row + ",")