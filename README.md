# Low-Latency Limit Order Book (C++)

A from-scratch **limit order book (LOB) matching engine** in C++, built as a series of
optimization stages. Each stage keeps the exact same behavior but swaps in better data
structures, taking the engine from a naive **O(N)** baseline to a **zero-allocation,
cache-friendly** design that is roughly **300× faster** in throughput.

---

## What is a matching engine?

A limit order book holds all the outstanding buy orders (**bids**) and sell orders
(**asks**) for a single instrument (one stock). The matching engine is the code that:

- **Adds** an order. If it doesn't cross the spread it *rests* in the book; if it does cross, it matches immediately.
- **Cancels** a resting order by its ID.
- **Matches** an incoming order against the opposite side, **best price first**, and within a price level **oldest order first** — this rule is called **price–time priority**.
- **Queries** the top of book (best bid / best ask).

Each operation wants a different data structure: matching wants prices kept *sorted* so
the best is cheap to find, the queue at each price wants **FIFO** ordering, and cancel-by-ID
wants **O(1)** lookup. The whole optimization story is about serving all three cheaply.

---

## The optimization journey at a glance

| Stage | Core data structures | Headline change | Complexity per op |
|-------|----------------------|-----------------|-------------------|
| **0 — Naive** | `vector<Order>` | brute-force scan | **O(N)** |
| **1 — Right structures** | `map` of (price → `list`) + `unordered_map` (of order_id → `list::iterator`)| correct algorithm | ~**O(log P)**, O(1) cancel |
| **2 — Array price levels** | `vector<list<Order>>` inplace of `map` + `buy_idx` / `sell_idx` | dropped the tree (maps) | **O(1)** access |
| **3 — Memory pool** | pre-allocated `vector<OrderNode>` all at once | reducing frequent heap allocations | **O(1)**, avoiding `new` usage|
| **4 — Open-addressing hash** | custom `openHash` inplace of `unordered_map` | truly zero-allocation | **O(1)**, fully in-place |

*P = number of distinct price levels, N = total orders in the book.*

---

## Stage 0 — Naive baseline

> File: `Naive_Approach.cpp` — everything is **O(N)**, but it's simple and correct. This is the reference every later stage must agree with.

**Data structures**

- A single `vector<Order>` holding *every* order (both sides mixed together).
- Each `Order` carries `{id, side, price, qty, timestamp}`.

**How they're used**

- **Add / match:** scan the *entire* vector to find the best opposite order (best price, earliest timestamp), trade against it, and `erase` it when filled — repeating until the incoming order is exhausted or rests.
- **Cancel:** scan the vector for the ID, then `erase`.
- **Best bid/ask:** scan the whole vector.

**Why it's slow**

Every single operation is a full linear scan, and `vector::erase` shifts all following
elements (another O(N)). With a book of thousands of orders, each add costs thousands of
nanoseconds. Useful only as a correctness baseline.

---

## Stage 1 — The right data structures

> File: `Stage-1_Optimization.cpp` — pick structures that match the problem: sorted prices + FIFO queues + a lookup table.

**Data structures**

- `map<int, list<Order>, greater<int>> buy` — price → FIFO list of orders, sorted **descending**, so `buy.begin()` is the **highest bid** (the best buy).
- `map<int, list<Order>> sell` — sorted **ascending**, so `sell.begin()` is the **lowest ask** (the best sell).
- `unordered_map<int, list<Order>::iterator> mapping` — order ID → its position in the list.

**How they're used**

- **Best price** is just `map.begin()` — no scanning.
- Each **price level is a `list`** (doubly linked), and we always match from the `front()`. Because new orders are appended to the back, the front is always the oldest → this gives **time priority** for free.
- **Cancel** looks up the iterator in the hash map and erases it from its list directly, no scan.

**Optimization win**

The big jump: every operation goes from **O(N)** to roughly **O(log P)**, and cancel becomes **O(1)** on average.

**What's still wrong**

`std::map` is a red-black **tree**: reaching the best price means hopping through
scattered, heap-allocated nodes — a **cache-miss** on almost every hop. Worse, when a price
level empties we erase it from the tree, forcing rebalancing operations.

---

## Stage 2 — Array-indexed price levels

> File: `Stage-2_Optimization.cpp` — replace the sorted tree with a flat array indexed directly by price.

**Data structures**

- `vector<list<Order>> buy` and `vector<list<Order>> sell`, each sized to the price range (e.g. 20,000). **The price *is* the array index** — `buy[100]` is the FIFO queue at price 100.
- `unordered_map<int, list<Order>::iterator> mapping` — unchanged, still for O(1) cancel.
- `int buy_idx` and `int sell_idx` — remember the current **best price** on each side so we never search for it.

**How they're used**

- **Best level** is a direct array access: `buy[buy_idx]` / `sell[sell_idx]` — **O(1)**, no tree navigation.
- When the best level empties after matching, we step `buy_idx` / `sell_idx` to the next non-empty price.
- Matching within a level still walks the FIFO `list` from the front.

**Optimization win**

- A red-black tree lookup (logarithmic, pointer-chasing through **RAM**) becomes a single array index — contiguous memory that the CPU keeps in **L1/L2 cache**.
- **No rebalancing:** an empty price level is just an empty array slot. We don't delete anything, so there's no tree maintenance.
- `buy_idx` / `sell_idx` keep top-of-book queries **O(1)**.

**What's still wrong**

`std::list` calls `new` for **every** node it stores. That hidden heap allocation on every
`push_back` is both slow and *unpredictable* (anywhere from ~10 ns to ~2000 ns), which
creates latency spikes.

---

## Stage 3 — Pre-allocated memory pool + intrusive linked list

> File: `Stage-3_Optimization.cpp` — stop asking the OS for memory mid-trade. Allocate everything once, up front.

**Data structures**

- `vector<OrderNode> pool` — one massive array (e.g. 1,000,000 slots) pre-allocated at startup. This **is** our memory; we never call `new` again.
- `OrderNode` now stores `{id, qty, price, side, prev_idx, next_idx}` — the `prev_idx` / `next_idx` make it an **intrusive doubly linked list** that links by **array index** instead of pointer.
- `vector<int> buy_heads, buy_tails, sell_heads, sell_tails` — per price, store the head/tail **index** of that level's list.
- A **free list**: `free_head` plus the `next_idx` field threads all unused slots together, so `allocate_index()` and `freeNode()` are **O(1)**.
- `unordered_map<int, int> mapping` — order ID → pool index.

**How they're used**

- **Adding** an order grabs a free slot from the pool (O(1)), writes the order into it, and links its index into the right price level's list via `add_to_list`.
- **Cancelling / filling** unlinks the node (`remove_from_list`) and returns its slot to the free list.

**Optimization win**

This removes **heap allocation from the hot path entirely**. `std::list` was secretly
allocating per order; the pool replaces that with reusing slots in one contiguous array.
The payoff is twofold: **deterministic latency** (no surprise `malloc` stalls) and **cache
locality** (all order nodes live next to each other in one block of memory).

**What's still wrong**

`std::unordered_map` resolves hash collisions by **separate chaining** — a linked list per
bucket — which means it *also* calls `new` on collisions. The exact problem we just
eliminated is still hiding in the ID lookup table.

---

## Stage 4 — Open-addressing hash map (final)

> File: `Stage-4_Optimization.cpp` — replace `unordered_map` with a custom zero-allocation hash table.

**Data structures**

- Everything from Stage 3, **plus** a custom `openHash` that replaces `unordered_map<int,int>`:
  - A single `vector<Entry>` where `Entry = {id, pool_idx}` — one flat, contiguous array.
  - Capacity is a **power of two**, so the hash is `id & mask` (`mask = capacity - 1`) — a bitwise AND instead of an expensive `%` division.
  - Collisions are handled by **linear probing**: on a clash, just step to `(idx + 1) & mask`.
  - Slots use `-1` for empty and `-2` for a deleted **tombstone**; load factor is kept around 0.5 so probe chains stay short.

**How it's used**

- **Insert / get / remove** of an ID → pool index all operate inside that one array, walking forward a few slots on collision. No nodes, no pointers, no allocation.

**Optimization win**

Separate chaining (the standard library's approach) allocates a list node on every
collision — re-introducing the unpredictable `new` we worked to remove. Open addressing
keeps the **entire map in one cache-resident array**, and `id & mask` is faster than modulo.
With this change, **the whole engine is allocation-free on the hot path**: add, cancel,
match, and lookup are all O(1) with no calls to the allocator.

---

## Performance

Measured by replaying an identical order stream through Stage 0 and Stage 4
(single-core Xeon, `-O3 -march=native`). Both engines produced an **identical final book**,
confirming the speedup costs nothing in correctness.

| Metric (mixed workload) | Naive (Stage 0) | Stage 4 | Improvement |
|---|---|---|---|
| Throughput | ~0.11 M ops/s | ~33 M ops/s | **~300×** |
| Median latency (p50) | ~4,350 ns | ~42 ns | **~104×** |
| Tail latency (p99.9) | ~110,000 ns | ~275 ns | **~400×** |

The key result is the **shape**: the naive engine slows down *linearly* as the book fills
(O(N)), while Stage 4 stays flat (O(1)) — so the gap widens the busier the book gets.
