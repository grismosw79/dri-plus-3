"""Bounded HeapAlloc/HeapFree fixture, separate from SDF record arena rewinds."""
class NativeHeapFixture:
    def __init__(self, memory, base, capacity):
        assert base % 16 == 0 and capacity > 0 and capacity % 16 == 0
        self.memory, self.base, self.capacity = memory, base, capacity
        self.free_blocks = [(base, capacity)]
        self.live = {}
        self.allocations = self.frees = self.allocated_bytes = self.freed_bytes = 0
        self.current_bytes = self.peak_bytes = self.highwater = self.reuses = 0
        self.seen = set()

    def allocate(self, size, flags=0):
        assert size >= 0
        span = max(16, (size + 15) & ~15)
        candidates = [(n, i, p) for i, (p, n) in enumerate(self.free_blocks) if n >= span]
        assert candidates, ('bounded native fixture heap exhausted', size, self.current_bytes, self.peak_bytes)
        length, index, pointer = min(candidates)
        self.free_blocks.pop(index)
        if length > span:
            self.free_blocks.append((pointer + span, length - span))
        assert pointer not in self.live
        self.live[pointer] = (size, span)
        self.allocations += 1
        self.allocated_bytes += size
        self.current_bytes += size
        self.peak_bytes = max(self.peak_bytes, self.current_bytes)
        self.highwater = max(self.highwater, pointer + span - self.base)
        self.reuses += pointer in self.seen
        self.seen.add(pointer)
        if flags & 8 and size:
            self.memory.mem_write(pointer, b'\0' * size)
        return pointer

    def release(self, pointer):
        assert pointer in self.live, ('invalid or duplicate native heap free', hex(pointer))
        size, span = self.live.pop(pointer)
        self.memory.mem_write(pointer, b'\xdd' * span)
        self.frees += 1
        self.freed_bytes += size
        self.current_bytes -= size
        self.free_blocks.append((pointer, span))
        self.free_blocks.sort()
        merged = []
        for p, n in self.free_blocks:
            if merged and merged[-1][0] + merged[-1][1] == p:
                merged[-1] = (merged[-1][0], merged[-1][1] + n)
            else:
                merged.append((p, n))
        self.free_blocks = merged
        return 1

    def report(self):
        return {'arena_bytes': self.capacity, 'allocations': self.allocations,
                'frees': self.frees, 'allocated_bytes': self.allocated_bytes,
                'freed_bytes': self.freed_bytes, 'current_bytes': self.current_bytes,
                'peak_bytes': self.peak_bytes, 'arena_highwater': self.highwater,
                'reused_addresses': self.reuses,
                'live_block_sizes': sorted(n for n, span in self.live.values())}


def self_test():
    class Memory:
        def __init__(self): self.data = bytearray(256)
        def mem_write(self, pointer, data): self.data[pointer:pointer + len(data)] = data
    memory = Memory()
    heap = NativeHeapFixture(memory, 0, 256)
    a = heap.allocate(17, 8)
    b = heap.allocate(32, 8)
    memory.mem_write(a, b'\x7f' * 17)
    memory.mem_write(b, b'\xa5' * 32)
    heap.release(a)
    assert heap.allocate(17, 8) == a and memory.data[a:a + 17] == b'\0' * 17
    assert memory.data[b:b + 32] == b'\xa5' * 32
    heap.release(a)
    for bad in [a, 255]:
        try: heap.release(bad)
        except AssertionError: pass
        else: raise AssertionError('invalid free was accepted')
    heap.release(b)
    assert heap.current_bytes == 0 and heap.free_blocks == [(0, 256)]
    assert heap.allocate(256, 8) == 0 and memory.data == bytes(256)
    heap.release(0)
    assert heap.allocations == heap.frees and heap.reuses == 2
    return {'zero_on_reuse': True, 'adjacent_live_block_unchanged': True,
            'invalid_and_double_free_rejected': True, 'coalescing_and_bounded_arena': True}

if __name__ == '__main__':
    import json
    print(json.dumps(self_test(), indent=2))
