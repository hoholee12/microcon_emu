
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
        
        # Find sentinels (start and end of pool)
        print("\n--- Finding Sentinels ---")
        sorted_headers = sorted(self.headers, key=lambda h: h['index'])
        first_header = sorted_headers[0]
        last_header = sorted_headers[-1]
        
        print("  First header at idx=0x{0:06X}, status={1}".format(
            first_header['index'], first_header.get('status', 'UNKNOWN')))
        print("  Last header at idx=0x{0:06X}, status={1}".format(
            last_header['index'], last_header.get('status', 'UNKNOWN')))
        
        # Validate sentinels are allocated blocks
        if first_header.get('status') != 'ALLOCATED':
            msg = "First sentinel at 0x{0:06X} is not ALLOCATED (status={1})".format(
                first_header['index'], first_header.get('status'))
            self.errors.append(msg)
            print("  [ERROR] {0}".format(msg))
        else:
            print("  [OK] First sentinel is ALLOCATED")
        
        if last_header.get('status') != 'ALLOCATED':
            msg = "Last sentinel at 0x{0:06X} is not ALLOCATED (status={1})".format(
                last_header['index'], last_header.get('status'))
            self.errors.append(msg)
            print("  [ERROR] {0}".format(msg))
        else:
            print("  [OK] Last sentinel is ALLOCATED")
        
        # Verify sentinels link to each other (circular structure)
        print("\n--- Validating Sentinel Links (Circular Structure) ---")
        first_prev = first_header['prev']
        last_next = last_header['next']
        
        if first_prev != last_header['index']:
            msg = "First sentinel prev=0x{0:06X}, expected last sentinel at 0x{1:06X}".format(
                first_prev, last_header['index'])
            self.errors.append(msg)
            print("  [ERROR] {0}".format(msg))
        else:
            print("  [OK] First sentinel.prev -> Last sentinel")
        
        if last_next != first_header['index']:
            msg = "Last sentinel next=0x{0:06X}, expected first sentinel at 0x{1:06X}".format(
                last_next, first_header['index'])
            self.errors.append(msg)
            print("  [ERROR] {0}".format(msg))
        else:
            print("  [OK] Last sentinel.next -> First sentinel")
        
        # Validate allocated blocks
        print("\n--- Validating Allocated Blocks ---")
        allocated_blocks = [h for h in self.headers if h.get('status') == 'ALLOCATED']
        
        for header in allocated_blocks:
            idx = header['index']
            prev = header['prev']
            nxt = header['next']
            
            # Rule 1: Allocated block's next must point to another ALLOCATED block (never FREE)
            if nxt in index_map:
                next_header = index_map[nxt]
                next_status = next_header.get('status', 'UNKNOWN')
                
                if next_status == 'FREED':
                    msg = "Allocated block 0x{0:06X}: next=0x{1:06X} points to FREE block (should only point to ALLOCATED)".format(
                        idx, nxt)
                    self.errors.append(msg)
                    print("  [ERROR] {0}".format(msg))
                elif next_status == 'ALLOCATED':
                    print("  [OK] Allocated 0x{0:06X}: next=0x{1:06X} (ALLOCATED)".format(idx, nxt))
                else:
                    msg = "Allocated block 0x{0:06X}: next=0x{1:06X} has unknown status".format(idx, nxt)
                    self.warnings.append(msg)
                    print("  [WARN] {0}".format(msg))
            else:
                if nxt != 0:
                    msg = "Allocated block 0x{0:06X}: next=0x{1:06X} points to non-existent header".format(idx, nxt)
                    self.errors.append(msg)
                    print("  [ERROR] {0}".format(msg))
            
            # Rule 2: Allocated block's prev should eventually reach another ALLOCATED block
            # If it points to a FREE block, trace back through FREE blocks to find ALLOCATED
            if prev in index_map:
                prev_header = index_map[prev]
                prev_status = prev_header.get('status', 'UNKNOWN')
                
                if prev_status == 'ALLOCATED':
                    # Direct link to allocated - perfect
                    print("  [OK] Allocated 0x{0:06X}: prev=0x{1:06X} (ALLOCATED)".format(idx, prev))
                elif prev_status == 'FREED':
                    # Trace back through FREE blocks to find the ALLOCATED block
                    print("  [INFO] Allocated 0x{0:06X}: prev=0x{1:06X} is FREE, tracing back...".format(idx, prev))
                    
                    visited = set()
                    current = prev_header
                    trace_path = [prev]
                    found_allocated = False
                    has_dead_link = False
                    
                    while current and current.get('status') == 'FREED':
                        current_idx = current['index']
                        
                        # Detect cycles
                        if current_idx in visited:
                            msg = "Allocated block 0x{0:06X}: FREE chain has cycle at 0x{1:06X}".format(idx, current_idx)
                            self.errors.append(msg)
                            print("    [ERROR] {0}".format(msg))
                            has_dead_link = True
                            break
                        
                        visited.add(current_idx)
                        
                        # Follow prev link
                        prev_idx = current['prev']
                        if prev_idx in index_map:
                            prev_block = index_map[prev_idx]
                            if prev_block.get('status') == 'ALLOCATED':
                                # Reached allocated block - good
                                trace_path.append(prev_idx)
                                found_allocated = True
                                print("    [OK] Trace: {0} -> 0x{1:06X} (ALLOCATED)".format(
                                    " -> ".join("0x{0:06X}".format(x) for x in trace_path), prev_idx))
                                break
                            elif prev_block.get('status') == 'FREED':
                                # Continue tracing through FREE blocks
                                trace_path.append(prev_idx)
                                current = prev_block
                            else:
                                msg = "Allocated block 0x{0:06X}: FREE chain reaches unknown status at 0x{1:06X}".format(
                                    idx, prev_idx)
                                self.errors.append(msg)
                                print("    [ERROR] {0}".format(msg))
                                has_dead_link = True
                                break
                        else:
                            # Dead link - FREE block points to non-existent header
                            msg = "Allocated block 0x{0:06X}: FREE chain has dead link at 0x{1:06X} (prev=0x{2:06X} does not exist)".format(
                                idx, current_idx, prev_idx)
                            self.errors.append(msg)
                            print("    [ERROR] {0}".format(msg))
                            has_dead_link = True
                            break
                    
                    if not found_allocated and not has_dead_link:
                        msg = "Allocated block 0x{0:06X}: FREE chain does not reach an ALLOCATED block".format(idx)
                        self.errors.append(msg)
                        print("    [ERROR] {0}".format(msg))
                else:
                    msg = "Allocated block 0x{0:06X}: prev=0x{1:06X} has unknown status".format(idx, prev)
                    self.warnings.append(msg)
                    print("  [WARN] {0}".format(msg))
            else:
                if prev != 0:
                    msg = "Allocated block 0x{0:06X}: prev=0x{1:06X} points to non-existent header".format(idx, prev)
                    self.errors.append(msg)
                    print("  [ERROR] {0}".format(msg))
        
        # Info about free blocks (not errors - they're just leftover data)
        print("\n--- Free Block Information (Not Validated) ---")
        free_blocks = [h for h in self.headers if h.get('status') == 'FREED']
        if free_blocks:
            print("  Found {0} FREE blocks (leftover unused data, may be overwritten)".format(len(free_blocks)))
            for fb in free_blocks[:5]:  # Show first 5
                print("    FREE at idx=0x{0:06X}, prev=0x{1:06X}".format(fb['index'], fb['prev']))
            if len(free_blocks) > 5:
                print("    ... and {0} more FREE blocks".format(len(free_blocks) - 5))
        else:
            print("  No FREE blocks found")
        
        if len(self.errors) == 0:
            print("\n[OK] All link validations passed")
        else:
            print("\n[ERROR] Found {0} link validation errors".format(len(self.errors)))
    
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
