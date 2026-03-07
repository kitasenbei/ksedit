# single_line_apocalypse.py
# This creates ONE line designed to destroy naive editors

import random

OUTPUT_FILE = "single_line_apocalypse.txt"

# Total size in bytes (start small, scale up)
# 256 MB, 1 GB, 4 GB are good checkpoints
SIZE_BYTES = 512 * 1024 * 1024  # 512 MB

CHUNK_SIZE = 1024 * 1024  # 1 MB

# Mix of:
# - ASCII
# - multi-byte UTF-8
# - combining characters
# - emojis (4-byte)
CHAR_POOL = [
    "a",
    "b",
    "c",
    "λ",
    "ж",
    "中",
    "💥",
    "🔥",
    "🧠",
    "e\u0301",  # combining accent
    "क",
    "𐍈",  # 4-byte UTF-8
]

with open(OUTPUT_FILE, "w", encoding="utf-8") as f:
    written = 0

    while written < SIZE_BYTES:
        s = "".join(random.choice(CHAR_POOL) for _ in range(1024))
        f.write(s)
        written += len(s.encode("utf-8"))

print(f"Created {OUTPUT_FILE} (~{SIZE_BYTES} bytes) as ONE line")
