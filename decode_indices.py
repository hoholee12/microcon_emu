
import struct
import os

MAGIC_NUMBER_ALLOCATED = 0xAAAAAAAA
MAGIC_NUMBER_FREED = 0xCCCCCCCC
MAGIC_NUMBER_ALLOCATED_REL = 0xAA
MAGIC_NUMBER_FREED_REL = 0xCC

MAX_POOL_SIZE = 0x100000  # 1MB
HEADER_SIZE_FULL = 12  # 3 uint32s
HEADER_SIZE_REL = 8    # 2 uint32s

class PoolValidator:
    def __init__(self, filename):
        self.filename = filename
        self.data = None
        self.mode = None
        self.errors = []
        self.warnings = []
        self.headers = []
        
    def read_file(self):
        """Read the binary dump file"""
        try:
            with open(self.filename, 'rb') as f:
                self.data = f.read()
            print("[OK] Read {0} bytes from {1}".format(len(self.data), self.filename))
            return True
        except FileNotFoundError:
            print("[ERROR] File not found: {0}".format(self.filename))
            return False
        except Exception as e:
            print("[ERROR] Error reading file: {0}".format(e))
            return False
    
    def detect_mode(self):
        """Detect if pool uses FULL or RELATIVE indexing"""
        if len(self.data) < 4:
            self.errors.append("File too small to read first magic number")
            return False
        
        # Check index 0 for full mode magic number
        first_magic = struct.unpack('<I', self.data[0:4])[0]
        
        if first_magic == MAGIC_NUMBER_ALLOCATED:
            self.mode = "FULL"
            print("[OK] Detected FULL mode (magic at offset 0: 0x{0:08X})".format(first_magic))
            return True
        else:
            self.mode = "RELATIVE"
            print("[INFO] Detected RELATIVE mode (no magic at offset 0: 0x{0:08X})".format(first_magic))
            return True
    
    def parse_full_mode(self):
        """Parse headers in FULL mode (absolute indexing)"""
        print("\n--- Parsing FULL Mode Headers ---")
        offset = 0
        block_num = 0
        
        while offset + HEADER_SIZE_FULL <= len(self.data):
            magic, prev, nxt = struct.unpack('<III', self.data[offset:offset + HEADER_SIZE_FULL])
            index = offset // 4  # Convert byte offset to word index
            
            self.headers.append({
                'index': index,
                'offset': offset,
                'magic': magic,
                'prev': prev,
                'next': nxt,
                'mode': 'FULL'
            })
            
            offset += HEADER_SIZE_FULL
            block_num += 1
            
            if block_num <= 10 or block_num % 1000 == 0:
                status = "ALLOCATED" if magic == MAGIC_NUMBER_ALLOCATED else "FREED" if magic == MAGIC_NUMBER_FREED else "INVALID"
                print("  Block {0}: idx=0x{1:06X} magic=0x{2:08X} ({3}) prev=0x{4:06X} next=0x{5:06X}".format(
                    block_num, index, magic, status, prev, nxt))
        
        print("[OK] Parsed {0} headers".format(block_num))
        return block_num
    
    def parse_relative_mode(self):
        """Parse headers in RELATIVE mode (relative indexing)"""
        print("\n--- Parsing RELATIVE Mode Headers ---")
        offset = 0
        block_num = 0
        
        while offset + HEADER_SIZE_REL <= len(self.data):
            firstword, secondword = struct.unpack('<II', self.data[offset:offset + HEADER_SIZE_REL])
            index = offset // 4  # Convert byte offset to word index
            
            # Decode the header
            decoded_first = (~index & 0xFFFFFFFF) ^ firstword
            decoded_second = index ^ secondword
            
            # Extract magic
            magic_byte1 = decoded_first & 0xFF
            magic_byte2 = decoded_second & 0xFF
            
            # Extract indices
            prev_idx = (((decoded_first >> 8) & 0xFF) << 16) | \
                       (((decoded_first >> 24) & 0xFF) << 8) | \
                       ((decoded_second >> 8) & 0xFF)
            
            next_idx = (((decoded_first >> 16) & 0xFF) << 16) | \
                       (((decoded_second >> 16) & 0xFF) << 8) | \
                       ((decoded_second >> 24) & 0xFF)
            
            # Determine status
            if magic_byte1 == MAGIC_NUMBER_ALLOCATED_REL and magic_byte2 == MAGIC_NUMBER_ALLOCATED_REL:
                status = "ALLOCATED"
            elif magic_byte1 == MAGIC_NUMBER_FREED_REL and magic_byte2 == MAGIC_NUMBER_FREED_REL:
                status = "FREED"
            else:
                status = "INVALID (magic1=0x{0:02X}, magic2=0x{1:02X})".format(magic_byte1, magic_byte2)
            
            self.headers.append({
                'index': index,
                'offset': offset,
                'firstword': firstword,
                'secondword': secondword,
                'decoded_first': decoded_first,
                'decoded_second': decoded_second,
                'magic1': magic_byte1,
                'magic2': magic_byte2,
                'prev': prev_idx,
                'next': next_idx,
                'mode': 'RELATIVE'
            })
            
            offset += HEADER_SIZE_REL
            block_num += 1
            
            if block_num <= 10 or block_num % 1000 == 0:
                print("  Block {0}: idx=0x{1:06X} ({2}) prev=0x{3:06X} next=0x{4:06X}".format(
                    block_num, index, status, prev_idx, next_idx))
        
        print("[OK] Parsed {0} headers".format(block_num))
        return block_num
    
    def validate_links(self):
        """Validate that forward and backward links are consistent"""
        print("\n--- Validating Links ---")
        
        if not self.headers:
            print("[ERROR] No headers to validate")
            return
        
        for i, header in enumerate(self.headers):
            idx = header['index']
            prev = header['prev']
            nxt = header['next']
            
            # Check if next index points to a valid header
            if nxt > 0:
                next_header = None
                for h in self.headers:
                    if h['index'] == nxt:
                        next_header = h
                        break
                
                if next_header is None:
                    msg = "Block 0x{0:06X}: next pointer 0x{1:06X} doesn't point to valid header".format(idx, nxt)
                    self.errors.append(msg)
                    if i <= 10:
                        print("  [ERROR] {0}".format(msg))
                elif next_header['prev'] != idx:
                    msg = "Block 0x{0:06X}: next block's prev (0x{1:06X}) doesn't point back to this block".format(idx, next_header['prev'])
                    self.errors.append(msg)
                    if i <= 10:
                        print("  [ERROR] {0}".format(msg))
            
            # Check if prev index points to a valid header
            if prev > 0:
                prev_header = None
                for h in self.headers:
                    if h['index'] == prev:
                        prev_header = h
                        break
                
                if prev_header is None:
                    msg = "Block 0x{0:06X}: prev pointer 0x{1:06X} doesn't point to valid header".format(idx, prev)
                    self.errors.append(msg)
                    if i <= 10:
                        print("  [ERROR] {0}".format(msg))
                elif prev_header['next'] != idx:
                    msg = "Block 0x{0:06X}: prev block's next (0x{1:06X}) doesn't point back to this block".format(idx, prev_header['next'])
                    self.errors.append(msg)
                    if i <= 10:
                        print("  [ERROR] {0}".format(msg))
        
        if not self.errors:
            print("[OK] All links are valid")
        else:
            print("[ERROR] Found {0} link validation errors".format(len(self.errors)))
    
    def check_magic_numbers(self):
        """Validate magic numbers"""
        print("\n--- Validating Magic Numbers ---")
        
        invalid_count = 0
        for i, header in enumerate(self.headers):
            if self.mode == "FULL":
                magic = header['magic']
                if magic not in [MAGIC_NUMBER_ALLOCATED, MAGIC_NUMBER_FREED]:
                    msg = "Block 0x{0:06X}: invalid magic 0x{1:08X}".format(header['index'], magic)
                    self.errors.append(msg)
                    invalid_count += 1
                    if invalid_count <= 10:
                        print("  [ERROR] {0}".format(msg))
            else:  # RELATIVE
                m1, m2 = header['magic1'], header['magic2']
                if not ((m1 == MAGIC_NUMBER_ALLOCATED_REL and m2 == MAGIC_NUMBER_ALLOCATED_REL) or \
                        (m1 == MAGIC_NUMBER_FREED_REL and m2 == MAGIC_NUMBER_FREED_REL)):
                    msg = "Block 0x{0:06X}: invalid magic bytes 0x{1:02X}/0x{2:02X}".format(header['index'], m1, m2)
                    self.errors.append(msg)
                    invalid_count += 1
                    if invalid_count <= 10:
                        print("  [ERROR] {0}".format(msg))
        
        if invalid_count == 0:
            print("[OK] All magic numbers valid")
        else:
            print("[ERROR] Found {0} invalid magic numbers".format(invalid_count))
    
    def print_summary(self):
        """Print validation summary"""
        print("\n" + "="*60)
        print("VALIDATION SUMMARY")
        print("="*60)
        print("Mode: {0}".format(self.mode))
        print("File size: {0} bytes".format(len(self.data)))
        print("Headers parsed: {0}".format(len(self.headers)))
        print("Errors: {0}".format(len(self.errors)))
        print("Warnings: {0}".format(len(self.warnings)))
        
        if self.errors:
            print("\nERRORS:")
            for error in self.errors[:20]:
                print("  - {0}".format(error))
            if len(self.errors) > 20:
                print("  ... and {0} more".format(len(self.errors) - 20))
        
        if self.warnings:
            print("\nWARNINGS:")
            for warning in self.warnings[:20]:
                print("  - {0}".format(warning))
            if len(self.warnings) > 20:
                print("  ... and {0} more".format(len(self.warnings) - 20))
        
        if not self.errors and not self.warnings:
            print("\n[OK] Pool validation PASSED - No errors found")
        else:
            print("\n[ERROR] Pool validation FAILED")
        print("="*60)
    
    def validate(self):
        """Main validation routine"""
        print("="*60)
        print("LOGALLOC POOL VALIDATOR")
        print("="*60)
        
        if not self.read_file():
            return False
        
        if not self.detect_mode():
            return False
        
        if self.mode == "FULL":
            self.parse_full_mode()
        else:
            self.parse_relative_mode()
        
        self.check_magic_numbers()
        self.validate_links()
        self.print_summary()
        
        return len(self.errors) == 0


if __name__ == "__main__":
    validator = PoolValidator("microcon_emu/logalloc_dump.bin")
    success = validator.validate()
