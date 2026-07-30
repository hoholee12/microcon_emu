
import struct
import os

MAGIC_NUMBER_ALLOCATED = 0xAAAAAAAA
MAGIC_NUMBER_FREED = 0xCCCCCCCC
MAGIC_NUMBER_ALLOCATED_REL = 0xAA
MAGIC_NUMBER_FREED_REL = 0xCC

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
        self.pool_size_words = 0  # Will be set when file is read
        
    def read_file(self):
        """Read the binary dump file"""
        try:
            with open(self.filename, 'rb') as f:
                self.data = f.read()
            # Calculate pool size from file size
            self.pool_size_words = len(self.data) // 4
            print("[OK] Read {0} bytes from {1} (pool size: 0x{2:X} words, 0x{3:X} bytes)".format(
                len(self.data), self.filename, self.pool_size_words, len(self.data)))
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
        
        # Scan every 4-byte (word) boundary to catch sentinels at any alignment
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
            
            offset += 4  # Scan every word (4 bytes), not every header (12 bytes)
            scanned_blocks += 1
        
        print("[OK] Found {0} headers out of {1} scanned blocks".format(len(self.headers), scanned_blocks))
        return len(self.headers)
    
    def parse_relative_mode(self):
        """Parse headers in RELATIVE mode (relative indexing)"""
        print("\n--- Parsing RELATIVE Mode Headers ---")
        offset = 0
        scanned_blocks = 0
        
        # Scan every 4-byte (word) boundary to catch headers at any alignment
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
                # Extract relative offsets (24-bit values)
                prev_offset = (((decoded_first >> 8) & 0xFF) << 16) | \
                              (((decoded_first >> 24) & 0xFF) << 8) | \
                              ((decoded_second >> 8) & 0xFF)
                
                next_offset = (((decoded_first >> 16) & 0xFF) << 16) | \
                              (((decoded_second >> 16) & 0xFF) << 8) | \
                              ((decoded_second >> 24) & 0xFF)
                
                # Convert relative offsets to absolute indices with pool size wraparound
                # next_idx = index + next_offset
                # prev_idx = index - prev_offset (with pool size underflow)
                next_idx = (index + next_offset) % self.pool_size_words
                prev_idx = (index - prev_offset) % self.pool_size_words
                
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
                    'prev_offset': prev_offset,
                    'next_offset': next_offset,
                    'prev': prev_idx,
                    'next': next_idx,
                    'mode': 'RELATIVE',
                    'status': status
                })
                
                print("  Header at idx=0x{0:06X}: ({1}) prev_offset=0x{2:06X} next_offset=0x{3:06X} -> prev=0x{4:06X} next=0x{5:06X}".format(
                    index, status, prev_offset, next_offset, prev_idx, next_idx))
            
            offset += 4  # Scan every word (4 bytes), not every header (8 bytes)
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
            elif nxt > 0:
                next_header = index_map[nxt]
                # Check if next block's prev points back to us
                if next_header['prev'] != idx:
                    # This is OK if there's a freed (gap) block between us and next
                    # In this case, next.prev points to the freed block, not us
                    if next_header['prev'] in index_map and index_map[next_header['prev']].get('status') == "FREED":
                        # Valid gap structure - allocated block skips over freed block
                        pass
                    else:
                        next_prev = next_header['prev']
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
            elif prev > 0:
                prev_header = index_map[prev]
                # If prev block is freed (gap block), it may not point back to us - this is intentional
                # Freed blocks are used as gap markers and their next may not point back
                if prev_header.get('status') == "FREED":
                    # Valid gap structure - this freed block is a gap between current and prev's prev
                    pass
                elif prev_header['next'] != idx:
                    prev_next = prev_header['next']
                    msg = "Header 0x{0:06X}: prev header 0x{1:06X} has next=0x{2:06X}, expected 0x{3:06X}".format(
                        idx, prev, prev_next, idx)
                    self.errors.append(msg)
                    mismatch_count += 1
                    print("  [MISMATCH] {0}".format(msg))
        
        # Validate freed blocks (gap blocks)
        print("\n--- Validating Free Blocks (Informational) ---")
        for header in self.headers:
            idx = header['index']
            prev = header['prev']
            status = header.get('status', 'UNKNOWN')
            
            # Check freed blocks only
            if status != "FREED":
                continue
            
            # Free blocks should have their prev pointing to an allocated block
            # This ensures they are properly coalesced between allocated blocks
            if prev > 0 and prev not in index_map:
                msg = "Free block 0x{0:06X}: prev pointer 0x{1:06X} is not a valid header (memory corrupted)".format(idx, prev)
                print("  [CORRUPTION_WARN] {0}".format(msg))
            elif prev > 0:
                prev_header = index_map[prev]
                prev_status = prev_header.get('status', 'UNKNOWN')
                
                # Prev must be ALLOCATED, not another FREE block
                if prev_status == "FREED":
                    msg = "Free block 0x{0:06X}: prev is another free block 0x{1:06X} (not coalesced properly)".format(idx, prev)
                    print("  [COALESCE_WARN] {0}".format(msg))
                else:
                    # Valid: free block between two allocated blocks
                    print("  [OK] Free block 0x{0:06X}: prev=0x{1:06X} (allocated)".format(idx, prev))
        
        if mismatch_count == 0:
            print("\n[OK] All links are consistent (gaps properly handled)")
        else:
            print("\n[ERROR] Found {0} index link mismatches".format(mismatch_count))
    
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
            for error in self.errors:
                print("  - {0}".format(error))
        
        if self.warnings:
            print("\nWARNINGS:")
            for warning in self.warnings:
                print("  - {0}".format(warning))
        
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
    validator = PoolValidator("logalloc_dump.bin")
    success = validator.validate()
