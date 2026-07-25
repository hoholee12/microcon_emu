
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
        scanned_blocks = 0
        
        while offset + HEADER_SIZE_FULL <= len(self.data):
            magic, prev, nxt = struct.unpack('<III', self.data[offset:offset + HEADER_SIZE_FULL])
            index = offset // 4  # Convert byte offset to word index
            
            # Check if this block contains a valid magic number (header)
            if magic in [MAGIC_NUMBER_ALLOCATED, MAGIC_NUMBER_FREED]:
                status = "ALLOCATED" if magic == MAGIC_NUMBER_ALLOCATED else "FREED"
                self.headers.append({
                    'index': index,
                    'offset': offset,
                    'magic': magic,
                    'prev': prev,
                    'next': nxt,
                    'mode': 'FULL',
                    'status': status
                })
                print("  Header at idx=0x{0:06X}: magic=0x{1:08X} ({2}) prev=0x{3:06X} next=0x{4:06X}".format(
                    index, magic, status, prev, nxt))
            
            offset += HEADER_SIZE_FULL
            scanned_blocks += 1
        
        print("[OK] Found {0} headers out of {1} scanned blocks".format(len(self.headers), scanned_blocks))
        return len(self.headers)
    
    def parse_relative_mode(self):
        """Parse headers in RELATIVE mode (relative indexing)"""
        print("\n--- Parsing RELATIVE Mode Headers ---")
        offset = 0
        scanned_blocks = 0
        
        while offset + HEADER_SIZE_REL <= len(self.data):
            firstword, secondword = struct.unpack('<II', self.data[offset:offset + HEADER_SIZE_REL])
            index = offset // 4  # Convert byte offset to word index
            
            # Decode the header
            decoded_first = (~index & 0xFFFFFFFF) ^ firstword
            decoded_second = index ^ secondword
            
            # Extract magic
            magic_byte1 = decoded_first & 0xFF
            magic_byte2 = decoded_second & 0xFF
            
            # Check if this block contains a valid magic number (header)
            is_allocated = (magic_byte1 == MAGIC_NUMBER_ALLOCATED_REL and magic_byte2 == MAGIC_NUMBER_ALLOCATED_REL)
            is_freed = (magic_byte1 == MAGIC_NUMBER_FREED_REL and magic_byte2 == MAGIC_NUMBER_FREED_REL)
            
            if is_allocated or is_freed:
                # Extract indices
                prev_idx = (((decoded_first >> 8) & 0xFF) << 16) | \
                           (((decoded_first >> 24) & 0xFF) << 8) | \
                           ((decoded_second >> 8) & 0xFF)
                
                next_idx = (((decoded_first >> 16) & 0xFF) << 16) | \
                           (((decoded_second >> 16) & 0xFF) << 8) | \
                           ((decoded_second >> 24) & 0xFF)
                
                status = "ALLOCATED" if is_allocated else "FREED"
                
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
                    'mode': 'RELATIVE',
                    'status': status
                })
                
                print("  Header at idx=0x{0:06X}: ({1}) prev=0x{2:06X} next=0x{3:06X}".format(
                    index, status, prev_idx, next_idx))
            
            offset += HEADER_SIZE_REL
            scanned_blocks += 1
        
        print("[OK] Found {0} headers out of {1} scanned blocks".format(len(self.headers), scanned_blocks))
        return len(self.headers)
    
    def validate_links(self):
        """Validate that forward and backward links are consistent"""
        print("\n--- Validating Index Links ---")
        
        if not self.headers:
            print("[ERROR] No headers to validate")
            return
        
        # Build index map for fast lookup
        index_map = {h['index']: h for h in self.headers}
        
        mismatch_count = 0
        
        for header in self.headers:
            idx = header['index']
            prev = header['prev']
            nxt = header['next']
            status = header.get('status', 'UNKNOWN')
            
            # Skip validation for freed blocks - they are intentionally disconnected
            # Freed blocks have their prev preserved but next may be removed for gap logic
            if status == "FREED":
                continue
            
            # For allocated blocks, validate bidirectional links
            # Check if next index points to a valid header
            if nxt > 0 and nxt not in index_map:
                msg = "Header 0x{0:06X}: next pointer 0x{1:06X} points to non-existent header".format(idx, nxt)
                self.errors.append(msg)
                mismatch_count += 1
                print("  [MISMATCH] {0}".format(msg))
            elif nxt > 0 and index_map[nxt]['prev'] != idx:
                next_prev = index_map[nxt]['prev']
                msg = "Header 0x{0:06X}: next header 0x{1:06X} has prev=0x{2:06X}, expected 0x{3:06X}".format(
                    idx, nxt, next_prev, idx)
                self.errors.append(msg)
                mismatch_count += 1
                print("  [MISMATCH] {0}".format(msg))
            
            # Check if prev index points to a valid header
            if prev > 0 and prev not in index_map:
                msg = "Header 0x{0:06X}: prev pointer 0x{1:06X} points to non-existent header".format(idx, prev)
                self.errors.append(msg)
                mismatch_count += 1
                print("  [MISMATCH] {0}".format(msg))
            elif prev > 0 and index_map[prev]['next'] != idx:
                prev_next = index_map[prev]['next']
                msg = "Header 0x{0:06X}: prev header 0x{1:06X} has next=0x{2:06X}, expected 0x{3:06X}".format(
                    idx, prev, prev_next, idx)
                self.errors.append(msg)
                mismatch_count += 1
                print("  [MISMATCH] {0}".format(msg))
        
        if mismatch_count == 0:
            print("[OK] All links are consistent")
        else:
            print("[ERROR] Found {0} index link mismatches".format(mismatch_count))
    
    def check_magic_numbers(self):
        """Magic number check (already filtered during parsing)"""
        print("\n--- Magic Number Validation ---")
        print("[OK] All {0} headers have valid magic numbers (filtered during parsing)".format(len(self.headers)))
    
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
