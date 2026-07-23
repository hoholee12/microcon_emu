
def extract_indices(index, firstword, secondword):
    firstword = (0xffffffff - index) ^ firstword
    secondword = index ^ secondword

    magic = (firstword & 0xFF) | (secondword & 0xFF)
    if magic == 0xAA:
        print("Magic number check passed (ALLOCATED): firstword=0x" + format(firstword, "08X") + ", secondword=0x" + format(secondword, "08X"))
    elif magic == 0xCC:
        print("Magic number check passed (FREED): firstword=0x" + format(firstword, "08X") + ", secondword=0x" + format(secondword, "08X"))
    else:
        print("Magic number check failed: firstword=0x" + format(firstword, "08X") + ", secondword=0x" + format(secondword, "08X"))
    
    # Extract nextindex: (next0 << 16) | (next1 << 8) | next2
    nextindex = (((firstword >> 16) & 0xFF) << 16) | \
                (((secondword >> 16) & 0xFF) << 8) | \
                ((secondword >> 24) & 0xFF)
    
    # Extract previndex: (prev1 << 16) | (prev0 << 8) | prev2
    previndex = (((firstword >> 8) & 0xFF) << 16) | \
                (((firstword >> 24) & 0xFF) << 8) | \
                ((secondword >> 8) & 0xFF)
    
    return previndex, nextindex


if __name__ == "__main__":
    try:
        index = int(input("Enter index (hex): "), 16)
        firstword = int(input("Enter firstword (hex): "), 16)
        secondword = int(input("Enter secondword (hex): "), 16)
    except ValueError:
        print("Error: Invalid input. Please enter hexadecimal values.")
        exit(1)
    
    previndex, nextindex = extract_indices(index, firstword, secondword)
    
    print("\nInput:")
    print("  index:      0x" + format(index, "08X"))
    print("  firstword:  0x" + format(firstword, "08X"))
    print("  secondword: 0x" + format(secondword, "08X"))
    print("\nOutput:")
    print("  previndex:  0x" + format(previndex, "08X"))
    print("  nextindex:  0x" + format(nextindex, "08X"))
